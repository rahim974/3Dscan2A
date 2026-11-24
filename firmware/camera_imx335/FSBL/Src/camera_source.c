#include "camera_source.h"
#include "main.h"
#include "isp_api.h"
#include "imx335_E27_isp_param_conf.h"

/* Etat du pipeline materiel camera. */
DCMIPP_HandleTypeDef hdcmipp;
ISP_HandleTypeDef hcamera_isp;

static __IO uint32_t NbMainFrames = 0;
static CameraFrame g_camera_frame;

static IMX335_Object_t   IMX335Obj;
static int32_t isp_gain;
static int32_t isp_exposure;
/* Calculate division factor for a given source and destination dimension (width or height) */
#define DIV_FACTOR(SRC, DST) (((uint32_t)((1024 * DST) / SRC)) > 1023 ? 1023 : ((uint32_t)((1024 * DST) / SRC)))

/* Calculate down scale ratio in unsigned 3.13 fixed-point format */
#define DOWNSCALE_RATIO(SRC, DST) (((uint32_t)(((float_t)(SRC) / (float_t)(DST)) * 8192) < 8192) ? 8192 : \
                                   ((((uint32_t)(((float_t)(SRC) / (float_t)(DST)) * 8192)) > 65535) ? 65535 : \
                                   ((uint32_t)(((float_t)(SRC) / (float_t)(DST)) * 8192))))

static void MX_DCMIPP_Init(void);
static void IMX335_Probe(uint32_t Resolution, uint32_t PixelFormat);
static ISP_StatusTypeDef GetSensorInfoHelper(uint32_t Instance, ISP_SensorInfoTypeDef *SensorInfo);
static ISP_StatusTypeDef SetSensorGainHelper(uint32_t Instance, int32_t Gain);
static ISP_StatusTypeDef GetSensorGainHelper(uint32_t Instance, int32_t *Gain);
static ISP_StatusTypeDef SetSensorExposureHelper(uint32_t Instance, int32_t Exposure);
static ISP_StatusTypeDef GetSensorExposureHelper(uint32_t Instance, int32_t *Exposure);


/* Configuration fixe, initialisee avant l'acquisition. */
static CameraFrame s_frame_template;

/* Publies par l'ISR, lus par la boucle principale. */
static volatile uint32_t s_published_id = 0u;
static volatile uint32_t s_published_timestamp = 0u;
static volatile bool s_initialized = false;

/* Modifie uniquement par la boucle principale. */
static uint32_t s_consumed_id = 0u;

void CameraSource_Init(const CameraFrame *initial_frame)
{
    if (initial_frame == 0)
    {
        return;
    }

    s_initialized = false;
    s_frame_template = *initial_frame;
    s_published_id = initial_frame->frame_id;
    s_published_timestamp = initial_frame->timestamp_ms;
    s_consumed_id = initial_frame->frame_id;
    s_initialized = true;
}

void CameraSource_OnFrameEvent(uint32_t frame_id,
                               uint32_t timestamp_ms)
{
    if (!s_initialized)
    {
        return;
    }

    /* L'identifiant est publie en dernier. */
    s_published_timestamp = timestamp_ms;
    s_published_id = frame_id;
}

bool CameraSource_IsFrameReady(void)
{
    return s_initialized &&
           (s_published_id != s_consumed_id);
}

bool CameraSource_GetLatestFrame(CameraFrame *out_frame)
{
    uint32_t primask;

    if (out_frame == 0)
    {
        return false;
    }

    primask = __get_PRIMASK();
    __disable_irq();

    if (!s_initialized ||
        s_published_id == s_consumed_id)
    {
        __set_PRIMASK(primask);
        return false;
    }

    *out_frame = s_frame_template;
    out_frame->frame_id = s_published_id;
    out_frame->timestamp_ms = s_published_timestamp;

    s_consumed_id = out_frame->frame_id;

    __set_PRIMASK(primask);
    return true;
}

