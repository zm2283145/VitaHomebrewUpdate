cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE
            "$ENV{VITASDK}/share/vita.toolchain.cmake"
            CACHE PATH "VitaSDK toolchain")
    else()
        message(FATAL_ERROR "Set VITASDK before configuring")
    endif()
endif()

project(VitaHomebrewUpdate VERSION 1.0.1 LANGUAGES C ASM)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_OBJECT_PATH_MAX 128)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

find_package(Git QUIET)
set(VHBU_GIT_REVISION "unknown")
set(VHBU_GIT_STATE "unknown")
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE VHBU_GIT_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE VHBU_GIT_REVISION_RESULT
    )
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" status --porcelain
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE VHBU_GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE VHBU_GIT_STATUS_RESULT
    )
    if(VHBU_GIT_REVISION_RESULT EQUAL 0 AND
       VHBU_GIT_STATUS_RESULT EQUAL 0)
        if(VHBU_GIT_STATUS STREQUAL "")
            set(VHBU_GIT_STATE "clean")
        else()
            set(VHBU_GIT_STATE "dirty")
        endif()
    endif()
endif()
file(WRITE "${CMAKE_BINARY_DIR}/vhbu-build-revision.txt"
    "${VHBU_GIT_REVISION}\n${VHBU_GIT_STATE}\n")

option(VHBU_ENABLE_CUSTOM_DIALOG
    "Enable the hardware-validated LiveArea update confirmation path"
    ON)

add_library(HomebrewUpdateClient STATIC
    client/src/homebrew_update_client.c
)
target_include_directories(HomebrewUpdateClient
    PUBLIC client/include
    PRIVATE common
)
target_compile_options(HomebrewUpdateClient PRIVATE
    -Wall
    -Wextra
    -Werror
    -O2
)
target_link_libraries(HomebrewUpdateClient PUBLIC
    SceAppMgr_stub
    SceIofilemgr_stub
    SceNet_stub
)
set_target_properties(HomebrewUpdateClient PROPERTIES
    OUTPUT_NAME HomebrewUpdateClient
)

add_library(vhbu_bgdl STATIC common/bgdl.c)
target_include_directories(vhbu_bgdl PUBLIC common)
target_compile_options(vhbu_bgdl PRIVATE
    -Wall
    -Wextra
    -Werror
    -O2
)
target_link_libraries(vhbu_bgdl PUBLIC
    taihen_stub
    SceKernelModulemgr_stub
)

add_library(vhbu_paf_stubs STATIC service-plugin/paf_stubs.S)

file(GLOB_RECURSE VHBU_BEARSSL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/BearSSL-0.6/src/*.c")
add_library(vhbu_bearssl STATIC ${VHBU_BEARSSL_SOURCES})
target_include_directories(vhbu_bearssl
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/BearSSL-0.6/inc"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/BearSSL-0.6/src"
)
target_compile_definitions(vhbu_bearssl PRIVATE
    BR_USE_URANDOM=0
    BR_USE_WIN32_RAND=0
    BR_USE_UNIX_TIME=0
    BR_USE_WIN32_TIME=0
    BR_AES_X86NI=0
    BR_SSE2=0
    BR_POWER8=0
    BR_INT128=0
)
target_compile_options(vhbu_bearssl PRIVATE -O2)

add_executable(vita_homebrew_update
    service-plugin/main.c
    common/changeinfo.c
    common/bearssl_support.c
    common/libc_compat.c
    common/file_verify.c
    common/sha1.c
    common/fs_tree.c
    common/notification_db.c
    common/update_config.c
    common/vpk_extract.c
)
target_include_directories(vita_homebrew_update PRIVATE common)
target_compile_options(vita_homebrew_update PRIVATE
    -Wall
    -Wextra
    -Werror
    -O2
    -ffreestanding
    -fno-stack-protector
)
if(VHBU_ENABLE_CUSTOM_DIALOG)
    target_compile_definitions(vita_homebrew_update PRIVATE
        VHBU_ENABLE_CUSTOM_DIALOG=1)
endif()
target_link_options(vita_homebrew_update PRIVATE -Wl,-q -nostdlib)
target_link_libraries(vita_homebrew_update
    vhbu_bgdl
    vhbu_paf_stubs
    vhbu_bearssl
    taihen_stub_weak
    SceAppMgr_stub_weak
    SceIofilemgr_stub_weak
    SceKernelModulemgr_stub_weak
    SceKernelThreadMgr_stub_weak
    SceLibKernel_stub_weak
    SceNotificationUtil_stub_weak
    ScePromoterUtil_stub_weak
    SceRtc_stub_weak
    SceSqlite_stub_weak
    SceSysmem_stub_weak
    SceSysmodule_stub_weak
    z
    gcc
)
set(VITA_ELF_CREATE_FLAGS "")
vita_create_self(vita-homebrew-update.suprx vita_homebrew_update UNSAFE
    CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/service-plugin/exports.yml"
)

install(TARGETS HomebrewUpdateClient ARCHIVE DESTINATION lib)
install(FILES client/include/homebrew_update_client.h DESTINATION include)
