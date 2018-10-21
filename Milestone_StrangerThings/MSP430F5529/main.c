/*  MSP430F5529
    Author: Jacob Okun and Cameron Korzeniowski
    Created: October 10, 2018
    Last Updated: October 18, 2018*/

#include <msp430.h>

int byte = 0;
volatile unsigned int total;

void timer(void);   //timer setup  
void LED(void);     //LED setup
void UART(void);    //UART setup

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // Disable the watchdog timer

    timer();
    LED();
    UART();

    __bis_SR_register(LPM0_bits + GIE); // Enter Low Power Mode once interrupt is detected & Global Interrupt Enable
    __enable_interrupt();   // Enable interrupt algorithm

    for(;;){}; // Infinite for loop
}

void timer(void)
{
    TA0CTL = TASSEL_2 + MC_1 + ID_0 + TACLR;   // Timer A0 Control: SMCLK (~ 1MHz), Up mode, No Division, Clear timer

    TA0CCTL1 |= OUTMOD_3;   //Set and reset for red mode
    TA0CCTL2 |= OUTMOD_3;   //Set and reset for green mode
    TA0CCTL3 |= OUTMOD_3;   //Set and reset for blue mode

    TA0CCR0 = 256;  //Full Cycle
    TA0CCR1 = 0;    //Red LED set at 0%
    TA0CCR2 = 0;    //Green LED set at 0%
    TA0CCR3 = 0;    //Blue LED set at 0%
}

void LED(void)
{
    P1DIR |= BIT2 + BIT3 + BIT4;  //resets P1.2-4 to be outputs
    P1SEL |= BIT2 + BIT3 + BIT4;  //Enable PWM on P1.2-4
}

void UART(void)
{
    P4SEL |= BIT4 + BIT5;                  //Enable UART on P4.4 and P4.5

    UCA1CTL1 |= UCSWRST;                    //reset state machine
    UCA1CTL1 |= UCSSEL_2;                   //SMCLK for UART
    UCA1BR0 = 104;                          //9600 BAUD Rate
    UCA1BR1 = 0;                            //9600 BAUD Rate
    UCA1MCTL |= UCBRS_1 + UCBRF_0;          //Modulation
    UCA1CTL1 &= ~UCSWRST;                   //Initialize USCI state machine
    UCA1IE |= UCRXIE;                       //Enable USCI_A0 RX interrupt
    UCA1IFG &= ~UCRXIFG;                    //Clear interrupt flags

}

#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    switch(byte)
    {
    case 0:
            total = UCA1RXBUF; //Sets total equal to the first byte of UART signal 
            break;
    case 1:
            TA0CCR1 = UCA1RXBUF; //Sets CC register 1 of Timer A0 to the second byte of UART signal
            break;
    case 2:
            TA0CCR2 = UCA1RXBUF;  //Sets CC register 2 of Timer A0 to the third byte of UART signal
            break;
    case 3:
            TA0CCR3 = UCA1RXBUF;  //Sets CC register 3 of Timer A0 to the third byte of UART signal
            while(!(UCA1IFG & UCTXIFG));  //Checks to make sure TX buffer is ready
            UCA1TXBUF = total - 3;  //Sets the first byte of the transmitted UART signal to the previous total minus 3
            break;

    default:
            if(byte > total)  
            {
                byte = -1;
            }
            else
            {
                while(!(UCA1IFG & UCTXIFG)); //Checks to make sure TX buffer is ready
                UCA1TXBUF = UCA1RXBUF; //sets TX buffer to the remaining bytes stored in RX buffer
            }
            break;
    }

    byte += 1; //increments byte by 1 after each case, selecting the next case in switch
}
