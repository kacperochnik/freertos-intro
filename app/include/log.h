#ifndef APP_INCLUDE_LOG_H
#define APP_INCLUDE_LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <FreeRTOS.h>
#include <timers.h>

typedef enum {
    DBG,
    INF,
    WRN,
    ERR,
} log_t;

void LOG_init();
void LOG(log_t type, const char *c, ...);


void vTimerCallback( TimerHandle_t xTimer );

#endif