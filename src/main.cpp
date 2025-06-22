#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/clib.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/net/http.h>
#include <psp2/libssl.h>
#include <psp2/paf.h>
#include <psp2/sysmodule.h>
#include <psp2/common_dialog.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/system_param.h>
#include <psp2/ime_dialog.h>
#include <vita2d.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <algorithm>
#include <memory>
#include <thread>
#include <chrono>
#include <jsoncpp/json/json.h>
#include <math.h>

#include "config.h"
#include "net.h"
#include "ui.h"
#include "keyboard.h"
#include "types.h"
#include "settings.h"
#include "persistence.h"
#include "camera.h"
#include "image_utils.h"
#include "sessions.h"
#include "input.h"
#include "app.h"



bool camera_initialized = false;
bool camera_mode_active = false;
bool photo_taken = false;
vita2d_texture* staged_photo = NULL;
vita2d_texture* photo_to_free = NULL;

int main(int argc, char *argv[]) {
	AppContext ctx;
	initialize_app(ctx);
	run_app(ctx);
	cleanup_app(ctx);
                            
    
    sceKernelExitProcess(0);
	return 0;
}