/* P03: one independent, cache-line-aligned DMA destination. */
#define SNAPSHOT_TIMEOUT_MS 500u
static uint8_t s_snapshot_buffer[FRAME_BUFFER_SIZE]
    __attribute__((section(".camera_snapshot"), aligned(32), used));
_Static_assert((FRAME_BUFFER_SIZE % 32u) == 0u, "Snapshot cache alignment");

typedef enum
{
    SNAPSHOT_FREE = 0,
    SNAPSHOT_CAPTURING,
    SNAPSHOT_COMPLETE,
    SNAPSHOT_READY,
    SNAPSHOT_HELD,
    SNAPSHOT_ERROR
} SnapshotState;

static volatile SnapshotState s_snapshot_state = SNAPSHOT_FREE;
static volatile uint32_t s_snapshot_id = 0u;
static volatile uint32_t s_snapshot_timestamp = 0u;
static volatile uint32_t s_snapshot_errors = 0u;
static uint32_t s_snapshot_request_tick;

static volatile uint32_t s_snapshot_startup_csi_errors = 0u;
static volatile uint32_t s_snapshot_startup_csi_code = 0u;
static volatile uint32_t s_snapshot_startup_csi_last_tick = 0u;
static volatile uint32_t s_snapshot_startup_csi_last_lane = 0u;

static void SnapshotError(void)
{
    s_snapshot_errors++;
    if (s_snapshot_state == SNAPSHOT_CAPTURING ||
        s_snapshot_state == SNAPSHOT_COMPLETE ||
        s_snapshot_state == SNAPSHOT_READY ||
        s_snapshot_state == SNAPSHOT_FREE)
    {
        s_snapshot_state = SNAPSHOT_ERROR;
    }
    /* A HELD buffer remains owned by its consumer; no rearm on error. */
}

static bool SnapshotHardwareIdle(void)
{
    return ((hdcmipp.Instance->CMSR1 & DCMIPP_CMSR1_P2CPTACT) == 0u) &&
           ((hdcmipp.Instance->P2FCTCR & DCMIPP_P2FCTCR_CPTREQ) == 0u);
}

static bool SnapshotHardwareClean(void)
{
    return (HAL_DCMIPP_GetError(&hdcmipp) == 0u) &&
           ((CSI->SR1 & (DCMIPP_CSI_FLAG_ECTRLDL0 |
                         DCMIPP_CSI_FLAG_ECTRLDL1)) == 0u) &&
           ((hdcmipp.Instance->CMSR2 & (DCMIPP_FLAG_PIPE2_OVR |
             DCMIPP_FLAG_AXI_TRANSFER_ERROR)) == 0u) &&
           (hdcmipp.PipeState[DCMIPP_PIPE2] == HAL_DCMIPP_PIPE_STATE_READY);
}

bool CameraSource_RequestSnapshot(void)
{
    if (s_snapshot_state != SNAPSHOT_FREE || s_snapshot_errors != 0u ||
        !s_initialized)
    {
        return false;
    }
    if (!SnapshotHardwareIdle() || !SnapshotHardwareClean())
    {
        SnapshotError();
        return false;
    }

    /* Transfer ownership CPU -> DCMIPP, no dirty cache lines remain. */
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)s_snapshot_buffer,
                                     (int32_t)sizeof(s_snapshot_buffer));
    __DSB();
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (s_snapshot_state != SNAPSHOT_FREE || s_snapshot_errors != 0u)
    {
        __set_PRIMASK(primask);
        return false;
    }
    if (!SnapshotHardwareIdle() || !SnapshotHardwareClean() ||
        (s_snapshot_request_tick == 0u &&
         s_snapshot_startup_csi_errors != 0u &&
         (HAL_GetTick() - s_snapshot_startup_csi_last_tick) < 100u))
    {
        SnapshotError();
        __set_PRIMASK(primask);
        return false;
    }

    /* Restore DPHY control-error monitoring before capture. */
    __HAL_DCMIPP_CSI_DPHY_ENABLE_IT(
        CSI,
        DCMIPP_CSI_IT_ECTRLDL0 | DCMIPP_CSI_IT_ECTRLDL1);

    s_snapshot_request_tick = HAL_GetTick();
    s_snapshot_state = SNAPSHOT_CAPTURING;
    __set_PRIMASK(primask);

    if (HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp, DCMIPP_PIPE2,
            DCMIPP_VIRTUAL_CHANNEL0, (uint32_t)(uintptr_t)s_snapshot_buffer,
            DCMIPP_MODE_SNAPSHOT) != HAL_OK)
    {
        primask = __get_PRIMASK();
        __disable_irq();
        SnapshotError();
        __set_PRIMASK(primask);
        return false;
    }
    return s_snapshot_state != SNAPSHOT_ERROR;
}

