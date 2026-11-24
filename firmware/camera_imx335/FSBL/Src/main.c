/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */


#include "camera_types.h"
#include "camera_source.h"
#include "lidar_simulator.h"
#include "lidar_processing.h"
#include "lidar_source.h"
#include "lidar_source_sim.h"
#include <string.h>

#define P03_SNAPSHOT_TEST_ENABLED 0
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

LTDC_HandleTypeDef hltdc;
/* USER CODE BEGIN PV */

static uint32_t g_last_fps_tick = 0;
static uint32_t g_last_frame_count = 0;
static uint32_t g_camera_fps = 0;

static CameraFrame g_camera_source_latest_frame;
static __IO uint32_t g_camera_source_read_count = 0u;
static __IO uint32_t g_camera_source_last_id = 0u;
static __IO uint32_t g_camera_source_last_timestamp_ms = 0u;

/* P03 diagnostic: temporary automatic test, view g_snapshot_test in GDB. */
#if P03_SNAPSHOT_TEST_ENABLED
static CameraFrame g_snapshot_test_frame;
static volatile struct
{
    uint32_t phase; /* 0=wait, 1=capture, 2=held, 3=error */
    uint32_t captures;
    uint32_t releases;
    uint32_t refused_while_held;
    uint32_t integrity_failures;
    uint32_t changed_captures;
    uint32_t errors;
    uint32_t crc_before;
    uint32_t crc_after;
    uint32_t previous_crc;
    uint32_t request_tick;
    uint32_t capture_ms;
    uint32_t hold_tick;
    uint32_t held_ms;
    uint32_t hold_start_frames;
    uint32_t held_pipe1_frames;
    uint32_t frame_id;
    uint32_t timestamp_ms;
    uint32_t address;
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t next_tick;
} g_snapshot_test = {0};
#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void LCD_Init(uint32_t Width, uint32_t Height);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define P04_CASE_COUNT 6u
#define P04_ZONE_COUNT 2268u

typedef struct
{
    uint32_t scene;
    uint32_t noise;
    uint32_t invalid_percent;
    uint32_t min_mm;
    uint32_t max_mm;
    uint32_t mean_mm;
    uint32_t valid_count;
    uint32_t invalid_count;
    uint32_t hash;
} P04Expected;

static const P04Expected p04_expected[P04_CASE_COUNT] =
{
    {0, 50,   2, 2000, 2000, 2000, 2268,    0, 3250742445u},
    {1, 50,   2, 1200, 4000, 3600, 2268,    0, 1354660117u},
    {2, 50,   2, 1137, 4175, 2656, 2268,    0, 2634872127u},
    {3, 50,   2, 2450, 2550, 2500, 2156,  112, 3424744673u},
    {3,  0,   0, 2500, 2500, 2500, 2189,   79, 1515758359u},
    {3, 50, 100,    0,    0,    0,    0, 2268, 3875788133u}
};

typedef struct
{
    uint32_t scene;
    uint32_t noise;
    uint32_t invalid_percent;
    LidarFrameStats stats;
    uint32_t hash;
    uint32_t timestamp_ok;
    uint32_t elapsed_ms;
    uint32_t passed;
} P04Result;

static LidarFrame g_p04_frame;
static LidarSimulator g_p04_sim;

static volatile struct
{
    uint32_t completed;
    uint32_t pass_mask;
    uint32_t failures;
    uint32_t width;
    uint32_t height;
    uint32_t frame_bytes;
    uint32_t last_tick;
    uint32_t start_tick;
    uint32_t start_frames;
    uint32_t start_reads;
    uint32_t camera_elapsed_ms;
    uint32_t camera_frames;
    uint32_t camera_reads;
    uint32_t done;

    P04Result results[P04_CASE_COUNT];

} g_p04 = {0};

static uint32_t P04Hash(const LidarFrame *frame)
{
    uint32_t hash = 2166136261u;

    for (uint32_t r = 0; r < LIDAR_HEIGHT; r++)
    {
        for (uint32_t c = 0; c < LIDAR_WIDTH; c++)
        {
            hash = (hash ^ frame->range_mm[r][c]) * 16777619u;
            hash = (hash ^ frame->amplitude[r][c]) * 16777619u;
            hash = (hash ^ frame->ambient[r][c]) * 16777619u;
            hash = (hash ^ frame->dss[r][c]) * 16777619u;
        }
    }

    return hash;
}

/* P05: additional checks on the existing P04 frame and schedule. */
#define P05_CASE_COUNT 19u
#define P05_CENTROID_EPS 0.0001f

typedef struct
{
    uint32_t scene_index; /* P04 case index, 6 = independent synthetic frame */
    uint16_t threshold_mm;
    uint32_t count;
    uint16_t nearest;
    uint16_t mean;
    float row;
    float col;
} P05Expected;

