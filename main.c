/* Messprogramm fuer eine Messung mit
   Hydac Drucksensor
   Contrinex Induktiver Naeherungsschalter
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>

void usi_uart_init();
void usi_uart_transmit(uint8_t data);
void tcnt1_init_100ms();

volatile uint8_t usi_status;
volatile uint8_t usi_databuffer;

volatile uint16_t adcresults[8];
volatile uint8_t adcchannel;

volatile uint8_t timer_signal;

int main(void)
{
/* CONFIGURE I/O PINS, all unused Port pins as input with pull up */

  DDRA  = 0b00000000;
  PORTA = 0b11111100;  

  DDRB  = 0b00001010;
  PORTB = 0b01110100;

  usi_uart_init();
  tcnt1_init_100ms();

/* Configure and Enable the ADC */
  ADCSRA = 0b10001011;
  DIDR0  = 0b00011111;
  DIDR1  = 0b00000000;
 
  ADMUX  = 0b11000000;
  ADCSRB = (1 << REFS2);

  sei();

/*  Start the first conversion */ 
  ADCSRA |= (1 << ADSC);

  while(1)
  {
    if(timer_signal || (TIFR & (1 << TOV1))) /* Timer 1 Overflow Flag */
    {
      timer_signal = 0;
      TIFR |= (1 << TOV1); /* Clear the Timer 1 Overflow Flag */
      usi_uart_transmit('x');

/*
      if(PORTB & 0b00001000)
      {
        PORTB &= 0b11110111;
      }
      else
      {
        PORTB |= 0b00001000;
      }
*/
    }
/*
    if(adcresults[1] > 500)
    {
      PORTB |= 0b00001000;
    }
    else
    {
      PORTB &= 0b11110111;
    }
*/
  }

  return 0;
}

void tcnt0_init_9600baud()
{
  OCR0A = 104;
  TCCR0A = 0b00000001; /* (1 << CTC0) */
  TCCR0B = (0 << CS02)|(0 << CS01)|(1 << CS00);  /* Clock Select: No Prescaling */
}

void usi_uart_init()
{
/*
 Timer/Counter0 is used for driving the USI Baud Rate. ATtiny861 uses
 the Compare Match function.
*/
  DDRB  |= 0b00000010;
  tcnt0_init_9600baud();
  USICR = (1 << USIOIE) | (1 << USIWM0);   /* enable the usi, except the clock source */
}

uint8_t nibble_reverse_lookup[] = {0, 8, 4, 0xC, 2, 0xA, 6, 0xE, 1, 9, 5, 0xD, 3, 0xB, 7, 0xF};

uint8_t byte_reverse(uint8_t data)
{
  uint8_t rb = 0;
  rb |= (nibble_reverse_lookup[(data & 0xF)] << 4);
  rb |= (nibble_reverse_lookup[((data >> 4) & 0xF)]);
  return rb;
}

void usi_uart_transmit(uint8_t data)
{
  while(usi_status != 0)
  {
    ;
  }
  usi_status = 0x01;  /* Set UART status busy */
  usi_databuffer = byte_reverse(data);
  USIDR = 0xFF;       /* Write some high bits to USI data register */
  USISR = (1 << USIOIF) | 0b1110; /* send high bits until counter overflow */
  USICR |= (0 << USICS1)|(1 << USICS0)|(0 << USICLK); /* enable the usi clock source */
}

void tcnt1_init_100ms()
{
  TCCR1A = 0b00000000;
/*  TCCR1B = 0b00001010;*/ /* enable the timer with prescaler 512 */
  TCCR1B = 0b00001101; /* enable the timer with prescaler 4096 */
/*  TCCR1C = 0b00000000;
  TCCR1D = 0b00000000;
  TCCR1E = 0b00000000;
  PLLCSR = 0b00000000;
  OCR1A  = 0b00000000;
  OCR1B  = 0b00000000; */
/*  OCR1C  = 195;       */ /* Set the TOP Value so that we will get 100 ms rate */
  OCR1C  = 244;        /* Set the TOP Value so that we will get 1000 ms rate */
/*  OCR1D  = 0b00000000; */
/*  TIMSK |= 0b01000000; */
}

ISR(ADC_vect)
{
  uint16_t result;
  result = 0x0000;
  result |= (uint16_t)ADCL;
  result |= ((uint16_t)ADCH) << 8;
  adcresults[adcchannel] = result;
  adcchannel++;
  adcchannel &= 0b00000001;
  ADMUX  = 0b11000000 | adcchannel;
  /*  Start the next conversion */ 
  ADCSRA |= (1 << ADSC);
}

ISR(TIMER1_COMPA_vect)
{
  TCNT1 = 0x00;
  timer_signal = 1;
}

ISR(TIMER1_OVF_vect)
{

}

ISR(TIMER0_OVF_vect)
{

}

ISR(USI_START_vect)
{

}

ISR(USI_OVF_vect)
{
  switch(usi_status)
  {
    case 1:
    {
      /* Write a high bit, the start bit and the first 6 data bits to USI Data Register */
      USIDR = 0b10000000 | (usi_databuffer >> 2);
      USISR = (1 << USIOIF) | 0b1010;  /* send 6 bits (1 x high, 1 x start bit, 4 of the data bits) */
      if(PORTB & 0b00001000)
      {
        PORTB &= 0b11110111;
      }
      else
      {
        PORTB |= 0b00001000;
      }
      usi_status = 2;
      break;
    }
    case 2:
    {
      USIDR = (usi_databuffer << 4) | 0b00001111;  /* write the data to the bus */
      USISR = (1 << USIOIF) | 0b1010;      /* send 6 bits (4 data and 2 stop bits) */
      usi_status = 3;
      break;
    }
    default:
    {
      USICR &= ~((1 << USICS1)|(1 << USICS0)|(1 << USICLK)); /* disable the clock source */
      USISR = (1 << USIOIF);
      usi_status = 0;
      break;
    }
  }
}


