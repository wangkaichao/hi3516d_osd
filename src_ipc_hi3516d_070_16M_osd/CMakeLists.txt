#
# 364596530@qq.com 2019/06/08
# cmake file
#

# minimum cmake version
cmake_minimum_required(VERSION 3.5.1)

# include top cmakefile.txt
include(top.cmake)

# project name
project(demo)

# include src files
aux_source_directory(demo DEMO_SRC)

# target execute 
add_executable(demo ${DEMO_SRC})
target_link_libraries(demo osd_static)

################################################################

# include src files
aux_source_directory(osd OSD_SRC)

# target shared/static library
#add_library(osd SHARED ${DEMO_SRC})
#SET_TARGET_PROPERTIES(osd PROPERTIES CLEAN_DIRECT_OUTPUT 1)

add_library(osd_static STATIC ${OSD_SRC})
set_target_properties(osd_static PROPERTIES OUTPUT_NAME "osd")
SET_TARGET_PROPERTIES(osd_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