static const P05Expected p05_expected[P05_CASE_COUNT] =
{
    {0, 1500, 0, 65535, 0, 0.0f, 0.0f},
    {0, 2000, 0, 65535, 0, 0.0f, 0.0f},
    {0, 2001, 2268, 2000, 2000, 20.5f, 26.5f},
    {1, 1500, 324, 1200, 1200, 20.5f, 26.5f},
    {1, 1200, 0, 65535, 0, 0.0f, 0.0f},
    {1, 1201, 324, 1200, 1200, 20.5f, 26.5f},
    {2, 1500, 251, 1137, 1333, 19.103585657f, 2.533864542f},
    {3, 1500, 0, 65535, 0, 0.0f, 0.0f},
    {3, 2500, 1066, 2450, 2474, 20.275797373f, 26.136960600f},
    {4, 1500, 0, 65535, 0, 0.0f, 0.0f},
    {4, 2500, 0, 65535, 0, 0.0f, 0.0f},
    {4, 2501, 2189, 2500, 2500, 20.476016446f, 26.367291000f},
    {5, 1500, 0, 65535, 0, 0.0f, 0.0f},
    {5, 65535, 0, 65535, 0, 0.0f, 0.0f},
    {6, 0, 0, 65535, 0, 0.0f, 0.0f},
    {6, 1, 1, 0, 0, 0.0f, 0.0f},
    {6, 1500, 2, 0, 749, 20.5f, 26.5f},
    {6, 1501, 3, 0, 999, 13.666666667f, 35.333333333f},
    {6, 65535, 5, 0, 14006, 16.6f, 21.4f}
};

typedef struct
{
    uint32_t scene_index;
    uint32_t threshold_mm;
    LidarObstacleInfo obstacle;
    uint32_t elapsed_ms;
    uint32_t passed;
} P05Result;

static volatile struct
{
    uint32_t completed;
    uint32_t pass_mask;
    uint32_t failures;
    uint32_t legacy_stats_mask;
    uint32_t done;
    P05Result results[P05_CASE_COUNT];
} g_p05 = {0};

static uint32_t P05FloatEqual(float a, float b)
{
    float delta = a - b;
    return (delta >= -P05_CENTROID_EPS) && (delta <= P05_CENTROID_EPS);
}

static uint32_t P05StatsEqual(const LidarFrameStats *a,
                              const LidarFrameStats *b)
{
    return a->min_mm == b->min_mm && a->max_mm == b->max_mm &&
           a->mean_mm == b->mean_mm && a->valid_count == b->valid_count &&
           a->invalid_count == b->invalid_count;
}

static void P05TestScene(uint32_t scene_index, const LidarFrame *frame)
{
    for (uint32_t i = 0u; i < P05_CASE_COUNT; ++i)
    {
        const P05Expected *e = &p05_expected[i];
        if (e->scene_index != scene_index)
            continue;

        P05Result result = {0};
        uint32_t start = HAL_GetTick();
        result.scene_index = scene_index;
        result.threshold_mm = e->threshold_mm;
        LidarProcessing_DetectObstacle(frame, e->threshold_mm, &result.obstacle);
        result.elapsed_ms = HAL_GetTick() - start;
        result.passed = result.obstacle.detected == (e->count != 0u) &&
            result.obstacle.close_count == e->count &&
            result.obstacle.nearest_mm == e->nearest &&
            result.obstacle.mean_close_mm == e->mean &&
            P05FloatEqual(result.obstacle.centroid_row, e->row) &&
            P05FloatEqual(result.obstacle.centroid_col, e->col);
        g_p05.results[i] = result;
        if (result.passed)
            g_p05.pass_mask |= (1u << i);
        else
            ++g_p05.failures;
        ++g_p05.completed;
    }
}

static void P05TestSynthetic(LidarFrame *frame)
{
    /* Run before generating P04 case 5; no additional frame allocation. */
    for (uint32_t r = 0u; r < LIDAR_HEIGHT; ++r)
        for (uint32_t c = 0u; c < LIDAR_WIDTH; ++c)
            frame->range_mm[r][c] = LIDAR_INVALID_MM;

    frame->range_mm[0][0] = 0u;
    frame->range_mm[41][53] = 1499u;
    frame->range_mm[0][53] = 1500u;
    frame->range_mm[41][0] = 1501u;
    frame->range_mm[1][1] = 65534u;
    P05TestScene(6u, frame);

    /* P04 case 5 will overwrite all maps and the timestamp. */
}

