#ifndef LIDAR_TYPES_H
#define LIDAR_TYPES_H

#include <stdint.h>

#define LIDAR_WIDTH       54u
#define LIDAR_HEIGHT      42u
#define LIDAR_INVALID_MM  0xFFFFu

typedef struct
{
    uint32_t timestamp_ms;

    uint16_t range_mm[LIDAR_HEIGHT][LIDAR_WIDTH];
    uint16_t amplitude[LIDAR_HEIGHT][LIDAR_WIDTH];
    uint16_t ambient[LIDAR_HEIGHT][LIDAR_WIDTH];
    uint8_t  dss[LIDAR_HEIGHT][LIDAR_WIDTH];

} LidarFrame;

typedef struct
{
    uint16_t min_mm;
    uint16_t max_mm;
    uint32_t mean_mm;
    uint32_t invalid_count;
    uint32_t valid_count;

} LidarFrameStats;

/* Centroid is zero-based, unweighted; no detection => (0, 0). */
typedef struct
{
    uint32_t close_count;
    uint16_t nearest_mm;
    uint16_t mean_close_mm;
    float centroid_row;
    float centroid_col;
    uint8_t detected;
} LidarObstacleInfo;

#endif /* LIDAR_TYPES_H */
