#ifndef PT6961_H
#define PT6961_H

#define PT6961_HWSPI    1

#define PT6961_STB      P11
#define PT6961_DIN      P00
#define PT6961_CLK      P10

__code extern uint8_t u8segments[18];

inline void pt6961_writeBit(uint8_t u8data);
inline void pt6961_writeByte(uint8_t u8data);
void pt6961_writeCommand(uint8_t u8cmd);
void pt6961_clear(void);
void pt6961_setBrightness(uint8_t u8brightness);
void pt6961_init(void);
void pt6961_setNumber(int number, uint8_t u8colonOn);
void pt6961_setNumberFade(int number, uint8_t u8colonOn);

#endif