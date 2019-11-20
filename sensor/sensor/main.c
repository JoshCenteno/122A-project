#include <avr/io.h>
//#define F_CPU 8000000UL
#include <util/delay.h>
int main()
{
	DDRB=0x00; //configuring PortB pin 0 as input
	PORTB=0x00;
	DDRA=0x01; // configuring PortA as output
	PORTA=0x00; //LED off

	while(1)
	{
		if((PINB & (1<<0)))            // check for sensor pin PB0 using bit
		{
			PORTA = 0x01;            //LED on
//  		_delay_ms(5000);
// 			PORTA=0x00;
		}
		else
		PORTA=0x00;  //LED off
	}

	return 0;
}