static void P04TestProcess(void)
{
    uint32_t now = HAL_GetTick();

    if (g_p04.done)
        return;

    if (CameraSource_GetFrameCount() == 0u || now < 1000u)
        return;

    if (g_p04.completed < P04_CASE_COUNT &&
        (now - g_p04.last_tick) >= 500u)
    {
        uint32_t i = g_p04.completed;

        const P04Expected *e = &p04_expected[i];

        P04Result result = {0};

        if (i == 0u)
        {
            g_p04.width = LIDAR_WIDTH;
            g_p04.height = LIDAR_HEIGHT;
            g_p04.frame_bytes = sizeof(LidarFrame);

            g_p04.start_tick = now;
            g_p04.start_frames = CameraSource_GetFrameCount();
            g_p04.start_reads = g_camera_source_read_count;
        }

        result.scene = e->scene;
        result.noise = e->noise;
        result.invalid_percent = e->invalid_percent;

        if (i == P04_CASE_COUNT - 1u)
            P05TestSynthetic(&g_p04_frame);

        LidarSim_Init(&g_p04_sim, 1234u);

        LidarSim_SetNoise(
            &g_p04_sim,
            (uint16_t)e->noise
        );

        LidarSim_SetInvalidPercent(
            &g_p04_sim,
            (uint8_t)e->invalid_percent
        );

        LidarSim_Generate(
            &g_p04_sim,
            (LidarSimScene)e->scene,
            now,
            &g_p04_frame
        );

        LidarProcessing_ComputeStats(
            &g_p04_frame,
            &result.stats
        );
        LidarFrameStats legacy_stats = {0};
        LidarSim_ComputeStats(&g_p04_frame, &legacy_stats);
        uint32_t legacy_ok = P05StatsEqual(&result.stats, &legacy_stats);
        if (legacy_ok)
            g_p05.legacy_stats_mask |= (1u << i);

        result.hash = P04Hash(&g_p04_frame);

        result.timestamp_ok =
            g_p04_frame.timestamp_ms == now;

        result.elapsed_ms = HAL_GetTick() - now;

        result.passed =
            LIDAR_WIDTH == 54u &&
            LIDAR_HEIGHT == 42u &&
            sizeof(LidarFrame) == 15880u &&
            result.stats.min_mm == e->min_mm &&
            result.stats.max_mm == e->max_mm &&
            result.stats.mean_mm == e->mean_mm &&
            result.stats.valid_count == e->valid_count &&
            result.stats.invalid_count == e->invalid_count &&
            result.stats.valid_count +
                result.stats.invalid_count == P04_ZONE_COUNT &&
            result.hash == e->hash &&
            result.timestamp_ok && legacy_ok;

        g_p04.results[i] = result;

        if (result.passed)
            g_p04.pass_mask |= (1u << i);
        else
            g_p04.failures++;

        P05TestScene(i, &g_p04_frame);
        if (i == P04_CASE_COUNT - 1u)
            g_p05.done = 1u;

        g_p04.last_tick = HAL_GetTick();
        g_p04.completed++;
    }

    now = HAL_GetTick();

    if (g_p04.completed == P04_CASE_COUNT &&
        (now - g_p04.start_tick) >= 5000u)
    {
        g_p04.camera_elapsed_ms =
            now - g_p04.start_tick;

        g_p04.camera_frames =
            CameraSource_GetFrameCount() - g_p04.start_frames;

        g_p04.camera_reads =
            g_camera_source_read_count - g_p04.start_reads;

        g_p04.done = 1u;
    }
}


/* P06: same buffer as P04/P05, used only after g_p04.done. */
#define P06_PERIOD_MS 100u
#define P06_SAMPLE_COUNT 101u
#define P06_API_COUNT 22u
#define P06_API_MASK ((1u << P06_API_COUNT) - 1u)

static LidarSource g_p06_source;
static LidarSourceSim g_p06_sim = {
    .scene = LIDAR_SCENE_NOISY_INVALID, .seed = 1234u,
    .noise_mm = 50u, .invalid_percent = 2u
};
static volatile LidarFrameStats g_p06_live_stats;
static volatile LidarObstacleInfo g_p06_live_obstacle;
static volatile struct {
    uint32_t started, done, runtime_fault;
    uint32_t api_completed, api_pass_mask, api_failures;
    uint32_t samples, interval_count, interval_min_ms, interval_max_ms;
    uint32_t first_timestamp_ms, last_timestamp_ms, elapsed_ms;
    uint32_t first_id, last_id, id_failures, timestamp_failures;
    uint32_t hash_first, hash_second, data_failures;
    uint32_t max_generate_ms, max_processing_ms;
    uint32_t start_camera_frames, start_camera_reads;
    uint32_t camera_frames, camera_reads, camera_elapsed_ms, camera_start_ms;
    uint32_t produced, acquired, released, missed_slots, backend_errors;
    uint32_t measurement_pass;
} g_p06;

static void P06Check(uint32_t bit, bool passed)
{
    ++g_p06.api_completed;
    if (passed) g_p06.api_pass_mask |= (1u << bit);
    else ++g_p06.api_failures;
}

typedef struct {
    bool init_ok;
    LidarBackendResult start_result, poll_result;
    uint32_t start_ms;
} P06Fake;

static bool P06FakeInit(void *context)
{
    return ((P06Fake *)context)->init_ok;
}

static LidarBackendResult P06FakeStart(void *context, LidarFrame *frame,
                                     uint32_t now_ms)
{
    P06Fake *f = context;
    f->start_ms = now_ms;
    if (f->start_result == LIDAR_BACKEND_COMPLETE) {
        memset(frame, 0, sizeof(*frame));
        frame->timestamp_ms = now_ms;
    }
    return f->start_result;
}

static LidarBackendResult P06FakePoll(void *context, LidarFrame *frame)
{
    P06Fake *f = context;
    if (f->poll_result == LIDAR_BACKEND_COMPLETE) {
        memset(frame, 0, sizeof(*frame));
        frame->timestamp_ms = f->start_ms;
    }
    return f->poll_result;
}

