# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(CommunicationPrimitives)

# get all *.c files recursively
file(GLOB_RECURSE SRC_FILES_PROTOS ${PROJECT_SOURCE_DIR}/protocols/*.c)
file(GLOB_RECURSE SRC_FILES_DATA_STRUCT ${PROJECT_SOURCE_DIR}/data_structures/*.c)
file(GLOB_RECURSE SRC_FILES_APPLICATIONS ${PROJECT_SOURCE_DIR}/applications/*.c)
file(GLOB_RECURSE SRC_FILES_UTILITY ${PROJECT_SOURCE_DIR}/utility/*.c)

# include headers
include_directories(.)

include_directories(../Yggdrasil/Yggdrasil-LowLevelLib/)
include_directories(../Yggdrasil/Yggdrasil-LowLevelLib/src)

include_directories(../Yggdrasil/Yggdrasil/)
include_directories(../Yggdrasil/Yggdrasil/data_structures/generic/)

include_directories(/usr/include)
include_directories(/usr/include/uuid)
include_directories(/usr/include/libnl3)

#set(CMAKE_BUILD_TYPE RelWithDebInfo)
#set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O0 -lm -pthread -lnl-genl-3 -lnl-3 -luuid -lrt")
set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -lm -lnl-genl-3 -lnl-3 -luuid -lrt")

#exclude files
#list(REMOVE_ITEM SRC_FILES_PROTOS ${PROJECT_SOURCE_DIR}/protocols/incomplete/under_development.c)

#add libraries
add_library(protos OBJECT ${SRC_FILES_PROTOS})
add_library(datastruct OBJECT ${SRC_FILES_DATA_STRUCT})
add_library(utility OBJECT ${SRC_FILES_UTILITY})

add_executable(topology_discovery_test $<TARGET_OBJECTS:protos> $<TARGET_OBJECTS:utility> $<TARGET_OBJECTS:datastruct> $<TARGET_OBJECTS:yggprotos> $<TARGET_OBJECTS:ygginterfaces> $<TARGET_OBJECTS:yggdatastruct> $<TARGET_OBJECTS:yggcore> $<TARGET_OBJECTS:ygglllib_obj> applications/topology_discovery_test.c)

add_executable(hash_table_test $<TARGET_OBJECTS:protos> $<TARGET_OBJECTS:utility> $<TARGET_OBJECTS:datastruct> $<TARGET_OBJECTS:yggprotos> $<TARGET_OBJECTS:ygginterfaces> $<TARGET_OBJECTS:yggdatastruct> $<TARGET_OBJECTS:yggcore> $<TARGET_OBJECTS:ygglllib_obj> applications/hash_table_test.c)

add_executable(bcast_framework_test $<TARGET_OBJECTS:protos> $<TARGET_OBJECTS:utility> $<TARGET_OBJECTS:datastruct> $<TARGET_OBJECTS:yggprotos> $<TARGET_OBJECTS:ygginterfaces> $<TARGET_OBJECTS:yggdatastruct> $<TARGET_OBJECTS:yggcore> $<TARGET_OBJECTS:ygglllib_obj> applications/bcast_framework_test.c)

add_executable(graph_test $<TARGET_OBJECTS:protos> $<TARGET_OBJECTS:utility> $<TARGET_OBJECTS:datastruct> $<TARGET_OBJECTS:yggprotos> $<TARGET_OBJECTS:ygginterfaces> $<TARGET_OBJECTS:yggdatastruct> $<TARGET_OBJECTS:yggcore> $<TARGET_OBJECTS:ygglllib_obj> applications/graph_test.c)


