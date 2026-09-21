#include <stdint.h>
#include <string.h>
#include "inc/tm4c123gh6pm.h"

#define RUNNING_CHAR 0x50
#define PAUSED_CHAR  0x73
#define S_CHAR       0x6D

#define WAIT_COUNT 50
int rate = 0;
int colour = 0;
int isLightsPaused = 0;

// uart command buffer initialization
static char uartBuffer[16];
static uint8_t uartIndex = 0;


uint8_t getHEXNumber(int number)
{
    switch(number)
    {
    case 0:
        return 0x3F;

    case 1:
        return 0x06;

    case 2:
        return 0x5B;

    case 3:
        return 0x4F;

    case 4:
        return 0x66;

    case 5:
        return 0x6D;

    case 6:
        return 0x7D;

    case 7:
        return 0x07;

    case 8:
        return 0x7F;

    default:
        return 0x00;
    }
}


void changeColour(int colourValue){
    switch(colourValue)
            {
            case 0:
                GPIO_PORTF_DATA_R = 0x02;
                break;

            case 1:
                GPIO_PORTF_DATA_R = 0x08;
                break;

            case 2:
                GPIO_PORTF_DATA_R = 0x04;
                break;

            case 3:
                GPIO_PORTF_DATA_R = 0x0E;
                break;

            case 4:
                GPIO_PORTF_DATA_R = 0x02 | 0x04;
                break;

            case 5:
                GPIO_PORTF_DATA_R = 0x02 | 0x08;
                break;

            case 6:
                GPIO_PORTF_DATA_R = 0x04 | 0x08;
                break;

            case 7:
                GPIO_PORTF_DATA_R = 0;
                break;

            default:
                GPIO_PORTF_DATA_R = 0x0E;
                break;
            }

}

void displayDigit(int digit, uint8_t pattern)
{
    GPIO_PORTA_DATA_R &= ~0xF0;

    GPIO_PORTB_DATA_R = pattern;

    if(digit == 1)
    {
        GPIO_PORTA_DATA_R |= 0x10;
    }
    else if(digit == 2)
    {
        GPIO_PORTA_DATA_R |= 0x20;
    }
    else if(digit == 3)
    {
        GPIO_PORTA_DATA_R |= 0x40;
    }
    else if(digit == 4)
    {
        GPIO_PORTA_DATA_R |= 0x80;
    }
}


void delayRefresh(int delay)
{
    for(int j = 0; j < delay; j++)
    {
    }
}


void refresh7SegmentDisplay(int isLightsPausedValue, int colourValue, int rateValue)
{
    displayDigit(4, S_CHAR);
    delayRefresh(500);

    displayDigit(3, getHEXNumber(rateValue + 1));
    delayRefresh(500);

    displayDigit(2, getHEXNumber(colourValue));
    delayRefresh(500);

    displayDigit(1,
                 isLightsPausedValue == 0 ? RUNNING_CHAR : PAUSED_CHAR);
    delayRefresh(500);
}

void UART0_Init(void)
{

    SYSCTL_RCGCUART_R |= 0x01; // we will try to enable the clock for uart console

    delayRefresh(10);

    GPIO_PORTA_AFSEL_R |= 0x03;                 // we will try to have alternate function for Port a instead of the regular gpio
    GPIO_PORTA_PCTL_R   = (GPIO_PORTA_PCTL_R & 0xFFFFFF00) |
                           GPIO_PCTL_PA0_U0RX | GPIO_PCTL_PA1_U0TX;
    GPIO_PORTA_DEN_R   |= 0x03;                 // digital enable PA0, PA1
    GPIO_PORTA_AMSEL_R &= ~0x03;                // no analog on PA0, PA1

    UART0_CTL_R &= ~UART_CTL_UARTEN; // we need to disable the uart while we are configuring as these are critical and we might create issue. requires UARTCTL not be modified while the UART is enabled

    // Baud rate = 115200 with a 16 MHz system clock.
    //    BRD = 16,000,000 / (16 * 115200) = 8.6806
    //    UARTFBRD = 0.6806 * 64 + 0.5 = 44
    UART0_IBRD_R = 8; // only the integer part
    UART0_FBRD_R = 44;

    // 8 data bits, 1 stop bit, no parity, FIFOs disabled.
    UART0_LCRH_R = UART_LCRH_WLEN_8;

    // we will use system clock
    UART0_CC_R = 0x0;

    // enableuart tx and rx
    UART0_CTL_R |= (UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE);
}

