#include "lidar_source_sim.h"

static bool SimInit(void *context)
{
    LidarSourceSim *sim = context;
    if (!sim || (uint32_t)sim->scene > LIDAR_SCENE_NOISY_INVALID ||
        sim->invalid_percent > 100u)
        return false;
    LidarSim_Init(&sim->simulator, sim->seed);
    LidarSim_SetNoise(&sim->simulator, sim->noise_mm);
    LidarSim_SetInvalidPercent(&sim->simulator, sim->invalid_percent);
    return true;
}

static LidarBackendResult SimStart(void *context, LidarFrame *frame,
                                  uint32_t now_ms)
{
    LidarSourceSim *sim = context;
    if (!sim || !frame) return LIDAR_BACKEND_FAILED;
    LidarSim_Generate(&sim->simulator, sim->scene, now_ms, frame);
    return LIDAR_BACKEND_COMPLETE;
}

static LidarBackendResult SimPoll(void *context, LidarFrame *frame)
{
    (void)context;
    (void)frame;
    return LIDAR_BACKEND_FAILED; /* synchronous backend: never pending */
}

const LidarBackendOps LidarSourceSim_Ops = {SimInit, SimStart, SimPoll};