static void SnapshotProcess(void)
{
    if (s_snapshot_state == SNAPSHOT_COMPLETE)
    {
        if (!SnapshotHardwareIdle() || !SnapshotHardwareClean())
        {
            uint32_t primask = __get_PRIMASK();
            __disable_irq();
            SnapshotError();
            __set_PRIMASK(primask);
            return;
        }
        /* Transfer ownership DCMIPP -> CPU only after completion. */
        __DSB();
        SCB_InvalidateDCache_by_Addr((uint32_t *)s_snapshot_buffer,
                                     (int32_t)sizeof(s_snapshot_buffer));
        __DSB();
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        if (s_snapshot_state == SNAPSHOT_COMPLETE && s_snapshot_errors == 0u)
        {
            s_snapshot_state = SNAPSHOT_READY;
        }
        __set_PRIMASK(primask);
    }
    else if (s_snapshot_state == SNAPSHOT_CAPTURING &&
             (HAL_GetTick() - s_snapshot_request_tick) > SNAPSHOT_TIMEOUT_MS)
    {
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        if (s_snapshot_state == SNAPSHOT_CAPTURING)
        {
            SnapshotError();
        }
        __set_PRIMASK(primask);
    }
}

bool CameraSource_IsSnapshotReady(void)
{
    return s_snapshot_state == SNAPSHOT_READY;
}

bool CameraSource_AcquireSnapshot(CameraFrame *out_frame)
{
    if (out_frame == 0)
    {
        return false;
    }
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (s_snapshot_state != SNAPSHOT_READY || s_snapshot_errors != 0u)
    {
        __set_PRIMASK(primask);
        return false;
    }
    *out_frame = s_frame_template;
    out_frame->data = s_snapshot_buffer;
    out_frame->frame_id = s_snapshot_id;
    out_frame->timestamp_ms = s_snapshot_timestamp;
    s_snapshot_state = SNAPSHOT_HELD;
    __set_PRIMASK(primask);
    return true;
}

bool CameraSource_ReleaseSnapshot(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (s_snapshot_state != SNAPSHOT_HELD)
    {
        __set_PRIMASK(primask);
        return false;
    }
    s_snapshot_state = s_snapshot_errors == 0u ? SNAPSHOT_FREE : SNAPSHOT_ERROR;
    __set_PRIMASK(primask);
    return true;
}

uint32_t CameraSource_GetSnapshotErrorCount(void)
{
    return s_snapshot_errors;
}

/* Preparation avant initialisation du LCD. */
void CameraSource_PrepareHardware(void)
{
    MX_DCMIPP_Init();
    IMX335_Probe(IMX335_R2592_1944, IMX335_RAW_RGGB10);
}

