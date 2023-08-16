cmake_minimum_required(VERSION 3.7.1)

project(FreeRTOS-POSIX)

set(PORTABLE_FILES 
    src/portable/MemMang/heap_3.c 
    src/portable/GCC/POSIX/port.c
    )

set(SOURCE_FILES 
    src/croutine.c
    src/event_groups.c
    src/list.c
    src/queue.c
    src/tasks.c
    src/timers.c)

set(PORTABLE_FILES 
    src/portable/MemMang/heap_3.c 
    src/portable/GCC/POSIX/port.c
    )

file(GLOB APP_FILES
    "app/*.c"
)

include_directories(src/include src/portable/GCC/POSIX Demo app/include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -DDEBUG=1 -g -UUSE_STDIO -D__GCC_POSIX__=1 -DMAX_NUMBER_OF_TASKS=300 -O2")

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_BUILD_TYPE Debug)

find_package(eclipse-paho-mqtt-c REQUIRED)

add_executable(app ${SOURCE_FILES} ${PORTABLE_FILES} ${APP_FILES})

target_link_libraries(app eclipse-paho-mqtt-c::paho-mqtt3c) 