static const LidarBackendOps p06_fake_ops = {
    P06FakeInit, P06FakeStart, P06FakePoll
};

static void P06TestAPI(void)
{
    /* Simulated ticks here are deliberate, not measured STM32 time. */
    LidarFrameView v = {0};
    LidarSourceCounters c = {0};
    LidarSourceResult r;
    {
        LidarSource s = {0};
        LidarSourceSim sim = {.scene = LIDAR_SCENE_CENTRAL_OBSTACLE,
            .seed = 1234u, .noise_mm = 50u, .invalid_percent = 2u};
        P06Check(0, !LidarSource_IsFrameReady(&s) &&
            LidarSource_GetState(&s) == LIDAR_SOURCE_UNINITIALIZED &&
            LidarSource_Process(&s, 0u) == LIDAR_SOURCE_INVALID_STATE &&
            LidarSource_AcquireFrame(&s, &v) == LIDAR_SOURCE_INVALID_STATE &&
            LidarSource_ReleaseFrame(&s) == LIDAR_SOURCE_INVALID_STATE &&
            LidarSource_GetCounters(&s, &c) == LIDAR_SOURCE_INVALID_STATE);
        P06Check(1, LidarSource_Init(&s, &LidarSourceSim_Ops, &sim,
            &g_p04_frame, 0u, 0u) == LIDAR_SOURCE_INVALID_ARGUMENT &&
            LidarSource_Init(&s, &LidarSourceSim_Ops, &sim,
            NULL, 100u, 0u) == LIDAR_SOURCE_INVALID_ARGUMENT &&
            LidarSource_Process(NULL, 0u) == LIDAR_SOURCE_INVALID_ARGUMENT);
        r = LidarSource_Init(&s, &LidarSourceSim_Ops, &sim,
                            &g_p04_frame, 100u, UINT32_MAX - 49u);
        P06Check(2, r == LIDAR_SOURCE_OK &&
            LidarSource_GetState(&s) == LIDAR_SOURCE_IDLE);
        r = LidarSource_Process(&s, 49u);
        P06Check(3, r == LIDAR_SOURCE_NOT_READY &&
            !LidarSource_IsFrameReady(&s) &&
            LidarSource_AcquireFrame(&s, &v) == LIDAR_SOURCE_NOT_READY);
        r = LidarSource_Process(&s, 50u);
        P06Check(4, r == LIDAR_SOURCE_OK && LidarSource_IsFrameReady(&s) &&
            P04Hash(&g_p04_frame) == 1354660117u &&
            g_p04_frame.timestamp_ms == 50u);
        P06Check(5, LidarSource_AcquireFrame(&s, NULL) ==
            LIDAR_SOURCE_INVALID_ARGUMENT && LidarSource_IsFrameReady(&s));
        r = LidarSource_Process(&s, 150u);
        P06Check(6, r == LIDAR_SOURCE_NOT_READY && LidarSource_IsFrameReady(&s) &&
            P04Hash(&g_p04_frame) == 1354660117u &&
            g_p04_frame.timestamp_ms == 50u);
        r = LidarSource_AcquireFrame(&s, &v);
        P06Check(7, r == LIDAR_SOURCE_OK && v.frame == &g_p04_frame &&
            v.frame_id == 1u && v.timestamp_ms == 50u &&
            LidarSource_GetState(&s) == LIDAR_SOURCE_HELD &&
            !LidarSource_IsFrameReady(&s));
        r = LidarSource_AcquireFrame(&s, &v);
        P06Check(8, r == LIDAR_SOURCE_INVALID_STATE &&
            v.frame == &g_p04_frame && v.frame_id == 1u &&
            v.timestamp_ms == 50u &&
            LidarSource_Init(&s, &LidarSourceSim_Ops, &sim,
                &g_p04_frame, 100u, 150u) == LIDAR_SOURCE_INVALID_STATE);
        r = LidarSource_Process(&s, 350u);
        P06Check(9, r == LIDAR_SOURCE_NOT_READY &&
            LidarSource_GetState(&s) == LIDAR_SOURCE_HELD &&
            P04Hash(&g_p04_frame) == 1354660117u &&
            g_p04_frame.timestamp_ms == 50u && s.counters.missed_slots == 3u);
        r = LidarSource_ReleaseFrame(&s);
        v = (LidarFrameView){0};
        P06Check(10, r == LIDAR_SOURCE_OK &&
            LidarSource_ReleaseFrame(&s) == LIDAR_SOURCE_INVALID_STATE &&
            LidarSource_GetState(&s) == LIDAR_SOURCE_IDLE);
        bool ok = LidarSource_Process(&s, 449u) == LIDAR_SOURCE_NOT_READY;
        r = LidarSource_Process(&s, 450u); ok &= (r == LIDAR_SOURCE_OK);
        r = LidarSource_AcquireFrame(&s, &v); ok &= (r == LIDAR_SOURCE_OK);
        ok &= v.frame_id == 2u && v.timestamp_ms == 450u;
        r = LidarSource_ReleaseFrame(&s); ok &= (r == LIDAR_SOURCE_OK);
        v = (LidarFrameView){0};
        P06Check(11, ok);
        r = LidarSource_Process(&s, 750u); ok = (r == LIDAR_SOURCE_OK);
        r = LidarSource_AcquireFrame(&s, &v); ok &= (r == LIDAR_SOURCE_OK);
        ok &= v.frame_id == 3u && v.timestamp_ms == 750u;
        r = LidarSource_ReleaseFrame(&s); ok &= (r == LIDAR_SOURCE_OK);
        v = (LidarFrameView){0};
        P06Check(12, ok && LidarSource_Process(&s, 750u) == LIDAR_SOURCE_NOT_READY);
        r = LidarSource_GetCounters(&s, &c);
        P06Check(13, r == LIDAR_SOURCE_OK && c.produced == 3u &&
            c.acquired == 3u && c.released == 3u && c.missed_slots == 5u &&
            c.backend_errors == 0u);
    }
    {
        LidarSource s = {0};
        LidarSourceSim bad = {.scene = LIDAR_SCENE_FLAT_WALL,
                             .invalid_percent = 101u};
        r = LidarSource_Init(&s, &LidarSourceSim_Ops, &bad,
                            &g_p04_frame, 100u, 0u);
        P06Check(14, r == LIDAR_SOURCE_BACKEND_ERROR &&
            LidarSource_GetState(&s) == LIDAR_SOURCE_ERROR &&
            LidarSource_Process(&s, 100u) == LIDAR_SOURCE_BACKEND_ERROR &&
            s.counters.backend_errors == 1u);
        P06Fake f = {true, LIDAR_BACKEND_PENDING, LIDAR_BACKEND_PENDING, 0u};
        r = LidarSource_Init(&s, &p06_fake_ops, &f, &g_p04_frame, 100u, 0u);
        P06Check(15, r == LIDAR_SOURCE_OK && s.counters.backend_errors == 0u);
        r = LidarSource_Process(&s, 100u);
        P06Check(16, r == LIDAR_SOURCE_NOT_READY &&
            LidarSource_GetState(&s) == LIDAR_SOURCE_ACQUIRING &&
            LidarSource_AcquireFrame(&s, &v) == LIDAR_SOURCE_NOT_READY);
        r = LidarSource_Process(&s, 200u);
        P06Check(17, r == LIDAR_SOURCE_NOT_READY && s.counters.missed_slots == 1u);
        f.poll_result = LIDAR_BACKEND_COMPLETE;
        r = LidarSource_Process(&s, 201u);
        bool ok = r == LIDAR_SOURCE_OK;
        r = LidarSource_AcquireFrame(&s, &v); ok &= (r == LIDAR_SOURCE_OK);
        ok &= v.frame_id == 1u && v.timestamp_ms == 100u;
        r = LidarSource_ReleaseFrame(&s); ok &= (r == LIDAR_SOURCE_OK);
        v = (LidarFrameView){0};
        P06Check(18, ok);
        f.start_result = LIDAR_BACKEND_FAILED;
        r = LidarSource_Process(&s, 300u);
        P06Check(19, r == LIDAR_SOURCE_BACKEND_ERROR &&
            LidarSource_GetState(&s) == LIDAR_SOURCE_ERROR &&
            s.counters.backend_errors == 1u && !LidarSource_IsFrameReady(&s));
        f.start_result = LIDAR_BACKEND_PENDING;
        f.poll_result = LIDAR_BACKEND_FAILED;
        r = LidarSource_Init(&s, &p06_fake_ops, &f, &g_p04_frame, 100u, 300u);
        ok = r == LIDAR_SOURCE_OK;
        r = LidarSource_Process(&s, 400u); ok &= (r == LIDAR_SOURCE_NOT_READY);
        r = LidarSource_Process(&s, 401u); ok &= (r == LIDAR_SOURCE_BACKEND_ERROR);
        P06Check(20, ok && s.counters.produced == 0u &&
            s.counters.backend_errors == 1u &&
            LidarSource_AcquireFrame(&s, &v) == LIDAR_SOURCE_BACKEND_ERROR);
        f.init_ok = false;
        r = LidarSource_Init(&s, &p06_fake_ops, &f, &g_p04_frame, 100u, 500u);
        P06Check(21, r == LIDAR_SOURCE_BACKEND_ERROR &&
            s.counters.backend_errors == 1u &&
            LidarSource_GetState(&s) == LIDAR_SOURCE_ERROR);
    }
}

