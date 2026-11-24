#ifndef LIDAR_SOURCE_SIM_H
#define LIDAR_SOURCE_SIM_H

#include "lidar_source.h"
#include "lidar_simulator.h"

typedef struct {
    LidarSimulator simulator;
    LidarSimScene scene;
    uint32_t seed;
    uint16_t noise_mm;
    uint8_t invalid_percent;
} LidarSourceSim;

/* Configure before LidarSource_Init; leave unchanged while active. */
extern const LidarBackendOps LidarSourceSim_Ops;
#endif
