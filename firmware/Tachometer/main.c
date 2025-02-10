#include "numicro_8051.h"
#include "PT6961.h"

#define SW1 P17

void main(void) 
{
    uint16_t u16counter = 0;
    uint16_t u16cap0 = 0;
    uint16_t u16freq = 0, u16rev4stroke = 0, u16rev2stroke = 0;
    uint16_t u16swCount = 0;
    uint8_t u8mode = 0;

    /* Switch to external clock source */
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

    P17_QUASI_MODE;
    P12_QUASI_MODE;

    pt6961_init();

    CKCON |= 0x08; // Timer 0 source from Fsys directly
    TH0 = (uint8_t)(49536 >> 8); // 65536 - 16000
    TL0 = (uint8_t)(49536 & 0xFF);
    TMOD |= 0x01; // Timer 0 mode 1
    TCON |= 0x10; // Timer 0 run

    T2MOD = 0x48; // Timer 2 capture auto-clear, pre-scalar = 1/64
    CAPCON0 |= 0x10; // Input Capture 0 Enable
    // CAPCON1 &= 0xFC; // Input Capture 0 Level Select = Falling edge
    // CAPCON3 &= 0xF0; // Input Capture Channel 0 Input Pin = P1.2/IC0
    T2CON = 0x04; // Timer 2 run

    while(1)
    {
        if (SW1) {
            if (u16swCount > 20)
                u8mode++;
            u16swCount = 0;
        }
        else
            u16swCount++;

        if (CAPCON0 & 0x01) // Check capture 0 flag
        {
            CAPCON0 &= 0xFE; // Clear capture 0 flag
            u16cap0 = (uint16_t)C0H << 8 | C0L; // Get 16-bit capture data
            if (u16cap0 != 0)
            {
                u16freq = 250000 / u16cap0; // 16 MHz / 64 = 250000, the minimum capture frequency is 250000 Hz / 65536 = 3.81 Hz
                u16rev4stroke = 30000000 / u16cap0; // frequency * 60 * 2 = 4-stroke rev
                u16rev2stroke = 15000000 / u16cap0; // frequency * 60     = 2-stroke rev
            }

            if (u16counter > 100) // Minimum update interval is 100 ms
            {
                switch (u8mode) {
                case 0:
                    if (u16rev4stroke < 10000) { // Check the data within the display range
                        pt6961_setNumber(u16rev4stroke, 0);
                        u16counter = 0;
                    }
                    break;

                case 1:
                    if (u16rev2stroke < 10000) {
                        pt6961_setNumber(u16rev2stroke, 0);
                        u16counter = 0;
                    }
                    break;

                case 2:
                    if (u16freq < 10000) {
                        pt6961_setNumber(u16freq, 0);
                        u16counter = 0;
                    }
                    break;

                default:
                    u8mode = 0;
                }
            }
        }

        if (u16counter > 2000) // Clear display if no valid data for 2 seconds
        {
            pt6961_writeCommand(0xC0);
            pt6961_writeByte(0x40); // 'g' segment
            pt6961_writeCommand(0xC2);
            pt6961_writeByte(0x40);
            pt6961_writeCommand(0xC4);
            pt6961_writeByte(0x40);
            pt6961_writeCommand(0xC6);
            pt6961_writeByte(0x40);
        }
        else
            u16counter++;
        
        /* 1 ms delay */
        while (!(TCON & 0x20)); // Wait until timer 0 overflow
        TH0 = (uint8_t)(49536 >> 8); // 65536 - 16000
        TL0 = (uint8_t)(49536 & 0xFF);
        TCON &= 0xDF; // Clear TF0
    }
}