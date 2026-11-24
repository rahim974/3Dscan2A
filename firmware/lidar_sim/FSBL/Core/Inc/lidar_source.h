#ifndef LIDAR_SOURCE_H
#define LIDAR_SOURCE_H

#include <stdbool.h>
#include <stdint.h>
#include "lidar_types.h"

typedef enum {
    LIDAR_SOURCE_UNINITIALIZED = 0,
    LIDAR_SOURCE_IDLE,
    LIDAR_SOURCE_ACQUIRING,
    LIDAR_SOURCE_READY,
    LIDAR_SOURCE_HELD,
    LIDAR_SOURCE_ERROR
} LidarSourceState;

typedef enum {
    LIDAR_SOURCE_OK = 0,
    LIDAR_SOURCE_NOT_READY,
    LIDAR_SOURCE_INVALID_ARGUMENT,
    LIDAR_SOURCE_INVALID_STATE,
    LIDAR_SOURCE_BACKEND_ERROR
} LidarSourceResult;

typedef enum {
    LIDAR_BACKEND_COMPLETE = 0,
    LIDAR_BACKEND_PENDING,
    LIDAR_BACKEND_FAILED
} LidarBackendResult;

/* Backend borrows the buffer only from start until COMPLETE or FAILED.
 * COMPLETE publishes all maps and timestamp_ms. FAILED must cease writes.
 * PENDING means poll is required; no ISR calls into LidarSource.
 * Backend-specific timeouts and DMA/cache handling belong to the backend.
 */
typedef struct {
    bool (*init)(void *context);
    LidarBackendResult (*start)(void *context, LidarFrame *frame,
                                uint32_t now_ms);
    LidarBackendResult (*poll)(void *context, LidarFrame *frame);
} LidarBackendOps;

typedef struct {
    const LidarFrame *frame;
    uint32_t frame_id;
    uint32_t timestamp_ms;
} LidarFrameView;

typedef struct {
    uint32_t produced;
    uint32_t acquired;
    uint32_t released;
    uint32_t missed_slots;
    uint32_t backend_errors;
} LidarSourceCounters;

/* Caller allocates and zero-initializes the context once. All calls are
 * single-consumer, main-loop only. Members are private by convention.
 * Buffer and backend context must outlive the source; do not write them
 * directly while the source is active. Init is allowed from UNINITIALIZED
 * or ERROR only. Period is in [1, 0x7FFFFFFF] ms. Call Process regularly,
 * with monotonic modulo-2^32 ticks and less than one full wrap between calls.
 */
typedef struct {
    const LidarBackendOps *ops;
    void *backend_context;
    LidarFrame *buffer;
    LidarSourceState state;
    uint32_t period_ms;
    uint32_t last_slot_ms;
    LidarSourceCounters counters;
} LidarSource;

LidarSourceResult LidarSource_Init(LidarSource *source,
    const LidarBackendOps *ops, void *backend_context, LidarFrame *buffer,
    uint32_t period_ms, uint32_t now_ms);
/* OK means a new frame was published in this call, not merely READY. */
LidarSourceResult LidarSource_Process(LidarSource *source, uint32_t now_ms);
bool LidarSource_IsFrameReady(const LidarSource *source);
/* Acquire transfers exclusive read-only access until Release. On failure,
 * the output view is unchanged. Discard its pointer after successful Release.
 */
LidarSourceResult LidarSource_AcquireFrame(LidarSource *source,
                                         LidarFrameView *view);
LidarSourceResult LidarSource_ReleaseFrame(LidarSource *source);
LidarSourceState LidarSource_GetState(const LidarSource *source);
LidarSourceResult LidarSource_GetCounters(const LidarSource *source,
                                         LidarSourceCounters *out);
#endif