void CameraSource_Start(void)
{
    ISP_AppliHelpersTypeDef appliHelpers = {0};

  /* Fill init struct with Camera driver helpers */
  appliHelpers.GetSensorInfo = GetSensorInfoHelper;
  appliHelpers.SetSensorGain = SetSensorGainHelper;
  appliHelpers.GetSensorGain = GetSensorGainHelper;
  appliHelpers.SetSensorExposure = SetSensorExposureHelper;
  appliHelpers.GetSensorExposure = GetSensorExposureHelper;

  /* Initialize the Image Signal Processing middleware */
  if(ISP_Init(&hcamera_isp, &hdcmipp, 0, &appliHelpers, ISP_IQParamCacheInit[0]) != ISP_OK)
  {
    Error_Handler();
  }

  g_camera_frame.frame_id = NbMainFrames;
  g_camera_frame.timestamp_ms = HAL_GetTick();

  g_camera_frame.width = FRAME_WIDTH;
  g_camera_frame.height = FRAME_HEIGHT;

  g_camera_frame.format = CAMERA_PIXEL_FORMAT_RGB565;

  g_camera_frame.data = (uint8_t *)BUFFER_ADDRESS;

  g_camera_frame.data_size_bytes =
      FRAME_WIDTH * FRAME_HEIGHT * 2u;

  CameraSource_Init(&g_camera_frame);

  if (HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0 , BUFFER_ADDRESS, DCMIPP_MODE_CONTINUOUS) != HAL_OK)
  {
    Error_Handler();
  }

  /* Start the Image Signal Processing */
  if (ISP_Start(&hcamera_isp) != ISP_OK)
  {
    Error_Handler();
  }
}


bool CameraSource_Process(void)
{
    SnapshotProcess();
    return ISP_BackgroundProcess(&hcamera_isp) == ISP_OK;
}

uint32_t CameraSource_GetFrameCount(void)
{
    return NbMainFrames;
}

/**
  * @brief DCMIPP Initialization Function
  * @param None
  * @retval None
  */