static void P06Process(void)
{
    if (!g_p04.done || g_p06.runtime_fault) return;
    if (!g_p06.started) {
        P06TestAPI();
        g_p06.started = 1u;
        if (g_p06.api_pass_mask != P06_API_MASK || g_p06.api_failures != 0u ||
            LidarSource_Init(&g_p06_source, &LidarSourceSim_Ops, &g_p06_sim,
                &g_p04_frame, P06_PERIOD_MS, HAL_GetTick()) != LIDAR_SOURCE_OK) {
            g_p06.runtime_fault = 1u;
            g_p06.done = 1u;
        }
        return;
    }

    uint32_t now = HAL_GetTick();
    LidarSourceResult r = LidarSource_Process(&g_p06_source, now);
    uint32_t generate_ms = HAL_GetTick() - now;
    if (r != LIDAR_SOURCE_OK && r != LIDAR_SOURCE_NOT_READY) {
        g_p06.runtime_fault = 2u;
        g_p06.done = 1u;
        return;
    }
    if (!LidarSource_IsFrameReady(&g_p06_source)) return;
    LidarFrameView view = {0};
    if (LidarSource_AcquireFrame(&g_p06_source, &view) != LIDAR_SOURCE_OK) {
        g_p06.runtime_fault = 3u;
        g_p06.done = 1u;
        return;
    }
    uint32_t process_start = HAL_GetTick();
    LidarFrameStats stats;
    LidarObstacleInfo obstacle;
    LidarProcessing_ComputeStats(view.frame, &stats);
    LidarProcessing_DetectObstacle(view.frame, 1500u, &obstacle);
    g_p06_live_stats = stats;
    g_p06_live_obstacle = obstacle;
    uint32_t processing_ms = HAL_GetTick() - process_start;
    if (!g_p06.done) {
        if (generate_ms > g_p06.max_generate_ms) g_p06.max_generate_ms = generate_ms;
        if (processing_ms > g_p06.max_processing_ms) g_p06.max_processing_ms = processing_ms;
        if (view.frame != &g_p04_frame || view.timestamp_ms != view.frame->timestamp_ms ||
            stats.valid_count + stats.invalid_count != P04_ZONE_COUNT ||
            stats.min_mm < 2450u || stats.max_mm > 2550u ||
            obstacle.detected != 0u || obstacle.close_count != 0u)
            ++g_p06.data_failures;
        if (g_p06.samples == 0u) {
            g_p06.hash_first = P04Hash(view.frame);
            if (g_p06.hash_first != 3424744673u || stats.invalid_count != 112u)
                ++g_p06.data_failures;
            g_p06.first_timestamp_ms = view.timestamp_ms;
            g_p06.first_id = view.frame_id;
            if (view.frame_id != 1u) ++g_p06.id_failures;
            g_p06.interval_min_ms = UINT32_MAX;
        } else {
            if (g_p06.samples == 1u) {
                g_p06.hash_second = P04Hash(view.frame);
                if (g_p06.hash_second != 4087108911u || stats.invalid_count != 113u)
                    ++g_p06.data_failures;
            }
            uint32_t dt = view.timestamp_ms - g_p06.last_timestamp_ms;
            if (dt == 0u || dt > 0x7FFFFFFFu) ++g_p06.timestamp_failures;
            if (view.frame_id - g_p06.last_id != 1u) ++g_p06.id_failures;
            if (dt < g_p06.interval_min_ms) g_p06.interval_min_ms = dt;
            if (dt > g_p06.interval_max_ms) g_p06.interval_max_ms = dt;
            ++g_p06.interval_count;
        }
        g_p06.last_timestamp_ms = view.timestamp_ms;
        g_p06.last_id = view.frame_id;
        ++g_p06.samples;
    }
    if (LidarSource_ReleaseFrame(&g_p06_source) != LIDAR_SOURCE_OK) {
        g_p06.runtime_fault = 4u;
        g_p06.done = 1u;
        return;
    }
    if (g_p06.done) return;
    if (g_p06.samples == 1u) {
        g_p06.camera_start_ms = HAL_GetTick();
        g_p06.start_camera_frames = CameraSource_GetFrameCount();
        g_p06.start_camera_reads = g_camera_source_read_count;
    }
    if (g_p06.samples == P06_SAMPLE_COUNT) {
        LidarSourceCounters counters;
        if (LidarSource_GetCounters(&g_p06_source, &counters) != LIDAR_SOURCE_OK) {
            g_p06.runtime_fault = 5u;
            g_p06.done = 1u;
            return;
        }
        g_p06.elapsed_ms = g_p06.last_timestamp_ms - g_p06.first_timestamp_ms;
        g_p06.camera_elapsed_ms = HAL_GetTick() - g_p06.camera_start_ms;
        g_p06.camera_frames = CameraSource_GetFrameCount() - g_p06.start_camera_frames;
        g_p06.camera_reads = g_camera_source_read_count - g_p06.start_camera_reads;
        g_p06.produced = counters.produced;
        g_p06.acquired = counters.acquired;
        g_p06.released = counters.released;
        g_p06.missed_slots = counters.missed_slots;
        g_p06.backend_errors = counters.backend_errors;
        /* Compare 100 intervals to 9.5..10.5 Hz without float rounding. */
        g_p06.measurement_pass = g_p06.interval_count == 100u &&
            g_p06.elapsed_ms >= 9524u && g_p06.elapsed_ms <= 10526u &&
            g_p06.produced == P06_SAMPLE_COUNT &&
            g_p06.acquired == P06_SAMPLE_COUNT && g_p06.released == P06_SAMPLE_COUNT &&
            g_p06.missed_slots == 0u && g_p06.backend_errors == 0u &&
            g_p06.id_failures == 0u && g_p06.timestamp_failures == 0u &&
            g_p06.data_failures == 0u;
        /* Camera metrics evaluated separately: one pending ISR frame at a
         * measurement boundary can affect the difference by one. */
        g_p06.done = 1u;
    }
}

