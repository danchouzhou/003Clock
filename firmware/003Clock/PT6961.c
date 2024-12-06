#include "numicro_8051.h"
#include "PT6961.h"

__code extern uint8_t u8segments[18] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67, 0x77, 0x7c, 0x58, 0x5e, 0x79, 0x71, 0x80, 0x00
};

inline void pt6961_writeBit(uint8_t u8data)
{
    PT6961_DIN = u8data & 0x01;
    PT6961_CLK = 0;
    PT6961_CLK = 1;
}

inline void pt6961_writeByte(uint8_t u8data)
{
#if PT6961_HWSPI==1
    SPDR = u8data;
    while (!(SPSR & 0x80)); // Wait until transfer is complete
    SPSR &= 0x7F; // Clear SPIF
#else
    for(uint8_t i=0; i<8; i++)
        pt6961_writeBit(u8data >> i);
#endif
}

void pt6961_writeCommand(uint8_t u8cmd)
{
    PT6961_STB = 1; // Initialize serial communication
    PT6961_STB = 0;
    pt6961_writeByte(u8cmd);
}

void pt6961_clear(void)
{
	pt6961_writeCommand(0xC0);
	pt6961_writeByte(0);
	pt6961_writeCommand(0xC2);
	pt6961_writeByte(0);
	pt6961_writeCommand(0xC4);
	pt6961_writeByte(0);
	pt6961_writeCommand(0xC6);
	pt6961_writeByte(0);
}

void pt6961_setBrightness(uint8_t u8brightness)
{
    if (u8brightness > 0)
        pt6961_writeCommand(0x88 | (u8brightness - 1 & 0x07));
    else
        pt6961_writeCommand(0x80);
}

void pt6961_init(void)
{
    P00_PUSHPULL_MODE;
    P10_PUSHPULL_MODE;
    P11_PUSHPULL_MODE;

#if PT6961_HWSPI==1
    SPSR = 0x08; // DISMODF = 1
    SPCR = 0x7F; // SPIEN, LSBFE, MSTR, CPOL, CPHA = 1; SPI clock rate divide by 16
#endif
    pt6961_clear();
    pt6961_setBrightness(4);
}

void pt6961_setNumber(int number, uint8_t u8colonOn)
{
    pt6961_writeCommand(0xC0);
    pt6961_writeByte(u8segments[number / 1000] | (u8colonOn << 7));
    pt6961_writeCommand(0xC2);
    pt6961_writeByte(u8segments[number / 100 % 10] | (u8colonOn << 7));
    pt6961_writeCommand(0xC4);
    pt6961_writeByte(u8segments[number / 10 % 10]);
    pt6961_writeCommand(0xC6);
    pt6961_writeByte(u8segments[number % 10]);
}

void pt6961_setNumberFade(int number, uint8_t u8colonOn)
{
    static int lastNumber = 0;
    static uint8_t lastColon = 0;

    for(uint8_t i=0; i<20; i++)
    {
        for(uint8_t j=0; j<20; j++)
        {
            if (i>=j)
            {
                pt6961_writeCommand(0xC0);
                pt6961_writeByte(u8segments[number / 1000] | (u8colonOn << 7));
                pt6961_writeCommand(0xC2);
                pt6961_writeByte(u8segments[number / 100 % 10] | (u8colonOn << 7));
                pt6961_writeCommand(0xC4);
                pt6961_writeByte(u8segments[number / 10 % 10]);
                pt6961_writeCommand(0xC6);
                pt6961_writeByte(u8segments[number % 10]);
            }
            else
            {
                pt6961_writeCommand(0xC0);
                pt6961_writeByte(u8segments[lastNumber / 1000] | (lastColon << 7));
                pt6961_writeCommand(0xC2);
                pt6961_writeByte(u8segments[lastNumber / 100 % 10] | (lastColon << 7));
                pt6961_writeCommand(0xC4);
                pt6961_writeByte(u8segments[lastNumber / 10 % 10]);
                pt6961_writeCommand(0xC6);
                pt6961_writeByte(u8segments[lastNumber % 10]);
            }
        }
    }

    lastNumber = number;
    lastColon = u8colonOn;
}
