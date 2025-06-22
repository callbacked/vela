#ifndef CAMERA_H
#define CAMERA_H

#include <psp2/ctrl.h>
#include <psp2/camera.h>
#include <psp2/display.h>
#include <stdbool.h>
#include <vita2d.h>

typedef enum {
    CAMERA_RESOLUTION_640_480 = 1,
    CAMERA_RESOLUTION_320_240 = 2,
    CAMERA_RESOLUTION_160_120 = 3,
    CAMERA_RESOLUTION_352_288 = 4,
    CAMERA_RESOLUTION_176_144 = 5,
    CAMERA_RESOLUTION_480_272 = 6,
    CAMERA_RESOLUTION_640_360 = 7
} CameraResolution;

typedef enum {
    CAMERA_FRONT = 0,
    CAMERA_BACK = 1
} CameraDevice;

typedef enum {
    CAMERA_FRAMERATE_3_FPS = 0,
    CAMERA_FRAMERATE_5_FPS = 1,
    CAMERA_FRAMERATE_7_FPS = 2,
    CAMERA_FRAMERATE_10_FPS = 3,
    CAMERA_FRAMERATE_15_FPS = 4,
    CAMERA_FRAMERATE_20_FPS = 5,
    CAMERA_FRAMERATE_30_FPS = 6
} CameraFrameRate;

bool camera_init();

bool camera_open(CameraDevice device, CameraResolution resolution, CameraFrameRate framerate);

bool camera_start();

bool camera_is_active();

bool camera_read_frame();

void* camera_get_frame_buffer();

SceDisplayFrameBuf* camera_get_display_buffer();

int camera_get_width();

int camera_get_height();

vita2d_texture* camera_get_frame_texture();

vita2d_texture* camera_capture_frame();

void camera_stop();

void camera_close();

void camera_terminate();

void camera_toggle_device();

CameraDevice camera_get_current_device();

void camera_set_resolution(CameraResolution resolution);

void camera_set_framerate(CameraFrameRate framerate);

bool handle_camera_input(
    SceCtrlData& pad,
    SceCtrlData& old_pad,
    bool& camera_mode_active,
    bool& photo_taken,
    vita2d_texture*& staged_photo,
    vita2d_texture*& photo_to_free
);

#endif 