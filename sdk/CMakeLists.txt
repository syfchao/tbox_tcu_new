cmake_minimum_required(VERSION 3.15)
project(sdk C)

set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_SYSTEM_NAME Linux)

set(MYSYSROOT /home/renhono/CLionProjects/sim_open_sdk/sim_crosscompile/sysroots)
set(DIR_MDM9607 ${MYSYSROOT}/mdm9607-perf)

SET(CMAKE_C_COMPILER ${MYSYSROOT}/x86_64-linux/usr/bin/arm-oe-linux-gnueabi/arm-oe-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER ${MYSYSROOT}/x86_64-linux/usr/bin/arm-oe-linux-gnueabi/arm-oe-linux-gnueabi-g++)

set(AFLAGS "-march=armv7-a -mfloat-abi=softfp -mfpu=neon ")
set(BFLAGS "-O2 -fpermissive -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -DATCA_HAL_I2C -DATCAPRINTF -DUSE_GLIB -DBUILD_VERSION_WIFI_RTL -fdiagnostics-color=auto ")
set(CFLAGS " ")
string(CONCAT ALL_FLAGS ${AFLAGS} ${BFLAGS} ${CFLAGS})


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ALL_FLAGS} --sysroot=${DIR_MDM9607}" CACHE INTERNAL "" FORCE)
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} ${ALL_FLAGS} --sysroot=${DIR_MDM9607}" CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ALL_FLAGS} --sysroot=${DIR_MDM9607}" CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${ALL_FLAGS} --sysroot=${DIR_MDM9607}" CACHE INTERNAL "" FORCE)

#设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

include_directories(${DIR_MDM9607}/usr/include)
include_directories(${DIR_MDM9607}/usr/include/glib-2.0)
include_directories(${DIR_MDM9607}/usr/lib/glib-2.0/include)
include_directories(${DIR_MDM9607}/usr/include/c++/4.9.2)
include_directories(${DIR_MDM9607}/usr/include/c++/4.9.2/arm-oe-linux-gnueabi)


link_directories(${DIR_MDM9607}/usr/lib/)
link_directories(${DIR_MDM9607}/lib/)



#递归添加头文件函数
function(include_sub_directories_recursively root_dir)
    if (IS_DIRECTORY ${root_dir})               # 当前路径是一个目录吗，是的话就加入到包含目录
        message("include dir: " ${root_dir})
        include_directories(${root_dir})
    endif ()

    file(GLOB ALL_SUB RELATIVE ${root_dir} ${root_dir}/*) # 获得当前目录下的所有文件，加入ALL_SUB列表中
    foreach (sub ${ALL_SUB})
        if (IS_DIRECTORY ${root_dir}/${sub})
            include_sub_directories_recursively(${root_dir}/${sub}) # 对子目录递归调用，包含
        endif ()
    endforeach ()
endfunction()

#添加头文件
include_sub_directories_recursively(${CMAKE_SOURCE_DIR}/../inc) # 对子目录递归调用，包含
include_sub_directories_recursively(${CMAKE_SOURCE_DIR}/../sdk-includes) # 对子目录递归调用，包含
include_sub_directories_recursively(${CMAKE_SOURCE_DIR}/../sdk_inc) # 对子目录递归调用，包含
#include_sub_directories_recursively(${CMAKE_SOURCE_DIR}/include) # 对子目录递归调用，包含
MESSAGE(STATUS "CMAKE_SOURCE_DIR" ${CMAKE_SOURCE_DIR})

#include_directories(../sdk_inc)
#include_directories(../inc)
#include_directories(../sdk-includes)
#include_directories(${CMAKE_SOURCE_DIR}/include/rpc)


link_directories(${DIR_MDM9607}/usr/lib/)
link_directories(${DIR_MDM9607}/lib/)
link_directories(${CMAKE_SOURCE_DIR}/libs)

add_library(sdk
        ALSAControl.c
        ATControl.c
        DataCall.c
        DEVControl.c
        DMSControl.c
        DnsResolv.c
        NASControl.c
        sdkVersion.c
        SMSControl.c
        TTSControl.c
        UIMControl.c
        VoiceCall.c
        WDSControl.c
        WiFiControl.c)
