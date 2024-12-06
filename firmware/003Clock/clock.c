#include "clock.h"

void clock(STR_TIME_T *pTimeStr, char isStopwatch)
{
    if (pTimeStr->second >= 59)
    {
        pTimeStr->second = 0;
        if ((pTimeStr->minute >= 59 && isStopwatch == 0) || (pTimeStr->minute >= 99 && isStopwatch == 1))
        {
            pTimeStr->minute = 0;
            if (pTimeStr->hour >= 23)
                pTimeStr->hour = 0;
            else
                pTimeStr->hour = pTimeStr->hour + 1;
        }
        else
            pTimeStr->minute = pTimeStr->minute + 1;
    }
    else
        pTimeStr->second = pTimeStr->second + 1;
}

inline int getTime(STR_TIME_T *pTimeStr)
{
    int time_int;

    time_int = (int)pTimeStr->hour * 100 + pTimeStr->minute;

    return time_int;
}
