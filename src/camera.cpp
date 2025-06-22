#include "camera.h"
#include <psp2/kernel/sysmem.h>
#include <psp2/display.h>
#include <string.h>
#include <stdio.h>
#include <vita2d.h>

#define DISPLAY_WIDTH 960
#define DISPLAY_HEIGHT 544


static SceUID s_memblock = -1;
static void* s_base = NULL;
static SceDisplayFrameBuf s_dbuf;
static CameraDevice s_current_device = CAMERA_FRONT;
static CameraResolution s_current_resolution = CAMERA_RESOLUTION_640_480;
static CameraFrameRate s_current_framerate = CAMERA_FRAMERATE_30_FPS;
static int s_current_format = 5; // ABGR format
static vita2d_texture* s_camera_texture = NULL;


static const size_t s_resolution_table[][2] = {
    {0, 0},         // Invalid
    {640, 480},     // CAMERA_RESOLUTION_640_480
    {320, 240},     // CAMERA_RESOLUTION_320_240
    {160, 120},     // CAMERA_RESOLUTION_160_120
    {352, 288},     // CAMERA_RESOLUTION_352_288
    {176, 144},     // CAMERA_RESOLUTION_176_144
    {480, 272},     // CAMERA_RESOLUTION_480_272
    {640, 360}      // CAMERA_RESOLUTION_640_360
};


static const size_t s_framerate_table[] = {
    3,  // CAMERA_FRAMERATE_3_FPS
    5,  // CAMERA_FRAMERATE_5_FPS
    7,  // CAMERA_FRAMERATE_7_FPS
    10, // CAMERA_FRAMERATE_10_FPS
    15, // CAMERA_FRAMERATE_15_FPS
    20, // CAMERA_FRAMERATE_20_FPS
    30  // CAMERA_FRAMERATE_30_FPS
};

bool camera_init() {
    s_memblock = sceKernelAllocMemBlock("camera", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, 256 * 1024 * 5, NULL);
    if (s_memblock < 0) {
        return false;
    }
    
    if (sceKernelGetMemBlockBase(s_memblock, &s_base) < 0) {
        sceKernelFreeMemBlock(s_memblock);
        s_memblock = -1;
        return false;
    }
    
    memset(&s_dbuf, 0, sizeof(SceDisplayFrameBuf));
    s_dbuf.size = sizeof(SceDisplayFrameBuf);
    s_dbuf.base = s_base;
    s_dbuf.pitch = DISPLAY_WIDTH;
    s_dbuf.width = DISPLAY_WIDTH;
    s_dbuf.height = DISPLAY_HEIGHT;
    
    return true;
}

bool camera_open(CameraDevice device, CameraResolution resolution, CameraFrameRate framerate) {
    if (s_base == NULL) {
        return false;
    }
    
    s_current_device = device;
    s_current_resolution = resolution;
    s_current_framerate = framerate;
    
    SceCameraInfo info;
    memset(&info, 0, sizeof(SceCameraInfo));
    info.size = sizeof(SceCameraInfo);
    info.format = s_current_format;
    info.resolution = s_current_resolution;
    info.framerate = s_framerate_table[s_current_framerate];
    info.sizeIBase = 4 * s_resolution_table[s_current_resolution][0] * s_resolution_table[s_current_resolution][1];
    info.pitch = 0; 
    info.pIBase = s_base;
    
    int ret = sceCameraOpen(s_current_device, &info);
    return (ret >= 0);
}

bool camera_start() {
    int ret = sceCameraStart(s_current_device);
    return (ret >= 0);
}

bool camera_is_active() {
    return (sceCameraIsActive(s_current_device) > 0);
}

bool camera_read_frame() {
    if (!camera_is_active()) {
        return false;
    }
    
    SceCameraRead read;
    memset(&read, 0, sizeof(SceCameraRead));
    read.size = sizeof(SceCameraRead);
    read.mode = 0; 
    
    int ret = sceCameraRead(s_current_device, &read);
    if (ret < 0) {
        return false;
    }
    
    return true;
}

void* camera_get_frame_buffer() {
    return s_base;
}

SceDisplayFrameBuf* camera_get_display_buffer() {
    return &s_dbuf;
}

int camera_get_width() {
    return s_resolution_table[s_current_resolution][0];
}

int camera_get_height() {
    return s_resolution_table[s_current_resolution][1];
}

