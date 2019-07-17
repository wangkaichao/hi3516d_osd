#
# wangkaichao2@163.com 2018-09-16
# top CMakeList.txt
#

# minimum cmake version
cmake_minimum_required(VERSION 3.5.1)

# set top directory path
#exec_program("dirname `pwd`" OUTPUT_VARIABLE TOP)
set(TOP "/opt/source/work/hi3516d_osd/src_ipc_hi3516d_070_16M_osd")

set(CMAKE_SYSTEM_NAME Linux)
# specify the cross compiler
set(CMAKE_C_COMPILER "/opt/hisi-linux/x86-arm/arm-hisiv300-linux/target/bin/arm-hisiv300-linux-gcc")
set(CMAKE_CXX_COMPILER "/opt/hisi-linux/x86-arm/arm-hisiv300-linux/target/bin/arm-hisiv300-linux-g++")
#set(CMAKE_C_COMPILER "gcc")
#set(CMAKE_CXX_COMPILER "g++")
# where is the target environment 
seT(CMAKE_FIND_ROOT_PATH ${TOP} /opt/hisi-linux/x86-arm/arm-hisiv300-linux)

# search for programs in the build host directories (not necessary NEVER ONLY BOTH)
#sET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
seT(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
seT(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#set head file path
include_directories(${TOP}/hisi_070/include)
include_directories(${TOP}/osd)

#set library path
link_directories(${TOP}/hisi_070/lib)
link_directories(${TOP}/lib)

# set compile flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -ffunction-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-unaligned-access -fno-aggressive-loop-optimizations") 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DARCH=arm -DHIARCH=hi3516a -DHICHIP=0x3516A100 ") 

link_libraries("-Wl,-Bdynamic")
link_libraries("-lpthread -lrt -ldl -lm -lc -lstdc++")

set(LIBRARY_OUTPUT_PATH ${TOP}/lib)
set(EXECUTABLE_OUTPUT_PATH ${TOP}/bin)

#option(USE_SHARED "SHARED" ON)

message("========================================")
if(USE_SHARED)
    message("output shared library")
else()
    message("output static library")
endif()
message("========================================")


########################libosd.so/libosd.a#######################################
# add src from path "osd/**.c"
aux_source_directory(${TOP}/osd OSD_SRC)

if(USE_SHARED)
    add_library(osd_share SHARED ${OSD_SRC})
    set_target_properties(osd_share PROPERTIES OUTPUT_NAME "osd")
    set_target_properties(osd_share PROPERTIES CLEAN_DIRECT_OUTPUT 1)
    target_link_libraries(osd_share mpi VoiceEngine upvqe dnvqe)
else()
    add_library(osd_static STATIC ${OSD_SRC})
    set_target_properties(osd_static PROPERTIES OUTPUT_NAME "osd")
    set_target_properties(osd_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
    target_link_libraries(osd_static mpi VoiceEngine upvqe dnvqe)
endif()


######################### demo ######################################
# add src from path "demo/**.c"
aux_source_directory(${TOP}/demo DEMO_SRC)
# target execute 
add_executable(demo_exe ${DEMO_SRC})

if(USE_SHARED)
    add_dependencies(demo_exe osd_share)
else()
    add_dependencies(demo_exe osd_static)
endif()

set_target_properties(demo_exe PROPERTIES OUTPUT_NAME "demo")
set_target_properties(demo_exe PROPERTIES CLEAN_DIRECT_OUTPUT 1)
target_link_libraries(demo_exe osd mpi VoiceEngine upvqe dnvqe)
