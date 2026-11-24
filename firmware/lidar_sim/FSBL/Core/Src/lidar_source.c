#include "lidar_source.h"
#include <stddef.h>

static LidarSourceResult Finish(LidarSource *source, LidarBackendResult result)
{
    if (result == LIDAR_BACKEND_COMPLETE) {
        ++source->counters.produced;
        source->state = LIDAR_SOURCE_READY;
        return LIDAR_SOURCE_OK;
    }
    if (result == LIDAR_BACKEND_PENDING) {
        source->state = LIDAR_SOURCE_ACQUIRING;
        return LIDAR_SOURCE_NOT_READY;
    }
    ++source->counters.backend_errors;
    source->state = LIDAR_SOURCE_ERROR;
    return LIDAR_SOURCE_BACKEND_ERROR;
}

LidarSourceResult LidarSource_Init(LidarSource *source,
    const LidarBackendOps *ops, void *backend_context, LidarFrame *buffer,
    uint32_t period_ms, uint32_t now_ms)
{
    if (!source || !ops || !ops->init || !ops->start || !ops->poll ||
        !buffer || period_ms == 0u || period_ms > 0x7FFFFFFFu)
        return LIDAR_SOURCE_INVALID_ARGUMENT;
    if (source->state != LIDAR_SOURCE_UNINITIALIZED &&
        source->state != LIDAR_SOURCE_ERROR)
        return LIDAR_SOURCE_INVALID_STATE;
    *source = (LidarSource){0};
    source->ops = ops;
    source->backend_context = backend_context;
    source->buffer = buffer;
    source->period_ms = period_ms;
    source->last_slot_ms = now_ms;
    if (!ops->init(backend_context))
        return Finish(source, LIDAR_BACKEND_FAILED);
    source->state = LIDAR_SOURCE_IDLE;
    return LIDAR_SOURCE_OK;
}

LidarSourceResult LidarSource_Process(LidarSource *source, uint32_t now_ms)
{
    if (!source) return LIDAR_SOURCE_INVALID_ARGUMENT;
    if (source->state == LIDAR_SOURCE_UNINITIALIZED)
        return LIDAR_SOURCE_INVALID_STATE;
    if (source->state == LIDAR_SOURCE_ERROR)
        return LIDAR_SOURCE_BACKEND_ERROR;

    uint32_t slots = (now_ms - source->last_slot_ms) / source->period_ms;
    source->last_slot_ms += slots * source->period_ms;
    if (source->state == LIDAR_SOURCE_ACQUIRING) {
        source->counters.missed_slots += slots;
        return Finish(source, source->ops->poll(source->backend_context,
                                               source->buffer));
    }
    if (source->state != LIDAR_SOURCE_IDLE) {
        source->counters.missed_slots += slots;
        return LIDAR_SOURCE_NOT_READY;
    }
    if (slots == 0u) return LIDAR_SOURCE_NOT_READY;
    source->counters.missed_slots += slots - 1u;
    source->state = LIDAR_SOURCE_ACQUIRING;
    return Finish(source, source->ops->start(source->backend_context,
                                            source->buffer, now_ms));
}

bool LidarSource_IsFrameReady(const LidarSource *source)
{
    return source && source->state == LIDAR_SOURCE_READY;
}

LidarSourceResult LidarSource_AcquireFrame(LidarSource *source,
                                         LidarFrameView *view)
{
    if (!source || !view) return LIDAR_SOURCE_INVALID_ARGUMENT;
    if (source->state == LIDAR_SOURCE_ERROR)
        return LIDAR_SOURCE_BACKEND_ERROR;
    if (source->state == LIDAR_SOURCE_IDLE ||
        source->state == LIDAR_SOURCE_ACQUIRING)
        return LIDAR_SOURCE_NOT_READY;
    if (source->state != LIDAR_SOURCE_READY)
        return LIDAR_SOURCE_INVALID_STATE;
    view->frame = source->buffer;
    view->frame_id = source->counters.produced;
    view->timestamp_ms = source->buffer->timestamp_ms;
    ++source->counters.acquired;
    source->state = LIDAR_SOURCE_HELD;
    return LIDAR_SOURCE_OK;
}

LidarSourceResult LidarSource_ReleaseFrame(LidarSource *source)
{
    if (!source) return LIDAR_SOURCE_INVALID_ARGUMENT;
    if (source->state == LIDAR_SOURCE_ERROR)
        return LIDAR_SOURCE_BACKEND_ERROR;
    if (source->state != LIDAR_SOURCE_HELD)
        return LIDAR_SOURCE_INVALID_STATE;
    ++source->counters.released;
    source->state = LIDAR_SOURCE_IDLE;
    return LIDAR_SOURCE_OK;
}

LidarSourceState LidarSource_GetState(const LidarSource *source)
{
    return source ? source->state : LIDAR_SOURCE_UNINITIALIZED;
}

LidarSourceResult LidarSource_GetCounters(const LidarSource *source,
                                         LidarSourceCounters *out)
{
    if (!source || !out) return LIDAR_SOURCE_INVALID_ARGUMENT;
    if (source->state == LIDAR_SOURCE_UNINITIALIZED)
        return LIDAR_SOURCE_INVALID_STATE;
    *out = source->counters;
    return LIDAR_SOURCE_OK;
}
