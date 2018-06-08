/* Messprogramm fuer eine Messung mit
   Hydac Drucksensor
   Contrinex Induktiver Naeherungsschalter
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <inttypes.h>

struct uart_msg
{
  uint8_t id;
  uint8_t nb;
  uint8_t data[8];
};

typedef struct uart_msg uart_msg_t;

void usi_uart_init();
int8_t usi_uart_transmit(uint8_t* data, int8_t nb);
int8_t usi_uart_transmit_msg(uart_msg_t* msg);
void tcnt1_init_100ms();

volatile uint8_t usi_status;
#define USI_DATABUFFER_SIZE 0x0F
volatile uint8_t usi_databuffer[USI_DATABUFFER_SIZE+1];
volatile int8_t usi_idx_datastart;
volatile int8_t usi_idx_dataend;

volatile uint8_t adcresults[8];
volatile uint8_t adcchannel;

volatile uint8_t timer_signal;

uart_msg_t datamsg;

int main(void)
{
  int8_t i;
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

  timer_signal = 0;

  sei();

/*  Start the first conversion */ 
  ADCSRA |= (1 << ADSC);

  datamsg.id = 0x12;
  datamsg.nb = 8;

  while(1)
  {
    if(timer_signal)
    {
      timer_signal = 0;

      for(i=0 ; i<8 ; i++)
      {
        datamsg.data[i] = adcresults[i];
      }

      usi_uart_transmit_msg(&datamsg);

      if(PORTB & 0b00001000)
      {
        PORTB &= 0b11110111;
      }
      else
      {
        PORTB |= 0b00001000;
      }
    }
    sleep_enable();
    sleep_cpu();
    sleep_disable();
  }

  return 0;
}

void tcnt0_init_9600baud()
{
  OCR0A = 104;
  TCCR0A = 0b00000001; /* (1 << CTC0) */
  TCCR0B = (0 << CS02)|(1 << CS01)|(0 << CS00);  /* Clock Select: Prescaling 8 */
}

void usi_uart_init()
{
  usi_idx_datastart = 0;
  usi_idx_dataend   = 0;
/*
 Timer/Counter0 is used for driving the USI Baud Rate. ATtiny861 uses
 the Compare Match function.
*/
  DDRB  |= 0b00000010;
  tcnt0_init_9600baud();
  USICR = (1 << USIOIE) | (1 << USIWM0);   /* enable the usi, except the clock source */
}

static uint8_t nibble_reverse_lookup[] = {0, 8, 4, 0xC, 2, 0xA, 6, 0xE, 1, 9, 5, 0xD, 3, 0xB, 7, 0xF};

inline uint8_t byte_reverse(uint8_t data)
{
  uint8_t rb = 0;
  rb = (nibble_reverse_lookup[(data & 0xF)] << 4) | (nibble_reverse_lookup[(data >> 4)]);
  return rb;
}

int8_t usi_uart_transmit(uint8_t* data, int8_t nb)
{
  int8_t i;
  if(nb > (USI_DATABUFFER_SIZE + usi_idx_datastart - usi_idx_dataend))
  {
    return 0;
  }
  for(i=0 ; i<nb ; i++)
  {
    usi_databuffer[((usi_idx_dataend + i) & USI_DATABUFFER_SIZE)]  = byte_reverse(data[i]);
  }
  cli();
  usi_idx_dataend += nb;
  if(usi_status == 0)
  {
    usi_status = 0x01;  /* Set UART status busy */
    USIDR = 0xF0;       /* Write some high bits to USI data register */
    USISR = (1 << USIOIF) | 0b1101; /* send high bits until counter overflow */
    USICR |= (0 << USICS1)|(1 << USICS0)|(0 << USICLK); /* enable the usi clock source */
  }
  sei();
  return nb;
}

inline int8_t usi_uart_transmit_msg(uart_msg_t* msg)
{
  uint8_t* dataptr = (uint8_t*)msg;
  int8_t nb = msg->nb + 2;
  if(usi_uart_transmit(dataptr, nb) > 0)
  {
    return 1;
  }
  return 0;
}

void tcnt1_init_100ms()
{
  TCCR1A = 0b00000000;
/*  TCCR1B = 0b00001010;*/ /* enable the timer with prescaler 512 */
//  TCCR1B = 0b00001111; /* enable the timer with prescaler 16384 */
  TCCR1B = 0b00001101; /* enable the timer with prescaler 4096 */

/*  TCCR1C = 0b00000000;
  TCCR1D = 0b00000000;
  TCCR1E = 0b00000000;
  PLLCSR = 0b00000000;
  OCR1A  = 0b00000000;
  OCR1B  = 0b00000000; */
  OCR1C  = 195;        /* Set the TOP Value so that we will get 100 ms rate */
//  OCR1C  = 244;        /* Set the TOP Value so that we will get 500 ms rate */
/*  OCR1D  = 0b00000000; */
  TIMSK |= (1 << TOIE1);
}

ISR(ADC_vect)
{
  adcresults[adcchannel*2] = ADCL;
  adcresults[adcchannel*2 + 1] = ADCH;
  adcchannel++;
  adcchannel &= 0b00000001;
  ADMUX  = 0b11000000 | adcchannel;
  // Start the next conversion 
  ADCSRA |= (1 << ADSC);
}

ISR(TIMER1_COMPA_vect)
{
//  TCNT1 = 0x00;
//  timer_signal = 1;
}

ISR(TIMER1_OVF_vect)
{
  timer_signal = 1;
}

ISR(TIMER0_OVF_vect)
{

}

ISR(USI_START_vect)
{

}

ISR(USI_OVF_vect)
{
  uint8_t c;
  switch(usi_status)
  {
    case 1:
    {
      /* Write the start bit and the first 6 data bits to USI Data Register */
      c = usi_databuffer[usi_idx_datastart & USI_DATABUFFER_SIZE];
      USIDR = 0b00000000 | (c >> 1);
      USISR = (1 << USIOIF) | 0b1011;  /* send 5 bits (1 x start bit, 4 of the data bits) */
      usi_status = 2;
      break;
    }
    case 2:
    {
      c = usi_databuffer[usi_idx_datastart & USI_DATABUFFER_SIZE];
      USIDR = (c << 4) | 0b00001111;  /* write the lower 4 data bits and stop bits */
      USISR = (1 << USIOIF) | 0b1010;      /* send 6 bits (4 data and 2 stop bits) */
      usi_idx_datastart++;
      if(usi_idx_datastart == usi_idx_dataend)
      {
        usi_idx_dataend &= USI_DATABUFFER_SIZE;
        usi_idx_datastart = usi_idx_dataend;
        usi_status = 3;
      }
      else
      {
        usi_status = 1;
      }
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


