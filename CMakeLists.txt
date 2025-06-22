cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(vela)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Vela")
set(VITA_TITLEID  "VELA00001")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(
  ./common
)

set(SOURCES src/main.cpp src/net.cpp src/ui.cpp src/keyboard.cpp src/settings.cpp src/camera.cpp src/image_utils.cpp src/sessions.cpp src/persistence.cpp src/input.cpp src/app.cpp)

add_executable(${PROJECT_NAME}
  ${SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    -lvita2d
    -lSceDisplay_stub
    -lSceGxm_stub
    -lSceSysmodule_stub
    -lSceCtrl_stub
    -lSceTouch_stub
    -lScePgf_stub
    -lSceAppMgr_stub
    -lSceAppUtil_stub
    -lSceIofilemgr_stub
    -lSceNet_stub
    -lSceNetCtl_stub
    -lSceHttp_stub
    -lSceSsl_stub
    -lSceCommonDialog_stub
    -lSceCamera_stub
    -ljsoncpp
    -lpng
    -lz
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  
  # UI icons
  FILE img/camera.png img/camera.png
  FILE img/gear.png img/gear.png
  FILE img/history.png img/history.png
)
