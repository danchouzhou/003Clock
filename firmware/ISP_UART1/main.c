/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* SPDX-License-Identifier: Apache-2.0                                                                     */
/* Copyright(c) 2023 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include "numicro_8051.h"
#include "isp_uart1.h"

/******************************
 * @brief     Timer0 interrupt.
 * @param[in] None.
 * @return    None.
 ******************************/
void Timer0_ISR(void) __interrupt(1)
{
    if (g_timer0Counter)
    {
        g_timer0Counter--;
        if (!g_timer0Counter)
            g_timer0Over = 1;
    }

    if (g_timer1Counter)
    {
        g_timer1Counter--;
        if (!g_timer1Counter)
            g_timer1Over = 1;
    }
}

/******************************
 * @brief     Main loop.
 * @param[in] None.
 * @return    None.
 ******************************/
void main(void)
{
    uint8_t vo8temp;
    uint16_t vo16temp;

    set_CHPCON_IAPEN;
    MODIFY_HIRC(HIRC_166);
    /* Initialize UART1 115200 bps for NuMicro ISP Programming Tool */
    UART1_ini_115200_166MHz();
    TM0_ini();

    g_timer0Over = 0;
    g_timer0Counter = 100;
    g_progarmflag = 0;

    while (1)
    {
        /* UART1 handler */
        if (RI_1 == 1)
        {
            vo8temp = SBUF_1;
            uart_rcvbuf[bufhead++] = vo8temp;
            RI_1 = 0; // Clear RI (Receive Interrupt).
        }
        if (TI_1 == 1)
        {
            TI_1 = 0; // Clear TI (Transmit Interrupt).
        }
        if (bufhead == 1)
        {
            g_timer1Over = 0;
            g_timer1Counter = 90; // Reload UART1 timeout interval
        }
        if (bufhead == 64)
        {
            bUartDataReady = TRUE;
            g_timer1Counter = 0;
            g_timer1Over = 0;
            bufhead = 0;
        }

        /* ISP handler */
        if (bUartDataReady == TRUE)
        {
            EA = 0; // Disable all interrupt
            if (g_progarmflag == 1)
            {
                for (count = 8; count < 64; count++)
                {
                    IAPCN = BYTE_PROGRAM_AP; // Byte programming
                    IAPAL = flash_address & 0xff;
                    IAPAH = (flash_address >> 8) & 0xff;
                    IAPFD = uart_rcvbuf[count];
                    trig_IAPGO;

                    IAPCN = BYTE_READ_AP; // Byte read
                    trig_IAPGO;
                    vo8temp = uart_rcvbuf[count];
                    if (IAPFD != vo8temp)
                        while (1);
                    if (CHPCON == 0x43) // Held ISP progress if there is an error
                        while (1);

                    g_totalchecksum += vo8temp;
                    flash_address++;
                    vo16temp = AP_size;
                    if (flash_address == vo16temp)
                    {
                        g_progarmflag = 0;
                        g_timer0Over = 1;
                        goto END_2;
                    }
                }
            END_2:
                Package_checksum();
                uart_txbuf[8] = g_totalchecksum & 0xff;
                uart_txbuf[9] = (g_totalchecksum >> 8) & 0xff;
                Send_64byte_To_UART1();
            }

            switch (uart_rcvbuf[0])
            {
                case CMD_CONNECT:
                case CMD_SYNC_PACKNO:
                    Package_checksum();
                    Send_64byte_To_UART1();
                    g_timer0Counter = 0; // Stop timeout counter
                    g_timer0Over = 0;
                    break;

                case CMD_GET_FWVER:
                    Package_checksum();
                    uart_txbuf[8] = FW_VERSION;
                    Send_64byte_To_UART1();
                    break;

                case CMD_RUN_APROM:
                    goto _APROM;
                    break;

                // Request from NuMicro ISP Programming Tool, get device ID by using following rule
                case CMD_GET_DEVICEID:
                    READ_ID();
                    Package_checksum();
                    uart_txbuf[8] = DID_lowB;
                    uart_txbuf[9] = DID_highB;
                    uart_txbuf[10] = PID_lowB;
                    uart_txbuf[11] = PID_highB;
                    Send_64byte_To_UART1();
                    break;

                case CMD_ERASE_ALL:
                    set_IAPUEN_APUEN;
                    IAPFD = 0xFF; // Flash data must be 0xFF for erase command
                    IAPCN = PAGE_ERASE_AP;

                    for (flash_address = 0x0000; flash_address < APROM_SIZE / PAGE_SIZE; flash_address++)
                    {
                        IAPAL = LOBYTE(flash_address * PAGE_SIZE);
                        IAPAH = HIBYTE(flash_address * PAGE_SIZE);
                        trig_IAPGO;
                    }
                    Package_checksum();
                    Send_64byte_To_UART1();
                    break;

                case CMD_READ_CONFIG:
                    READ_CONFIG();
                    Package_checksum();
                    uart_txbuf[8] = CONF0;
                    uart_txbuf[9] = CONF1;
                    uart_txbuf[10] = CONF2;
                    uart_txbuf[11] = 0xff;
                    uart_txbuf[12] = CONF4;
                    uart_txbuf[13] = 0xff;
                    uart_txbuf[14] = 0xff;
                    uart_txbuf[15] = 0xff;
                    Send_64byte_To_UART1();
                    break;

                case CMD_UPDATE_CONFIG:
                    recv_CONF0 = uart_rcvbuf[8];
                    recv_CONF1 = uart_rcvbuf[9];
                    recv_CONF2 = uart_rcvbuf[10];
                    recv_CONF4 = uart_rcvbuf[12];
                    /* Erase CONFIG */
                    // set_CHPCON_IAPEN;
                    set_IAPUEN_CFUEN;
                    IAPCN = PAGE_ERASE_CONFIG;
                    IAPAL = 0x00;
                    IAPAH = 0x00;
                    IAPFD = 0xFF;
                    trig_IAPGO;
                    /* Program CONFIG */
                    IAPCN = BYTE_PROGRAM_CONFIG;
                    IAPAL = 0x00;
                    IAPAH = 0x00;
                    IAPFD = recv_CONF0;
                    trig_IAPGO;
                    IAPFD = recv_CONF1;
                    IAPAL = 0x01;
                    trig_IAPGO;
                    IAPAL = 0x02;
                    IAPFD = recv_CONF2;
                    trig_IAPGO;
                    IAPAL = 0x04;
                    IAPFD = recv_CONF4;
                    trig_IAPGO;
                    clr_IAPUEN_CFUEN;
                    /* Read new CONFIG */
                    READ_CONFIG();

                    Package_checksum();
                    uart_txbuf[8] = CONF0;
                    uart_txbuf[9] = CONF1;
                    uart_txbuf[10] = CONF2;
                    uart_txbuf[11] = 0xff;
                    uart_txbuf[12] = CONF4;
                    uart_txbuf[13] = 0xff;
                    uart_txbuf[14] = 0xff;
                    uart_txbuf[15] = 0xff;
                    Send_64byte_To_UART1();
                    break;

                case CMD_UPDATE_APROM:
                    // set_CHPCON_IAPEN;
                    set_IAPUEN_APUEN;
                    IAPFD = 0xFF; // Flash data must be 0xFF for erase command
                    IAPCN = PAGE_ERASE_AP;

                    for (flash_address = 0x0000; flash_address < APROM_SIZE / PAGE_SIZE; flash_address++)
                    {
                        IAPAL = LOBYTE(flash_address * PAGE_SIZE);
                        IAPAH = HIBYTE(flash_address * PAGE_SIZE);
                        trig_IAPGO;
                    }

                    g_totalchecksum = 0;
                    flash_address = 0;
                    AP_size = 0;
                    AP_size = uart_rcvbuf[12];
                    vo8temp = uart_rcvbuf[13];
                    AP_size |= (vo8temp << 8);
                    g_progarmflag = 1;

                    for (count = 16; count < 64; count++)
                    {
                        IAPCN = BYTE_PROGRAM_AP;
                        IAPAL = flash_address & 0xff;
                        IAPAH = (flash_address >> 8) & 0xff;
                        IAPFD = uart_rcvbuf[count];
                        clr_CHPCON_IAPFF;
                        trig_IAPGO;

                        IAPCN = BYTE_READ_AP; // Read the programmed data
                        trig_IAPGO;
                        vo8temp = uart_rcvbuf[count];
                        if (IAPFD != vo8temp)
                            while (1)
                                ;
                        if (CHPCON == 0x43) // Held ISP progress if there is an error
                            while (1)
                                ;

                        g_totalchecksum += vo8temp;
                        flash_address++;
                        vo16temp = AP_size;
                        if (flash_address == vo16temp)
                        {
                            g_progarmflag = 0;
                            goto END_1;
                        }
                    }
                END_1:
                    Package_checksum();
                    uart_txbuf[8] = g_totalchecksum & 0xff;
                    uart_txbuf[9] = (g_totalchecksum >> 8) & 0xff;
                    Send_64byte_To_UART1();
                    break;
            }
            bUartDataReady = FALSE;
            bufhead = 0;
            EA = 1;
        }

        // Check connect timer out
        if (g_timer0Over == 1)
            goto _APROM;

        // Check UART1 time out or buffer error
        if (g_timer1Over == 1)
        {
            if ((bufhead < 64) && (bufhead > 0) || (bufhead > 64))
            {
                bufhead = 0;
            }
        }
    }

_APROM:
    MODIFY_HIRC(HIRC_16);
    clr_CHPCON_IAPEN;
    TA = 0xAA;
    TA = 0x55;
    CHPCON = 0x00; // Boot from APROM
    TA = 0xAA;
    TA = 0x55;
    CHPCON = 0x80; // Perform software reset

    /* Trap the CPU */
    while (1);
}