vita2d_texture* camera_get_frame_texture() { 
    int width = camera_get_width();
    int height = camera_get_height();
    
    if (s_camera_texture == NULL ||
        vita2d_texture_get_width(s_camera_texture) != width ||
        vita2d_texture_get_height(s_camera_texture) != height) {

        if (s_camera_texture) {
            vita2d_free_texture(s_camera_texture);
        }

        s_camera_texture = vita2d_create_empty_texture(width, height);
        if (s_camera_texture == NULL) {
            return NULL;
        }
    }
    
    void* texture_data = vita2d_texture_get_datap(s_camera_texture);
    void* camera_data = s_base;
    

    memcpy(texture_data, camera_data, width * height * 4);
    
    return s_camera_texture;
}

vita2d_texture* camera_capture_frame() {
    int width = camera_get_width();
    int height = camera_get_height();
    
    vita2d_texture* captured_texture = vita2d_create_empty_texture(width, height);
    if (captured_texture == NULL) {
        return NULL;
    }
    
    void* texture_data = vita2d_texture_get_datap(captured_texture);
    void* camera_data = s_base;
    
    memcpy(texture_data, camera_data, width * height * 4);
    
    return captured_texture;
}

void camera_stop() {
    sceCameraStop(s_current_device);
}

void camera_close() {
    sceCameraClose(s_current_device);
    

    if (s_camera_texture != NULL) {
        vita2d_free_texture(s_camera_texture);
        s_camera_texture = NULL;
    }
}

void camera_terminate() {
    if (s_memblock >= 0) {
        sceKernelFreeMemBlock(s_memblock);
        s_memblock = -1;
        s_base = NULL;
    }
    

    if (s_camera_texture != NULL) {
        vita2d_free_texture(s_camera_texture);
        s_camera_texture = NULL;
    }
}

void camera_toggle_device() {
    s_current_device = (s_current_device == CAMERA_FRONT) ? CAMERA_BACK : CAMERA_FRONT;
}

CameraDevice camera_get_current_device() {
    return s_current_device;
}

void camera_set_resolution(CameraResolution resolution) {
    s_current_resolution = resolution;
}

void camera_set_framerate(CameraFrameRate framerate) {
    s_current_framerate = framerate;
}


bool handle_camera_input(
    SceCtrlData& pad,
    SceCtrlData& old_pad,
    bool& camera_mode_active,
    bool& photo_taken,
    vita2d_texture*& staged_photo,
    vita2d_texture*& photo_to_free
) {
    bool handled = false;
    
    if (camera_mode_active) {
        handled = true;
        
        if (!photo_taken) {
            // Live View
            if (camera_is_active() && camera_read_frame()) {
                if ((pad.buttons & SCE_CTRL_RTRIGGER) && !(old_pad.buttons & SCE_CTRL_RTRIGGER)) {
                    staged_photo = camera_capture_frame();
                    if (staged_photo) {
                        photo_taken = true;
                    }
                }
            }
        } else {
            // Confirmation View
            if ((pad.buttons & SCE_CTRL_LTRIGGER) && !(old_pad.buttons & SCE_CTRL_LTRIGGER)) {
                // Redo
                if (staged_photo) {
                    photo_to_free = staged_photo;
                    staged_photo = NULL;
                }
                photo_taken = false;
            }
            if ((pad.buttons & SCE_CTRL_CIRCLE) && !(old_pad.buttons & SCE_CTRL_CIRCLE)) {
                // Confirm photo and exit camera mode
                camera_mode_active = false;
                camera_stop();
                camera_close();
            }
        }
        
        // Shared camera controls
        if (camera_is_active()) {
            if ((pad.buttons & SCE_CTRL_TRIANGLE) && !(old_pad.buttons & SCE_CTRL_TRIANGLE)) {
                // Toggle between front and back camera
                camera_stop();
                camera_close();
                camera_toggle_device();
                
                // Open and start the camera with the new device
                if (camera_open(camera_get_current_device(), CAMERA_RESOLUTION_640_480, CAMERA_FRAMERATE_30_FPS)) {
                    camera_start();
                }
            }
            
            if ((pad.buttons & SCE_CTRL_CIRCLE) && !(old_pad.buttons & SCE_CTRL_CIRCLE)) {
                // Exit camera mode
                camera_mode_active = false;
                photo_taken = false;
                if (staged_photo) {
                    photo_to_free = staged_photo;
                    staged_photo = NULL;
                }
                camera_stop();
                camera_close();
            }
        }
    }
    
    return handled;
} 