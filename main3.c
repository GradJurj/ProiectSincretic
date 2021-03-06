
#include <avr/io.h>
#include <avr/interrupt.h>
#include "UART.h"
//#include "ADC.h"
#include "EEPROM.h"
#include <stdint-gcc.h>
#include <math.h>
#include <avr/signal.h>

#define BAUD 9600
#define MYUBRR 16000000/16/BAUD-1



int dsec = 0;
int secunda = 1;
int _step = 0;
int _step_reverse= 61;
int _ocr=255;
int nr = 0;
float cel = 0;
int contor_temperatura = 0;
float valoare_temperatura = 0;
float valoare_afisaj = 0;
char reverse_number[10];
int nr_to_display = 0;
volatile int temp_serial = 0;
int v[101]; // this is for getting the medium values
int pressbtnState = 0;
int pressbtnTime = 0;


int main(void)
{
	DDRD = 0xFF;
	DDRB = 0x3F;
	setTimer0();
	setTimer1();
	setTimer2();
	adc_init();
	init_button();
		// asta e ca la inceput s? afiseze ultima val
		 nr_to_display = EEPROM_read(200);
		 Afisaj_Segmente(nr_to_display);
		 // stop afisare ultima valoare
	sei();
	//Afisaj_Segmente(6);
	USART_Init(MYUBRR); // initializare USART
    //  Replace with your application code 
    while (1) 
    {
			char data = "*";
			data = USART_Receive();
			if (data == 'A')
			{
				PORTD  |= (1<<7);
			}
			else
			if ( data == 'S' )
			{
				PORTD &= ~(1<<7);
			}
	} 
}


void adc_init()
{
	//setting Port A as output
	DDRC = 0x00;
	//adc enable, interrupt enable, prescaler of 128
	ADCSRA |=(1<<ADEN)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0) | (1<<MUX0);  // set A1 as input
	//Left justify, AVcc voltage reference
	ADMUX |=(1<<ADLAR)|(1<<REFS0);
	ADCSRA |= 1<<ADSC;
	
}

ISR(ADC_vect)
{
	uint16_t val = ADC;
	int i;
	float voltage = val/5.0; //4.7
	voltage = voltage/1024; // rezolutia
	cel = (voltage-0.47)*100;
	v[contor_temperatura] = cel;
	contor_temperatura++;
	if (contor_temperatura == 100)
	{
			contor_temperatura = 0;
			for( i = 40; i<=60; i++)
			{
				valoare_temperatura += v[contor_temperatura];
			}
			if( valoare_temperatura >= 15)
				PORTD |=(1<<5);
			else
				PORTD &= ~(1<<5);
			temp_serial = (int)(valoare_temperatura/21);
			valoare_temperatura = 0;
	}
	ADCSRA |= 1<<ADSC;		// Start Conversion
}

void Display_Temperature(int val_temp)
{

	// val_temp - temperatura
	// afisarea se face cifra cu cifra;
	char numbers[10] ={0,};
	sprintf(numbers,"%d\n",val_temp);
	SendString(numbers);
}
void loop()
{
	
}


void setTimer0() //genereaza PWM
{
	//seteaza mod non-inverted
	TCCR0A |= (1 << COM0A1);
	
	TCCR0A |= (1 << WGM01) | (1 << WGM00); //Fast PWM
	
	TCCR0B |= (1 << CS00); // no prescaler
}

void setTimer1()
{
	TCCR1A = 0x00;
	TCCR1B = 0x00;
	
	//  TCCR1B |= (1 << WGM12); //CTC mode
	TIMSK1 |= (1 << TOIE1);  // enable overflow interrupt
	//intrerupere la 1 secunda
	//seteaza prescaler la 1024
	TCNT1 = 0xC2F7;
	TCCR1B |= (1 << CS10);
	TCCR1B |= (1 << CS12);
	sei(); // enagle global interrupts
}


ISR(TIMER1_OVF_vect)
{
	// La fiecare intrerupere, aprinde led-ul
	// De pe pinul 1
	TCNT1 = 0xC2F7;
	PORTD ^= ( 1 << 2 ); // blink la o secunda
	dsec++;
	if ( dsec == 2)
	{
		Display_Temperature(temp_serial);
		USART_Transmit('\n');

		nr++;
		if(nr == 10)
			nr = 0;
		dsec = 0;
	}
}

void setTimer2()
{
	TCCR2A = 0;
	TCCR2B = 0;
	//seteaza mod CTC
	TCCR2A |= (1 << WGM21);

	OCR2A=254; //intrerupere la 16ms

	//seteaza prescalera la 1024
	TCCR2B |= (1<<CS22) | (1 << CS21) | (1<<CS20);

	//activeaza intreruperi
	TIMSK2 |= (1 << OCIE2A);
	sei();
}


