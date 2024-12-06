#ifndef CLOCK_H
#define CLOCK_H

typedef struct {
    char second;
    char minute;
    char hour;
} STR_TIME_T;

void clock(STR_TIME_T *pTimeStr, char isStopwatch);
inline int getTime(STR_TIME_T *pTimeStr);

#endif