#if P03_SNAPSHOT_TEST_ENABLED
static uint32_t SnapshotTestCRC(const CameraFrame *frame)
{
    static uint32_t table[256];
    static bool initialized = false;
    if (!initialized)
    {
        for (uint32_t i = 0; i < 256u; i++)
        {
            uint32_t v = i;
            for (uint32_t bit = 0; bit < 8u; bit++)
            {
                v = (v >> 1) ^ ((v & 1u) ? 0xEDB88320u : 0u);
            }
            table[i] = v;
        }
        initialized = true;
    }
    /* Force reads from physical SRAM, not stale cache lines. */
    SCB_InvalidateDCache_by_Addr((uint32_t *)frame->data,
                                 (int32_t)frame->data_size_bytes);
    __DSB();
    uint32_t crc = 0xFFFFFFFFu;
    const uint8_t *data = frame->data;
    for (uint32_t i = 0; i < frame->data_size_bytes; i++)
    {
        crc = (crc >> 8) ^ table[(crc ^ data[i]) & 0xFFu];
    }
    return crc ^ 0xFFFFFFFFu;
}

static void SnapshotTestProcess(void)
{
    uint32_t now = HAL_GetTick();
    g_snapshot_test.errors = CameraSource_GetSnapshotErrorCount();
    if (g_snapshot_test.errors != 0u)
    {
        g_snapshot_test.phase = 3u;
        return;
    }
    switch (g_snapshot_test.phase)
    {
        case 0u:
            if ((int32_t)(now - g_snapshot_test.next_tick) >= 0)
            {
                g_snapshot_test.request_tick = now;
                g_snapshot_test.phase = CameraSource_RequestSnapshot() ? 1u : 3u;
            }
            break;
        case 1u:
            if (CameraSource_IsSnapshotReady() &&
                CameraSource_AcquireSnapshot(&g_snapshot_test_frame))
            {
                g_snapshot_test.frame_id = g_snapshot_test_frame.frame_id;
                g_snapshot_test.timestamp_ms = g_snapshot_test_frame.timestamp_ms;
                g_snapshot_test.address = (uint32_t)(uintptr_t)g_snapshot_test_frame.data;
                g_snapshot_test.size = g_snapshot_test_frame.data_size_bytes;
                g_snapshot_test.width = g_snapshot_test_frame.width;
                g_snapshot_test.height = g_snapshot_test_frame.height;
                g_snapshot_test.format = g_snapshot_test_frame.format;
                g_snapshot_test.capture_ms = g_snapshot_test.timestamp_ms - g_snapshot_test.request_tick;
                g_snapshot_test.crc_before = SnapshotTestCRC(&g_snapshot_test_frame);
                if (g_snapshot_test.captures != 0u &&
                    g_snapshot_test.crc_before != g_snapshot_test.previous_crc)
                {
                    g_snapshot_test.changed_captures++;
                }
                g_snapshot_test.captures++;
                if (CameraSource_RequestSnapshot())
                {
                    g_snapshot_test.integrity_failures++;
                    g_snapshot_test.phase = 3u;
                    break;
                }
                g_snapshot_test.refused_while_held++;
                g_snapshot_test.hold_start_frames = CameraSource_GetFrameCount();
                g_snapshot_test.hold_tick = HAL_GetTick();
                g_snapshot_test.phase = 2u;
            }
            break;
        case 2u:
            if ((now - g_snapshot_test.hold_tick) >= 5000u)
            {
                g_snapshot_test.held_ms = now - g_snapshot_test.hold_tick;
                g_snapshot_test.held_pipe1_frames = CameraSource_GetFrameCount() - g_snapshot_test.hold_start_frames;
                g_snapshot_test.crc_after = SnapshotTestCRC(&g_snapshot_test_frame);
                if (g_snapshot_test.crc_after != g_snapshot_test.crc_before)
                {
                    g_snapshot_test.integrity_failures++;
                    g_snapshot_test.phase = 3u;
                    break;
                }
                g_snapshot_test.previous_crc = g_snapshot_test.crc_before;
                if (!CameraSource_ReleaseSnapshot())
                {
                    g_snapshot_test.phase = 3u;
                    break;
                }
                g_snapshot_test_frame.data = 0;
                g_snapshot_test.releases++;
                g_snapshot_test.next_tick = HAL_GetTick() + 500u;
                g_snapshot_test.phase = 0u;
            }
            break;
        default:
            break;
    }
}
#endif
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_RED);
    /* UART log */
