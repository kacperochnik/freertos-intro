#include "log.h"

TimerHandle_t log_timer;
float log_time;

void LOG_init()
{
    if (NULL == (log_timer = xTimerCreate("timer", pdMS_TO_TICKS(1), pdTRUE, 0, vTimerCallback)))
    {
        LOG(ERR, "Couldn't create a timer.");
    }
    xTimerStart(log_timer, 0);
}

void vTimerCallback(TimerHandle_t xTimer)
{
    log_time += (0.001f);
}

void LOG(log_t type, const char *format, ...)
{
    char t[10];
    switch (type)
    {
    case DBG:
        strcpy(t, "[1;32mDBG");
        break;
    case WRN:
        strcpy(t, "[1;33mWRN");
        break;
    case INF:
        strcpy(t, "[0;37mINF");
        break;
    case ERR:
        strcpy(t, "[1;31mERR");
        break;
    default:
        strcpy(t, "[1;37mLOG");
        break;
    }
    printf("\033%s - %f - ", t, log_time);
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    printf("\033[0m\n");
}