static void MX_DCMIPP_Init(void)
{
  /* USER CODE BEGIN DCMIPP_Init 0 */
  /* USER CODE END DCMIPP_Init 0 */
  DCMIPP_PipeConfTypeDef pPipeConf = {0};
  DCMIPP_CSI_PIPE_ConfTypeDef pCSIPipeConf = {0};
  DCMIPP_CSI_ConfTypeDef csiconf = {0};
  DCMIPP_DownsizeTypeDef DonwsizeConf ={0};

  /* Set DCMIPP instance */
  hdcmipp.Instance = DCMIPP;
  if (HAL_DCMIPP_Init(&hdcmipp) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure the CSI */
  csiconf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csiconf.NumberOfLanes   = DCMIPP_CSI_TWO_DATA_LANES;
  csiconf.PHYBitrate      = DCMIPP_CSI_PHY_BT_1600;
  if(HAL_DCMIPP_CSI_SetConfig(&hdcmipp, &csiconf) != HAL_OK)
  {
    Error_Handler();
  }
  /* Configure the Virtual Channel 0 */
  /* Set Virtual Channel config */
  if(HAL_DCMIPP_CSI_SetVCConfig(&hdcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP10) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure the serial Pipe */
  pCSIPipeConf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
  pCSIPipeConf.DataTypeIDA  = DCMIPP_DT_RAW10;
  pCSIPipeConf.DataTypeIDB  = DCMIPP_DT_RAW10; /* Don't Care */


  if (HAL_DCMIPP_CSI_PIPE_SetConfig(&hdcmipp, DCMIPP_PIPE1, &pCSIPipeConf) != HAL_OK)
  {
    Error_Handler();
  }

  pPipeConf.FrameRate  = DCMIPP_FRAME_RATE_ALL;
  pPipeConf.PixelPackerFormat = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1;

  /* Set Pitch for Main and Ancillary Pipes */
  pPipeConf.PixelPipePitch  = 1600 ; /* Number of bytes */

  /* Configure Pipe */
  if (HAL_DCMIPP_PIPE_SetConfig(&hdcmipp, DCMIPP_PIPE1, &pPipeConf) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure the downsize */
  DonwsizeConf.HSize       = 800; /* Destination Image Width  */
  DonwsizeConf.VSize       = 480; /* Destination Image Height */
  DonwsizeConf.HRatio      = DOWNSCALE_RATIO(IMX335_WIDTH, DonwsizeConf.HSize);
  DonwsizeConf.VRatio      = DOWNSCALE_RATIO(IMX335_HEIGHT, DonwsizeConf.VSize);
  DonwsizeConf.HDivFactor  = DIV_FACTOR(IMX335_WIDTH, DonwsizeConf.HSize);
  DonwsizeConf.VDivFactor  = DIV_FACTOR(IMX335_HEIGHT, DonwsizeConf.VSize);

  if(HAL_DCMIPP_PIPE_SetDownsizeConfig(&hdcmipp, DCMIPP_PIPE1, &DonwsizeConf) != HAL_OK)
  {
    Error_Handler();
  }
  if(HAL_DCMIPP_PIPE_EnableDownsize(&hdcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    Error_Handler();
  }
  /* P03: share Pipe1 ISP input; configure independent RGB565 output. */
  if (HAL_DCMIPP_PIPE_CSI_EnableShare(&hdcmipp, DCMIPP_PIPE2) != HAL_OK ||
      HAL_DCMIPP_PIPE_SetConfig(&hdcmipp, DCMIPP_PIPE2, &pPipeConf) != HAL_OK ||
      HAL_DCMIPP_PIPE_SetDownsizeConfig(&hdcmipp, DCMIPP_PIPE2, &DonwsizeConf) != HAL_OK ||
      HAL_DCMIPP_PIPE_EnableDownsize(&hdcmipp, DCMIPP_PIPE2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DCMIPP_Init 2 */
  /* USER CODE END DCMIPP_Init 2 */
}

/**
  * @brief  Register Bus IOs if component ID is OK
  * @retval error status
  */
static void IMX335_Probe(uint32_t Resolution, uint32_t PixelFormat)
{
  IMX335_IO_t              IOCtx;
  uint32_t                 id;

  /* Configure the camera driver */
  IOCtx.Address     = CAMERA_IMX335_ADDRESS;
  IOCtx.Init        = BSP_I2C1_Init;
  IOCtx.DeInit      = BSP_I2C1_DeInit;
  IOCtx.ReadReg     = BSP_I2C1_ReadReg16;
  IOCtx.WriteReg    = BSP_I2C1_WriteReg16;
  IOCtx.GetTick     = BSP_GetTick;

  if (IMX335_RegisterBusIO(&IMX335Obj, &IOCtx) != IMX335_OK)
  {
    Error_Handler();
  }
  else if (IMX335_ReadID(&IMX335Obj, &id) != IMX335_OK)
  {
    Error_Handler();
  }
  else
  {
    if (id != (uint32_t) IMX335_CHIP_ID)
    {
      Error_Handler();
    }
    else
    {
      if (IMX335_Init(&IMX335Obj, Resolution, PixelFormat) != IMX335_OK)
      {
        Error_Handler();
      }
      else if(IMX335_SetFrequency(&IMX335Obj, IMX335_INCK_24MHZ)!= IMX335_OK)
      {
        Error_Handler();
      }
      else if (IMX335_MirrorFlipConfig(&IMX335Obj, IMX335_MIRROR) != IMX335_OK)
      {
        Error_Handler();
      }
      else
      {
        return;
      }
    }
  }
}

/**
  * @brief  ISP Middleware helper. Camera sensor info getter
  * @retval ISP Status
  */
static ISP_StatusTypeDef GetSensorInfoHelper(uint32_t Instance, ISP_SensorInfoTypeDef *SensorInfo)
{
  UNUSED(Instance);
  return (ISP_StatusTypeDef) IMX335_GetSensorInfo(&IMX335Obj, (IMX335_SensorInfo_t *) SensorInfo);
}

/**
  * @brief  ISP Middleware helper. Camera gain setter
  * @retval ISP Status
  */
static ISP_StatusTypeDef SetSensorGainHelper(uint32_t Instance, int32_t Gain)
{
  UNUSED(Instance);
  isp_gain = Gain;
  return (ISP_StatusTypeDef) IMX335_SetGain(&IMX335Obj, Gain);
}

/**
  * @brief  ISP Middleware helper. Camera gain getter
  * @retval ISP Status
  */
static ISP_StatusTypeDef GetSensorGainHelper(uint32_t Instance, int32_t *Gain)
{
  UNUSED(Instance);
  *Gain = isp_gain;
  return ISP_OK;
}

/**
  * @brief  ISP Middleware helper. Camera exposure setter
  * @retval ISP Status
  */
static ISP_StatusTypeDef SetSensorExposureHelper(uint32_t Instance, int32_t Exposure)
{
  UNUSED(Instance);
  isp_exposure = Exposure;
  return (ISP_StatusTypeDef) IMX335_SetExposure(&IMX335Obj, Exposure);
}

/**
  * @brief  ISP Middleware helper. Camera exposure getter
  * @retval ISP Status
  */
static ISP_StatusTypeDef GetSensorExposureHelper(uint32_t Instance, int32_t *Exposure)
{
  UNUSED(Instance);
  *Exposure = isp_exposure;
  return ISP_OK;
}


void HAL_DCMIPP_PIPE_FrameEventCallback(
    DCMIPP_HandleTypeDef *hdcmipp,
    uint32_t Pipe)
{
    if (Pipe == DCMIPP_PIPE2)
    {
        if (s_snapshot_state == SNAPSHOT_CAPTURING)
        {
            if (SnapshotHardwareIdle() && SnapshotHardwareClean())
            {
                s_snapshot_timestamp = HAL_GetTick();
                s_snapshot_id++;
                __DMB();
                s_snapshot_state = SNAPSHOT_COMPLETE;
            }
            else
            {
                SnapshotError();
            }
        }
        return;
    }
    if (Pipe != DCMIPP_PIPE1)
    {
        return;
    }
    UNUSED(hdcmipp);
    NbMainFrames++;

    g_camera_frame.frame_id = NbMainFrames;
    g_camera_frame.timestamp_ms = HAL_GetTick();
    CameraSource_OnFrameEvent(g_camera_frame.frame_id, g_camera_frame.timestamp_ms);
}


/**
 * @brief  Vsync Event callback on pipe
 * @param  hdcmipp DCMIPP device handle
 *         Pipe    Pipe receiving the callback
 * @retval None
 */
void HAL_DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  UNUSED(hdcmipp);
  /* Update the frame counter and call the ISP statistics handler */
  switch (Pipe)
  {
    case DCMIPP_PIPE0 :
      ISP_IncDumpFrameId(&hcamera_isp);
      break;
    case DCMIPP_PIPE1 :
      ISP_IncMainFrameId(&hcamera_isp);
      ISP_GatherStatistics(&hcamera_isp);
      break;
    case DCMIPP_PIPE2 :
      ISP_IncAncillaryFrameId(&hcamera_isp);
      break;
  }
}

/* ISR error reporting; no blocking HAL operations here. */
void HAL_DCMIPP_PIPE_ErrorCallback(DCMIPP_HandleTypeDef *handle, uint32_t Pipe)
{
    UNUSED(handle);
    UNUSED(Pipe);
    SnapshotError();
}

void HAL_DCMIPP_ErrorCallback(DCMIPP_HandleTypeDef *handle)
{
    UNUSED(handle);
    SnapshotError();
}

void HAL_DCMIPP_CSI_LineErrorCallback(DCMIPP_HandleTypeDef *handle, uint32_t lane)
{
    uint32_t code = HAL_DCMIPP_GetError(handle);

    /*
     * Only the observed startup DPHY CTRL incident is
     * recoverable. Preserve its diagnostic information.
     * Any error after the first request remains fatal.
     */
    if (s_snapshot_state == SNAPSHOT_FREE &&
        s_snapshot_request_tick == 0u &&
        code == HAL_DCMIPP_CSI_ERROR_DPHY_CTRL)
    {
        s_snapshot_startup_csi_errors++;
        s_snapshot_startup_csi_code |= code;
        s_snapshot_startup_csi_last_tick = HAL_GetTick();
        s_snapshot_startup_csi_last_lane = lane;

        handle->ErrorCode &= ~HAL_DCMIPP_CSI_ERROR_DPHY_CTRL;
        return;
    }

    SnapshotError();
}