void UART0_SendChar(char data)
{
    while (UART0_FR_R & UART_FR_TXFF) { }   // wait while Tx is full and then only send
    UART0_DR_R = data;
}

// Blocking transmit of a null-terminated string.
void UART0_SendString(const char *str)
{
    while (*str)
    {
        UART0_SendChar(*str);
        str++;
    }
}

// Non-blocking check, mirroring UARTCharsAvail from the TivaWare sample,
// so the calling code can poll it once per main-loop pass without ever
// stalling on an empty RX.
int UART0_CharAvail(void)
{
    return ((UART0_FR_R & UART_FR_RXFE) == 0);   // RXFE==0 means a byte is waiting
}

// Blocking read of a single character.
char UART0_GetChar(void)
{
    while (UART0_FR_R & UART_FR_RXFE) { }   // wait for a byte to arrive
    return (char)(UART0_DR_R & 0xFF);
}

void sendStatus(void)
{
    UART0_SendString("Rate: ");
    UART0_SendChar((char)('0' + rate));
    UART0_SendString("  Color: ");
    UART0_SendChar((char)('0' + colour));
    UART0_SendString("  State: ");
    UART0_SendString(isLightsPaused ? "PAUSED" : "RUNNING");
    UART0_SendString("\r\n");
}
void pollUART(void)
{
    if(!UART0_CharAvail())
    {
        return;
    }

    char c = UART0_GetChar();
    UART0_SendChar(c);

    if(c == '\r' || c == '\n')
    {
        uartBuffer[uartIndex] = '\0';

        if(strcmp(uartBuffer, "RATE") == 0)
        {
            rate = (rate + 1) > 7 ? 0 : rate + 1;
            sendStatus();
        }
        else if(strcmp(uartBuffer, "COLOUR") == 0)
        {
            colour = (colour + 1) > 7 ? 0 : colour + 1;
            changeColour(colour);
            sendStatus();
        }
        else if(strcmp(uartBuffer, "PAUSE") == 0)
        {
            if(!isLightsPaused){
            isLightsPaused = !isLightsPaused;
            sendStatus();
            }
            else{
                UART0_SendString("Light is already paused");
            }
        }
        else if(strcmp(uartBuffer, "STATUS") == 0)
        {
            UART0_SendString("status");
            sendStatus();
        }
        else if(strcmp(uartBuffer, "RESUME") == 0)
                {
            if(isLightsPaused){
            isLightsPaused = !isLightsPaused;
                    sendStatus();
            }
            else{
                UART0_SendString("Light is already running");
            }
                }
        else if(uartIndex > 0)
        {
            UART0_SendString("Unknown command\r\n");
        }

        uartIndex = 0;
    }
    else if(uartIndex < (sizeof(uartBuffer) - 1))
    {
        uartBuffer[uartIndex++] = c;
    }
}


