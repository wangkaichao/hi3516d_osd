#
# 364596530@qq.com 2019/06/08
# cmake file
#

# include top cmakefile.txt
include(../top.cmake)

#set(CMAKE_CXX_STANDARD 11)
# minimum cmake version
cmake_minimum_required(VERSION 3.5.1)
# project name
project(osd C)
#project(osd CXX)

# include all c/cxx files
aux_source_directory(. CSRC)
#aux_source_directory(. CSRCXX)

# target shared/static library
#add_library(osd SHARED ${CSRC} ${CXXSRC})

add_library(osd_static STATIC ${CSRC} ${CXXSRC})
set_target_properties(osd_static PROPERTIES OUTPUT_NAME "osd")

# target execute 
#add_executable(demo ${CSRC})
#add_executable(demo ${CSRCXX})