#if USE_COM_LOG
  COM_InitTypeDef COM_Init;

  /* Initialize COM init structure */
  COM_Init.BaudRate   = 115200;
  COM_Init.WordLength = COM_WORDLENGTH_8B;
  COM_Init.StopBits   = COM_STOPBITS_1;
  COM_Init.Parity     = COM_PARITY_NONE;
  COM_Init.HwFlowCtl  = COM_HWCONTROL_NONE;

  BSP_COM_Init(COM1, &COM_Init);

  if (BSP_COM_SelectLogPort(COM1) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
#endif
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  CameraSource_PrepareHardware();

  /* USER CODE BEGIN 2 */
  LCD_Init(FRAME_WIDTH, FRAME_HEIGHT);

  CameraSource_Start();
  #if P03_SNAPSHOT_TEST_ENABLED
  g_snapshot_test.next_tick = HAL_GetTick() + 3000u;
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    BSP_LED_Toggle(LED_GREEN);
    if (!CameraSource_Process())
    {
      BSP_LED_Toggle(LED_RED);
    }
    /* USER CODE BEGIN 3 */
    if (CameraSource_IsFrameReady())
    {
      if (CameraSource_GetLatestFrame(
              &g_camera_source_latest_frame))
      {
        g_camera_source_read_count++;
        g_camera_source_last_id =
            g_camera_source_latest_frame.frame_id;
        g_camera_source_last_timestamp_ms =
            g_camera_source_latest_frame.timestamp_ms;
      }
    }
    #if P03_SNAPSHOT_TEST_ENABLED
    SnapshotTestProcess();
#endif
    P04TestProcess();
    P06Process();
    if ((HAL_GetTick() - g_last_fps_tick) >= 1000u)
    {
        g_camera_fps = CameraSource_GetFrameCount() - g_last_frame_count;

        g_last_frame_count = CameraSource_GetFrameCount();
        g_last_fps_tick = HAL_GetTick();
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /** Configure the System Power Supply */
  if (HAL_PWREx_ConfigSupply(PWR_EXTERNAL_SOURCE_SUPPLY ) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable HSI
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_NONE;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Get current CPU/System buses clocks configuration and
 if necessary switch to intermediate HSI clock to ensure target clock can be set
  */
  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
  if((RCC_ClkInitStruct.CPUCLKSource == RCC_CPUCLKSOURCE_IC1) ||
     (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_IC2_IC6_IC11))
  {
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_CPUCLK|RCC_CLOCKTYPE_SYSCLK);
    RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_HSI;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL1.PLLM = 4;
  RCC_OscInitStruct.PLL1.PLLN = 75;
  RCC_OscInitStruct.PLL1.PLLFractional = 0;
  RCC_OscInitStruct.PLL1.PLLP1 = 1;
  RCC_OscInitStruct.PLL1.PLLP2 = 1;
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_CPUCLK|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2|RCC_CLOCKTYPE_PCLK5
                              |RCC_CLOCKTYPE_PCLK4;
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV1;
  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 2;
  RCC_ClkInitStruct.IC2Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC2Selection.ClockDivider = 3;
  RCC_ClkInitStruct.IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC6Selection.ClockDivider = 3;
  RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC11Selection.ClockDivider = 3;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

static void LCD_Init(uint32_t Width, uint32_t Height)
{
  LTDC_LayerCfgTypeDef pLayerCfg ={0};

  hltdc.Instance = LTDC;

  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;

  hltdc.Init.HorizontalSync     = RK050HR18_HSYNC - 1;
  hltdc.Init.AccumulatedHBP     = RK050HR18_HSYNC + RK050HR18_HBP - 1;
  hltdc.Init.AccumulatedActiveW = RK050HR18_HSYNC + Width + RK050HR18_HBP -1;
  hltdc.Init.TotalWidth         = RK050HR18_HSYNC + Width + RK050HR18_HBP + RK050HR18_HFP - 1;
  hltdc.Init.VerticalSync       = RK050HR18_VSYNC - 1;
  hltdc.Init.AccumulatedVBP     = RK050HR18_VSYNC + RK050HR18_VBP - 1;
  hltdc.Init.AccumulatedActiveH = RK050HR18_VSYNC + Height + RK050HR18_VBP -1 ;
  hltdc.Init.TotalHeigh         = RK050HR18_VSYNC + Height + RK050HR18_VBP + RK050HR18_VFP - 1;

  hltdc.Init.Backcolor.Blue  = 0x0;
  hltdc.Init.Backcolor.Green = 0xFF;
  hltdc.Init.Backcolor.Red   = 0x0;

  if(HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }

  pLayerCfg.WindowX0       = 0;
  pLayerCfg.WindowX1       = Width;
  pLayerCfg.WindowY0       = 0;
  pLayerCfg.WindowY1       = Height;
  pLayerCfg.PixelFormat    = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.FBStartAdress  = BUFFER_ADDRESS;
  pLayerCfg.Alpha = LTDC_LxCACR_CONSTA;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.ImageWidth = Width;
  pLayerCfg.ImageHeight = Height;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if(HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, LTDC_LAYER_1))
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while (1)
  {
    HAL_Delay(250);
    BSP_LED_Toggle(LED_RED);

  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* Infinite loop */
  while (1)
  {
  }
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
