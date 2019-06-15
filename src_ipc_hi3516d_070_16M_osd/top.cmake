#
# wangkaichao2@163.com 2018-09-16
# top CMakeList.txt
#

# set top directory path
#exec_program("dirname `pwd`" OUTPUT_VARIABLE TOP)
set(TOP "/mnt/work2/hi3516d_osd/src_ipc_hi3516d_070_16M_osd")

set(CMAKE_SYSTEM_NAME Linux)
# specify the cross compiler
set(CMAKE_C_COMPILER "/opt/hisi-linux/x86-arm/arm-hisiv300-linux/target/bin/arm-hisiv300-linux-gcc")
set(CMAKE_CXX_COMPILER "/opt/hisi-linux/x86-arm/arm-hisiv300-linux/target/bin/arm-hisiv300-linux-g++")
#set(CMAKE_C_COMPILER "gcc")
#set(CMAKE_CXX_COMPILER "g++")
# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH ${TOP} /opt/hisi-linux/x86-arm/arm-hisiv300-linux)

# search for programs in the build host directories (not necessary NEVER ONLY BOTH)
#SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#set head file path
include_directories(
    ${TOP}/hisi_070/include
    ${TOP}/osd
)

#set library path
link_directories(
  ${TOP}/hisi_070/lib
  ${TOP}/lib
)

# set compile flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99") 

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# add 3rd libraries here
#link_libraries("-Wl,-Bstatic")
#link_libraries("-Wl,--start-group")
#link_libraries("-lcommon")
#link_libraries("-Wl,--end-group")
#link_libraries("-Wl,-Bdynamic")

# system library
link_libraries("-lpthread -lrt -ldl -lm -lc -lstdc++")
link_libraries("-lmpi")

set(LIBRARY_OUTPUT_PATH ${TOP}/lib)
set(EXECUTABLE_OUTPUT_PATH ${TOP}/bin)
