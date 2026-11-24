#include "camera_source.h"
#include "main.h"

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
