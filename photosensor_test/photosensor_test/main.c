/*
 * photosensor_test.c
 *
 * Created: 12/1/2019 1:36:33 PM
 * Author : Joshua
 */ 

#include <avr/io.h>

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}


int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	unsigned short tmp = 0x00;
	unsigned char val;
	ADC_init();
	
    while (1) 
    {
		tmp = (char)ADC;
		if (tmp < 255)
		{
			PORTB = 0x00;
		}
		else{
			PORTB = 0x01;
		}
	}
	
}


// 
// unsigned short x = 0x00;
// unsigned char tmp;
// 
// int main(void)
// {
//     DDRA = 0x00; PORTA = 0xFF;
//     DDRB = 0xFF; PORTB = 0x00;
//     DDRD = 0xFF; PORTD = 0x00;
//     ADC_init();
//     while(1)
//     {
// 	    x = ADC;
// 	    PORTD = (char)(x >> 8);
// 	    PORTB =  (char)x;
//     }
// }

