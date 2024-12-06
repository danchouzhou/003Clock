#include "numicro_8051.h"
#include "PT6961.h"
#include "clock.h"

#define SW1 P17

STR_TIME_T time;
STR_TIME_T stopwatch;
void (*isrFunction)(void);

void Timer3_ISR(void) __interrupt (16)
{
    static uint8_t u8counter;

    if ((u8counter & 0x01) == 0)
    {
        clock(&time, 0);
        clock(&stopwatch, 1);
    }

    u8counter ++;

    (*isrFunction)();

    T3CON &= 0xEF; // Clear TF3
}

void showTime(void)
{
    static uint8_t u8counter;
    static int time_int;

    time_int = getTime(&time);

    if ((u8counter & 0x01) == 0)
    {
        pt6961_setNumberFade(time_int, 0);
    }
    else
    {
        pt6961_setNumberFade(time_int, 1);
    }
    u8counter ++;
}

void showSecond(void)
{
    pt6961_setNumberFade(time.second, 1);
}

void showStopwatch(void)
{
    static uint8_t u8counter;
    static int stopwatch_int;

    stopwatch_int = (int)stopwatch.minute * 100 + stopwatch.second;

    if ((u8counter & 0x01) == 0)
    {
        pt6961_setNumberFade(stopwatch_int, 0);
    }
    else
    {
        pt6961_setNumberFade(stopwatch_int, 1);
    }
    u8counter ++;
}

void showSetup(uint8_t u8blink)
{
    static uint16_t u16counter;
    static int time_int, time_int_last;

    time_int = getTime(&time);

    if (u16counter > 1000 || time_int != time_int_last)
        u16counter = 0;
    else
        u16counter ++;

    if (u16counter < 500)
    {
        pt6961_setNumber(time_int, 1);
    }
    else
    {
        switch (u8blink)
        {
            case 1:
                pt6961_writeCommand(0xC0);
                pt6961_writeByte(0x80); // Light up colon only
                break;
            
            case 2:
                pt6961_writeCommand(0xC2);
                pt6961_writeByte(0x80); // Light up colon only
                break;
            
            case 3:
                pt6961_writeCommand(0xC4);
                pt6961_writeByte(0x00);
                break;
            
            case 4:
                pt6961_writeCommand(0xC6);
                pt6961_writeByte(0x00);
                break;
            
            default:
                break;
        }
    }

    time_int_last = time_int;
}

void main(void) 
{
    uint16_t u16swCount = 0;
    uint8_t u8mode = 0;

    TA = 0xAA;
    TA = 0x55;
    CKEN |= 0xC0; // EXTEN
    while(!(CKSWT & 0x08)); // ECLKST
    TA = 0xAA;
    TA = 0x55;
    CKSWT |= 0x02; // Switch to external clock source
    while(CKEN & 0x01); // CKSWTF
    TA = 0xAA;
    TA = 0x55;
    CKEN &= 0xDF; // Disable HIRC

    P12_PUSHPULL_MODE;
    P17_QUASI_MODE;

    pt6961_init();
    pt6961_setBrightness(4);

    CKCON |= 0x08; // Timer 0 source from Fsys directly
    TH0 = (uint8_t)(49536 >> 8); // 65536 - 16000
    TL0 = (uint8_t)(49536 & 0xFF);
    TMOD |= 0x01; // Timer 0 mode 1
    TCON |= 0x10; // Timer 0 run

    isrFunction = &showTime;

    RH3 = (uint8_t)(3036 >> 8); // 65536 - 62500
    RL3 = (uint8_t)(3036 & 0xFF);
    T3CON = 0x0F; // Timer 3 run, pre-scalar = 1/128
    EIE1 |= 0x02; // Enable timer 3 interrupt
    EA = 1;

    while(1)
    {
        if (!SW1) {
            u16swCount++;
            if (u16swCount == 2000)
                u8mode++;
        }

        switch (u8mode) {
            case 0:
                isrFunction = &showTime;
                if (SW1) {
                    if (u16swCount > 20 && u16swCount < 2000)
                        u8mode = 6;
                    u16swCount = 0;
                }
                break;
            
            case 1:
                EIE1 &= 0xFD; // Disable timer 3 interrupt
                showSetup(u8mode);
                if (SW1) {
                    if (u16swCount > 20 && u16swCount < 2000) {
                        if (time.hour < 14)
                            time.hour += 10;
                        else if (time.hour < 20)
                            time.hour = 20;
                        else
                            time.hour -= 20;
                    }
                    u16swCount = 0;
                }
                break;

            case 2:
                showSetup(u8mode);
                if (SW1) {
                    if (u16swCount > 20 && u16swCount < 2000) {
                        if ((time.hour % 10) < 9 && time.hour < 23)
                            time.hour += 1;
                        else if (time.hour >= 23)
                            time.hour -= 3;
                        else
                            time.hour -= 9;
                    }
                    u16swCount = 0;
                }
                break;

            case 3:
                showSetup(u8mode);
                if (SW1) {
                    if (u16swCount > 20 && u16swCount < 2000) {
                        if (time.minute < 50)
                            time.minute += 10;
                        else
                            time.minute -= 50;
                    }
                    u16swCount = 0;
                }
                break;

            case 4:
                showSetup(u8mode);
                if (SW1) {
                    if (u16swCount > 20 && u16swCount < 2000) {
                        if ((time.minute % 10) < 9)
                            time.minute += 1;
                        else
                            time.minute -= 9;
                    }
                    u16swCount = 0;
                }
                break;

            case 5:
                time.second = 0;
                EIE1 |= 0x02; // Enable timer 3 interrupt
                u8mode = 0;
                break;

            case 6:
                isrFunction = &showSecond;
                if (SW1) {
                    if (u16swCount > 20 && u16swCount < 2000)
                        u8mode = 8;
                    u16swCount = 0;
                }
                break;

            /* case 7: u8mode = 0; break; */

            case 8:
                isrFunction = &showStopwatch;
                if (SW1) {
                    if (u16swCount > 20 && u16swCount < 2000)
                        u8mode = 0;
                    u16swCount = 0;
                }
                break;
            
            case 9:
                stopwatch.minute = 0; // Reset stopwatch then return to mode 8
                stopwatch.second = 0;
                u8mode = 8;
                break;
            
            default:
                u8mode = 0;
        }

        P12 = !P12;

        /* 1 ms delay */
        while (!(TCON & 0x20)); // Wait until timer 0 overflow
        TH0 = (uint8_t)(49536 >> 8); // 65536 - 16000
        TL0 = (uint8_t)(49536 & 0xFF);
        TCON &= 0xDF; // Clear TF0
    }
}
