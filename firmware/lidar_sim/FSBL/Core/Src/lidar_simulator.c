#include "lidar_simulator.h"
#include <stddef.h>

static uint32_t prng_next(LidarSimulator *sim)
{
    uint32_t x = sim->rng_state;

    if (x == 0u)
    {
        x = 0xA341316Cu;
    }

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    sim->rng_state = x;

    return x;
}

static int32_t random_signed(
    LidarSimulator *sim,
    uint16_t amplitude
)
{
    if (amplitude == 0u)
    {
        return 0;
    }

    uint32_t span = (uint32_t)amplitude * 2u + 1u;

    return (int32_t)(prng_next(sim) % span) - (int32_t)amplitude;
}

static uint16_t clamp_u16(
    int32_t value,
    uint16_t min_v,
    uint16_t max_v
)
{
    if (value < (int32_t)min_v)
    {
        return min_v;
    }

    if (value > (int32_t)max_v)
    {
        return max_v;
    }

    return (uint16_t)value;
}

static void fill_common_maps(
    LidarFrame *frame,
    uint16_t distance_mm
)
{
    uint32_t r;
    uint32_t c;

    for (r = 0u; r < LIDAR_HEIGHT; ++r)
    {
        for (c = 0u; c < LIDAR_WIDTH; ++c)
        {
            frame->range_mm[r][c] = distance_mm;
            frame->amplitude[r][c] = 1800u;
            frame->ambient[r][c] = 150u;
            frame->dss[r][c] = 7u;
        }
    }
}

static void scene_flat_wall(LidarFrame *frame)
{
    fill_common_maps(frame, 2000u);
}

static void scene_central_obstacle(LidarFrame *frame)
{
    uint32_t r;
    uint32_t c;

    const uint32_t r0 = 12u;
    const uint32_t r1 = 29u;
    const uint32_t c0 = 18u;
    const uint32_t c1 = 35u;

    fill_common_maps(frame, 4000u);

    for (r = r0; r <= r1; ++r)
    {
        for (c = c0; c <= c1; ++c)
        {
            frame->range_mm[r][c] = 1200u;
            frame->amplitude[r][c] = 2300u;
            frame->ambient[r][c] = 120u;
            frame->dss[r][c] = 7u;
        }
    }
}

static void scene_room_corner(LidarFrame *frame)
{
    uint32_t r;
    uint32_t c;

    fill_common_maps(frame, 2500u);

    for (r = 0u; r < LIDAR_HEIGHT; ++r)
    {
        for (c = 0u; c < LIDAR_WIDTH; ++c)
        {
            int32_t d =
                1200
                + (int32_t)c * 55
                + ((int32_t)r - (int32_t)(LIDAR_HEIGHT / 2u)) * 3;

            frame->range_mm[r][c] =
                clamp_u16(d, 500u, 5000u);

            frame->amplitude[r][c] =
                (uint16_t)(2100u - (c * 10u));

            frame->ambient[r][c] = 170u;
            frame->dss[r][c] = 7u;
        }
    }
}

static void scene_noisy_invalid(
    LidarSimulator *sim,
    LidarFrame *frame
)
{
    uint32_t r;
    uint32_t c;

    fill_common_maps(frame, 2500u);

    for (r = 0u; r < LIDAR_HEIGHT; ++r)
    {
        for (c = 0u; c < LIDAR_WIDTH; ++c)
        {
            int32_t d =
                2500 + random_signed(sim, sim->noise_amplitude_mm);

            frame->range_mm[r][c] =
                clamp_u16(d, 50u, 8800u);

            frame->amplitude[r][c] =
                clamp_u16(
                    1800 + random_signed(sim, 120u),
                    0u,
                    65535u
                );

            frame->ambient[r][c] =
                clamp_u16(
                    180 + random_signed(sim, 40u),
                    0u,
                    65535u
                );

            frame->dss[r][c] = 7u;

            if ((prng_next(sim) % 100u) < sim->invalid_percent)
            {
                frame->range_mm[r][c] = LIDAR_INVALID_MM;
                frame->amplitude[r][c] = 0u;
                frame->ambient[r][c] = 0u;
                frame->dss[r][c] = 0u;
            }
        }
    }

    for (r = 8u; r < 13u; ++r)
    {
        for (c = 10u; c < 16u; ++c)
        {
            frame->range_mm[r][c] = LIDAR_INVALID_MM;
            frame->amplitude[r][c] = 0u;
            frame->ambient[r][c] = 0u;
            frame->dss[r][c] = 0u;
        }
    }

    for (r = 25u; r < 32u; ++r)
    {
        for (c = 38u; c < 45u; ++c)
        {
            frame->range_mm[r][c] = LIDAR_INVALID_MM;
            frame->amplitude[r][c] = 0u;
            frame->ambient[r][c] = 0u;
            frame->dss[r][c] = 0u;
        }
    }
}

void LidarSim_Init(
    LidarSimulator *sim,
    uint32_t seed
)
{
    if (sim == NULL)
    {
        return;
    }

    sim->rng_state =
        (seed == 0u) ? 0x12345678u : seed;

    sim->noise_amplitude_mm = 50u;
    sim->invalid_percent = 2u;
}

void LidarSim_SetNoise(
    LidarSimulator *sim,
    uint16_t amplitude_mm
)
{
    if (sim != NULL)
    {
        sim->noise_amplitude_mm = amplitude_mm;
    }
}

void LidarSim_SetInvalidPercent(
    LidarSimulator *sim,
    uint8_t percent
)
{
    if (sim == NULL)
    {
        return;
    }

    sim->invalid_percent =
        (percent > 100u) ? 100u : percent;
}

void LidarSim_Generate(
    LidarSimulator *sim,
    LidarSimScene scene,
    uint32_t timestamp_ms,
    LidarFrame *frame
)
{
    if ((sim == NULL) || (frame == NULL))
    {
        return;
    }

    frame->timestamp_ms = timestamp_ms;

    switch (scene)
    {
        case LIDAR_SCENE_FLAT_WALL:
            scene_flat_wall(frame);
            break;

        case LIDAR_SCENE_CENTRAL_OBSTACLE:
            scene_central_obstacle(frame);
            break;

        case LIDAR_SCENE_ROOM_CORNER:
            scene_room_corner(frame);
            break;

        case LIDAR_SCENE_NOISY_INVALID:
        default:
            scene_noisy_invalid(sim, frame);
            break;
    }
}

void LidarSim_ComputeStats(
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
