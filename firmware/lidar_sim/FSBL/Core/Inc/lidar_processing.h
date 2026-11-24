#ifndef LIDAR_PROCESSING_H
#define LIDAR_PROCESSING_H

#include "lidar_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Original statistics semantics, including NULL arguments (no write). */
void LidarProcessing_ComputeStats(
    const LidarFrame *frame,
    LidarFrameStats *stats
);

/* Valid zone: d != LIDAR_INVALID_MM; close zone: d < threshold_mm.
 * No detection: count/mean/centroids/detected = 0, nearest = 0xFFFF.
 * NULL arguments: no write. Frame is read-only. Mean is truncated.
 */
void LidarProcessing_DetectObstacle(
    const LidarFrame *frame,
    uint16_t threshold_mm,
    LidarObstacleInfo *info
);

#ifdef __cplusplus
}
#endif

#endif /* LIDAR_PROCESSING_H */
