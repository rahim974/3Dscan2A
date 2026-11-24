#include "lidar_processing.h"
#include <stddef.h>

void LidarProcessing_ComputeStats(
    const LidarFrame *frame,
    LidarFrameStats *stats
)
{
    uint32_t r;
    uint32_t c;
    uint64_t sum = 0u;

    if ((frame == NULL) || (stats == NULL))
    {
        return;
    }

    stats->min_mm = 0xFFFFu;
    stats->max_mm = 0u;
    stats->mean_mm = 0u;
    stats->invalid_count = 0u;
    stats->valid_count = 0u;

    for (r = 0u; r < LIDAR_HEIGHT; ++r)
    {
        for (c = 0u; c < LIDAR_WIDTH; ++c)
        {
            uint16_t d = frame->range_mm[r][c];

            if (d == LIDAR_INVALID_MM)
            {
                stats->invalid_count++;
                continue;
            }

            if (d < stats->min_mm)
            {
                stats->min_mm = d;
            }

            if (d > stats->max_mm)
            {
                stats->max_mm = d;
            }

            sum += d;
            stats->valid_count++;
        }
    }

    if (stats->valid_count > 0u)
    {
        stats->mean_mm =
            (uint32_t)(sum / stats->valid_count);
    }
    else
    {
        stats->min_mm = 0u;
        stats->max_mm = 0u;
        stats->mean_mm = 0u;
    }
}

void LidarProcessing_DetectObstacle(
    const LidarFrame *frame,
    uint16_t threshold_mm,
    LidarObstacleInfo *info
)
{
    uint32_t row_sum = 0u;
    uint32_t col_sum = 0u;
    uint32_t distance_sum = 0u;

    if ((frame == NULL) || (info == NULL))
    {
        return;
    }

    info->close_count = 0u;
    info->nearest_mm = LIDAR_INVALID_MM;
    info->mean_close_mm = 0u;
    info->centroid_row = 0.0f;
    info->centroid_col = 0.0f;
    info->detected = 0u;

    for (uint32_t r = 0u; r < LIDAR_HEIGHT; ++r)
    {
        for (uint32_t c = 0u; c < LIDAR_WIDTH; ++c)
        {
            uint16_t d = frame->range_mm[r][c];
            if ((d == LIDAR_INVALID_MM) || (d >= threshold_mm))
            {
                continue;
            }
            ++info->close_count;
            distance_sum += d;
            row_sum += r;
            col_sum += c;
            if (d < info->nearest_mm)
            {
                info->nearest_mm = d;
            }
        }
    }

    if (info->close_count != 0u)
    {
        info->detected = 1u;
        info->mean_close_mm = (uint16_t)(distance_sum / info->close_count);
        info->centroid_row = (float)row_sum / (float)info->close_count;
        info->centroid_col = (float)col_sum / (float)info->close_count;
    }
}