int main(void)
{
    int switch1Value = 1;
    int switch2Value = 1;

    int delay;

    int waiting = 0;
    int waitCount = 0;


    SYSCTL_RCGC2_R |= 0x00000023;

    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R |= 0x01;

    GPIO_PORTF_DIR_R = 0x0E;
    GPIO_PORTF_PUR_R = 0x11;
    GPIO_PORTF_DEN_R = 0x1F;

    GPIO_PORTA_DIR_R |= 0xF0;
    GPIO_PORTA_DEN_R |= 0xF0;

    GPIO_PORTB_DIR_R = 0xFF;
    GPIO_PORTB_DEN_R = 0xFF;

    UART0_Init();
    UART0_SendString("LED Blinky, RATE FOR INCREMENTING THE RATE, COLOUR FOR INCREMENTING THE COLOUR, PAUSE FOR PAUSING THE LIGHTS , RUNNING FOR RESUMING, STATUS FOR STATUS");
    sendStatus();


    while(1)
    {
        switch(rate)
        {
        case 0:
            delay = 2000;
            break;

        case 1:
            delay = 1500;
            break;

        case 2:
            delay = 1000;
            break;

        case 3:
            delay = 750;
            break;

        case 4:
            delay = 500;
            break;

        case 5:
            delay = 200;
            break;

        case 6:
            delay = 100;
            break;

        case 7:
            delay = 50;
            break;

        default:
            delay = 250;
            break;
        }
        changeColour(colour);

        if(isLightsPaused == 0)
        {
            for(int i = 0; i < delay; i++)
            {
                pollUART();

                int s1now = GPIO_PORTF_DATA_R & 0x10;
                int s2now = GPIO_PORTF_DATA_R & 0x01;
                if(s1now == 0 &&
                   s2now == 0 &&
                   (switch1Value != 0 || switch2Value != 0))
                {
                    waiting = 0;
                    waitCount = 0;

                    isLightsPaused = 1;
                    sendStatus();
                }
                else if(s1now == 0 &&
                        switch1Value != 0 &&
                        isLightsPaused == 0)
                {
                    waiting = 1;
                    waitCount = 0;
                }
                else if(s2now == 0 &&
                        switch2Value != 0 &&
                        isLightsPaused == 0)
                {
                    waiting = 2;
                    waitCount = 0;
                }
                if(waiting != 0)
                {
                    waitCount++;

                    if(waitCount >= WAIT_COUNT)
                    {
                        if(waiting == 1)
                        {
                            rate++;

                            if(rate > 7)
                            {
                                rate = 0;
                            }
                            sendStatus();
                        }
                        else if(waiting == 2)
                        {
                            colour++;

                            if(colour > 7)
                            {
                                colour = 0;
                            }
                            changeColour(colour);
                            sendStatus();
                        }

                        waiting = 0;
                        waitCount = 0;
                    }
                }
                switch1Value = s1now;
                switch2Value = s2now;
                if(isLightsPaused)
                {
                    break;
                }


                for(int j = 0; j < 1000; j++)
                {
                }


                refresh7SegmentDisplay(isLightsPaused,colour,rate);
            }

            if(isLightsPaused == 0)
            {
                GPIO_PORTF_DATA_R = 0x00;


                for(int i = 0; i < delay; i++)
                {
                    pollUART();

                    int s1now = GPIO_PORTF_DATA_R & 0x10;
                    int s2now = GPIO_PORTF_DATA_R & 0x01;
                    if(s1now == 0 &&
                       s2now == 0 &&
                       (switch1Value != 0 || switch2Value != 0))
                    {
                        waiting = 0;
                        waitCount = 0;

                        isLightsPaused = 1;
                        sendStatus();
                    }
                    else if(s1now == 0 &&
                            switch1Value != 0 &&
                            isLightsPaused == 0)
                    {
                        waiting = 1;
                        waitCount = 0;
                    }
                    else if(s2now == 0 &&
                            switch2Value != 0 &&
                            isLightsPaused == 0)
                    {
                        waiting = 2;
                        waitCount = 0;
                    }
                    if(waiting != 0)
                    {
                        waitCount++;

                        if(waitCount >= WAIT_COUNT)
                        {
                            if(waiting == 1)
                            {
                                rate++;

                                if(rate > 7)
                                {
                                    rate = 0;
                                }
                                sendStatus();
                            }
                            else if(waiting == 2)
                            {
                                colour++;

                                if(colour > 7)
                                {
                                    colour = 0;
                                }
                                changeColour(colour);
                                sendStatus();
                            }

                            waiting = 0;
                            waitCount = 0;
                        }
                    }
                    switch1Value = s1now;
                    switch2Value = s2now;
                    if(isLightsPaused)
                    {
                        break;
                    }


                    for(int j = 0; j < 1000; j++)
                    {
                    }


                    refresh7SegmentDisplay(isLightsPaused,colour,rate);
                }
            }
        }
        else
        {
            pollUART();

            refresh7SegmentDisplay(isLightsPaused,colour,rate);

            int s1now = GPIO_PORTF_DATA_R & 0x10;
            int s2now = GPIO_PORTF_DATA_R & 0x01;
            if(s1now == 0 &&
               s2now == 0 &&
               (switch1Value != 0 || switch2Value != 0))
            {
                isLightsPaused = 0;
                sendStatus();

                waiting = 0;
                waitCount = 0;
            }
            switch1Value = s1now;
            switch2Value = s2now;
        }
    }
}
