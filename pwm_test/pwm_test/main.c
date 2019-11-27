/*
 * pwm_test.c
 *
 * Created: 11/14/2019 7:38:08 PM
 * Author : Joshua
 */ 

#include <avr/io.h>
#include <util/delay.h>
#define F_CPU 1000000UL


// pwm_init(){
// 	 DDRA |= (1 << DDA0);
// 	 // PA0 is now an output
// 
// 	 ICR1 = 0xFFFF;
// 	 // set TOP to 16bit
// 
// 	 OCR0A = 0x01FF;
// 	 // set PWM for 50% duty cycle @ 16bit
// 
// 	 TCCR0A |= (1 << COM1A1)|(1 << COM0A0);
// 	 // set none-inverting mode
// 
// 	 TCCR0A |= (1 << WGM01);
// 	 TCCR0B |= (1 << WGM12)|(1 << WGM13);
// 	 // set Fast PWM mode using ICR1 as TOP
// 	 
// 	 TCCR0B |= (1 << CS10);
// 	 // START the timer with no prescaler	
// }

#define b1 (~PIND & 0x01)
#define b2 (~PIND & 0x02)

int main(void)
{
    DDRB = 0xFF;
	DDRD = 0x00; 
 	TCCR0A = (1<<COM0A1)|(1<<COM0A0)|(1<<COM0B1)|(1<<COM0B0)|(1<<WGM01)|(1<<WGM00);
 	TCCR0B = (1<<CS00);
	OCR0A = 128;
// 	OCR0B = 0;
//    pwm_init();
	 while (1) 
    {
		if (b1)
		{
			 if (OCR0A < 255)
			 {
				 OCR0A++;
			 }
			 _delay_ms(50);
		} 
		if (b2)
		{
			if (OCR0A > 0)
			{
				OCR0A--;
			}
			_delay_ms(50);
		}
    }
}

