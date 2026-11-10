#ifndef CAMERA_TYPES_H
#define CAMERA_TYPES_H

#include <stdint.h>

typedef enum
{
    CAMERA_PIXEL_FORMAT_UNKNOWN = 0,
    CAMERA_PIXEL_FORMAT_RGB565,
    CAMERA_PIXEL_FORMAT_RGB888,
    CAMERA_PIXEL_FORMAT_RAW10
} CameraPixelFormat;

typedef struct
{
    uint32_t frame_id;
    uint32_t timestamp_ms;

    uint16_t width;
    uint16_t height;

    CameraPixelFormat format;

    uint8_t *data;
    uint32_t data_size_bytes;

} CameraFrame;

#endif /* CAMERA_TYPES_H */
