#ifndef LIDAR_SIMULATOR_H
#define LIDAR_SIMULATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef enum
{
    LIDAR_SCENE_FLAT_WALL = 0,
    LIDAR_SCENE_CENTRAL_OBSTACLE,
    LIDAR_SCENE_ROOM_CORNER,
    LIDAR_SCENE_NOISY_INVALID

} LidarSimScene;

typedef struct
{
    uint32_t rng_state;
    uint16_t noise_amplitude_mm;
    uint8_t invalid_percent;

} LidarSimulator;

typedef struct
{
    uint16_t min_mm;
    uint16_t max_mm;
    uint32_t mean_mm;
    uint32_t invalid_count;
    uint32_t valid_count;

} LidarFrameStats;

void LidarSim_Init(LidarSimulator *sim, uint32_t seed);

void LidarSim_SetNoise(LidarSimulator *sim, uint16_t amplitude_mm);

void LidarSim_SetInvalidPercent(LidarSimulator *sim, uint8_t percent);

void LidarSim_Generate(
    LidarSimulator *sim,
    LidarSimScene scene,
    uint32_t timestamp_ms,
    LidarFrame *frame
);

void LidarSim_ComputeStats(
    const LidarFrame *frame,
    LidarFrameStats *stats
);

#ifdef __cplusplus
}
#endif

#endif /* LIDAR_SIMULATOR_H */
