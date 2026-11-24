#ifndef LIDAR_SIMULATOR_H
#define LIDAR_SIMULATOR_H

#include "lidar_types.h"

#ifdef __cplusplus
extern "C" {
#endif

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
