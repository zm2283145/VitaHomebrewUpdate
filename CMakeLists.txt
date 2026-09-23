cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "VitaSDK toolchain")
    else()
        message(FATAL_ERROR "Set VITASDK before configuring")
    endif()
endif()

project(LiveAreaUpdatePrototype C)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -O2")

set(LAU_TEST_TITLE_ID "LAUTEST01" CACHE STRING "Title ID for the disposable update target")
set(LAU_TEST_APP_VERSION "01.00" CACHE STRING "Displayed and packaged test-app version")
set(LAU_DEFAULT_URL "http://192.168.1.2:8080/payload.pkg" CACHE STRING
    "Default URL used when ux0:data/LAUpdate/url.txt is absent")
set(LAU_UPDATE_XML_URL
    "https://raw.githubusercontent.com/zm2283145/VitaHomebrewUpdate/main/test-feed/LAUTEST01-ver.xml"
    CACHE STRING "Update XML embedded in the disposable test application")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test-app/sce_sys/homebrew_update.ini.in
    ${CMAKE_CURRENT_BINARY_DIR}/test-app/sce_sys/homebrew_update.ini
    @ONLY
)

add_library(HomebrewUpdateClient STATIC
    client/src/homebrew_update_client.c
)
target_include_directories(HomebrewUpdateClient PUBLIC client/include)
target_link_libraries(HomebrewUpdateClient PUBLIC SceNet_stub)

add_library(lau_bgdl STATIC common/bgdl.c)
target_include_directories(lau_bgdl PUBLIC common)
target_link_libraries(lau_bgdl PUBLIC taihen_stub SceKernelModulemgr_stub)

add_executable(lau_queue_probe
    queue-probe/main.c
    "$ENV{VITASDK}/share/gcc-arm-vita-eabi/samples/common/debugScreen.c"
)
target_compile_definitions(lau_queue_probe PRIVATE LAU_DEFAULT_URL="${LAU_DEFAULT_URL}")
target_include_directories(lau_queue_probe PRIVATE
    "$ENV{VITASDK}/share/gcc-arm-vita-eabi/samples/common"
)
target_link_libraries(lau_queue_probe
    lau_bgdl
    SceCtrl_stub
    SceDisplay_stub
    SceIofilemgr_stub
)
vita_create_self(lau-queue-probe-eboot.bin lau_queue_probe UNSAFE)
vita_create_vpk(lau-queue-probe.vpk LAUQUEUE1 lau-queue-probe-eboot.bin
    VERSION 01.00
    NAME "LiveArea Update Queue Probe"
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/../deploy/agent/assets/sce_sys/icon0.png sce_sys/icon0.png
)

add_executable(lau_test_app
    test-app/main.c
    "$ENV{VITASDK}/share/gcc-arm-vita-eabi/samples/common/debugScreen.c"
)
target_compile_definitions(lau_test_app PRIVATE LAU_TEST_APP_VERSION="${LAU_TEST_APP_VERSION}")
target_include_directories(lau_test_app PRIVATE
    client/include
    "$ENV{VITASDK}/share/gcc-arm-vita-eabi/samples/common"
)
target_link_libraries(lau_test_app
    HomebrewUpdateClient
    SceCtrl_stub
    SceDisplay_stub
    SceNet_stub
    SceSysmodule_stub
)
vita_create_self(lau-test-app-eboot.bin lau_test_app)
set(VITA_MKSFOEX_FLAGS
    "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE=0x8000 -d ATTRIBUTE2=0 -d ATTRIBUTE_MINOR=0x10 -s CATEGORY=gd -s CONTENT_ID=EP9000-${LAU_TEST_TITLE_ID}_00-VITAHBUUPDATE001 -d LAREA_TYPE=0 -d PARENTAL_LEVEL=1 -s PSP2_DISP_VER=03.600 -d PSP2_SYSTEM_VER=56623104"
)
vita_create_vpk(lau-test-app-${LAU_TEST_APP_VERSION}.vpk ${LAU_TEST_TITLE_ID} lau-test-app-eboot.bin
    VERSION ${LAU_TEST_APP_VERSION}
    NAME "LiveArea Update Test ${LAU_TEST_APP_VERSION}"
    FILE
        ${CMAKE_CURRENT_BINARY_DIR}/test-app/sce_sys/homebrew_update.ini sce_sys/homebrew_update.ini
)

add_executable(lau_shell_probe shell-plugin/main.c)
target_link_libraries(lau_shell_probe taihen_stub SceIofilemgr_stub)
vita_create_self(lau-shell-probe.suprx lau_shell_probe UNSAFE
    CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/shell-plugin/exports.yml
)

add_executable(vita_homebrew_update service-plugin/main.c)
target_compile_options(vita_homebrew_update PRIVATE
    -ffreestanding
    -fno-stack-protector
)
target_link_options(vita_homebrew_update PRIVATE -Wl,-q -nostdlib)
target_link_libraries(vita_homebrew_update
    taihen_stub_weak
    SceIofilemgr_stub_weak
    SceKernelThreadMgr_stub_weak
    SceLibKernel_stub_weak
)
vita_create_self(vita-homebrew-update.suprx vita_homebrew_update UNSAFE
    CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/service-plugin/exports.yml
)