void _fadeLed2()
{
	_step++;
	_step_reverse--;
	
	if( _step >= 61 && _step_reverse<=0 && secunda >= 4)
	{
		_step = 0;
		_step_reverse=61;
		secunda = 1;
	}
	if ( _step >= 61 && _step_reverse<=0 )
	{
		_step = 0;
		_step_reverse=61;
		secunda ++;
	}


	switch(secunda)
	{
		case 1:
		OCR0A = 3*_step;
		break;
		case 2:
		OCR0A = 254;
		_ocr = 254;
		break;
		case 3:
		OCR0A = 3*_step_reverse;
		break;
		case 4:
		OCR0A = 0;
		break;
		default:
		break;

		PORTD |= ( 1<<6 ); // DIMMING LED 
	}
}


ISR(TIMER2_COMPA_vect)
{
	_fadeLed2();
}


void Afisaj_Segmente(int nr)
{
	switch (nr)
	{
		case 0:
		{
			PORTB |= ( 1<<PINB0 ) | (1<<PINB3 ) | (1<<PINB4) | (1<<PINB2) | (1<<PINB1);
			PORTD |= ( 1<<PIND4 );
			PORTB &= ~(1<<PINB5);
		}
		break;
		case 1:
		{
			PORTD |= ( 1<<PIND4 );
			PORTB &= ~(1<<PINB0) & ~(1<<PINB1) & ~(1<<PINB3) & ~(1<<PINB4)& ~(1<<PINB5);
			PORTB |= ( 1<<PINB2);
		}
		break;
		case 2:
		{
			PORTD &= ~( 1<<PIND4 );
			PORTB &= ~(1<<PINB0);
			PORTB |=  ( 1<<PINB1 ) | (1<<PINB2 ) | ( 1<<PINB3 ) | (1<<PINB4 ) | ( 1<<PINB5 );
		}
		break;
		case 3:
		{
			PORTD |= ( 1<<PIND4 );
			PORTB &= ~(1<<PINB3) &~ (1<<PINB0);
			PORTB |= (1<<PINB2 ) | ( 1<<PINB4 ) | ( 1<<PINB5 ) | (1<<PINB1);
		}
		break;
		case 4:
		{
			PORTB &= ~(1<<PINB1) & ~(1<<PINB3) &  ~(1<<PINB4);
			PORTB |=  ( 1<<PINB0 ) | ( 1<<PINB2 ) | ( 1<<PINB5 );
			PORTD |= ( 1<<PIND4 );
		}
		break;
		case 5:
		{
			PORTB &=  ~(1<<PINB2) & ~(1<<PINB3);
			PORTB =  ( 1<<PINB0 ) | (1<<PINB1 ) | (1<<PINB4 ) | ( 1<<PINB5 );
			PORTD |= ( 1<<PIND4 );
		}
		break;
		case 6:
		{
			PORTB &= ~(1<<PINB2);
			PORTB |=  ( 1<<PINB0 ) | (1<<PINB1 ) | (1<<PINB3 ) | (1<<PINB4 ) | ( 1<<PINB5 );
			PORTD |= ( 1<<PINB4 );
		}
		break;
		case 7:
		{
			PORTB &=  ~(1<<PINB0) & ~(1<<PINB3) & ~(1<<PINB4) & ~(1<<PINB5);
			PORTD |= ( 1<<PIND4 );
			PORTB |= ( 1<<PINB1 ) | ( 1<<PINB2 );
		}
		break;
		case 8:
		{
			PORTB |=  ( 1<<PINB0 ) | (1<<PINB1 ) | ( 1<<PINB2 ) | (1<<PINB3 ) | (1<<PINB4 ) | ( 1<<PINB5 );
			PORTD |= ( 1<<PIND4 );
		}
		break;
		case 9:
		{
			PORTB &=  ~(1<<PINB3);
			PORTB |=  ( 1<<PINB0 ) | (1<<PINB1 ) | ( 1<<PINB2 ) | (1<<PINB4 ) | ( 1<<PINB5 );
			PORTD |= ( 1<<PINB4 );
		}
		break;
		default:
		break;
	}
}


 void init_button()
 {
/*	// DDRD &= ~(1<<PIND3);
	// PORTD |= (1<<PIND3);
	 EIMSK |= (1<<INT1); // Enable INT0 External Interrupt
   
	 EICRA |= (1<<ISC11); // Rising-Edge Trigger INT1
 */
	//setting pushbutton pin as input
	DDRD &=~(1<<PIND3);
	// turn On the Pull-up
	PORTD |= (1 << PIND3);
	//interrupt on falling edge
	EICRA|=(1<<ISC11);
	//enabling external INT0
	EIMSK|=(1<<INT1);
	//selecting PIND2 as interrupt pin
	//PCMSK2|=(1<<PCINT18);
	
 }

 ISR(INT1_vect)
 {
	 pressbtnState ++;
	 if(pressbtnState >= 3)
	 {
		 pressbtnState = 0;
		 EEPROM_write(200,nr_to_display);
		 int nr = EEPROM_read(200);
		 Afisaj_Segmente(nr);
		 nr_to_display++;
		 if(nr_to_display >= 10)
			nr_to_display = 0;
	 }
}

