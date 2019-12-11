#include <avr/io.h>
#include <avr/interrupt.h>
#include <bit.h>
#include <timer.h>
#include <stdio.h>
#include <util/delay.h>
#include <stdbool.h>
#include <alloca.h>
#include <stdlib.h>

//--------Find GCD function --------------------------------------------------
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}
//--------End find GCD function ----------------------------------------------

void transmit_data(unsigned char data){
	for(unsigned int i = 0; i < 8; i++){
		PORTC = SetBit(PORTC, 7, 1); //Set SRCLR to high
		PORTC = SetBit(PORTC, 6, 0); //Set SRCLK to low
		PORTC = SetBit(PORTC, 4, GetBit(data,i)); //Set SER to send bit
		PORTC = SetBit(PORTC, 6, 1); //Set SRCLK to high
	}
	PORTC = SetBit(PORTC, 5, 1);
	PORTC = SetBit(PORTC, 7, 0); //Set SRCLR to low
}


void pwm_init(){
	 	TCCR0A = (1<<COM0A1)|(1<<COM0A0)|(1<<COM0B1)|(1<<COM0B0)|(1<<WGM01)|(1<<WGM00);
	 	TCCR0B = (1<<CS00);
	 	TCCR1A =  (1<<COM1A1)|(1<<COM1A0)|(1<<COM1B1)|(1<<COM1B0)|(1<<WGM11)|(1<<WGM10);
	 	TCCR1B = (1<<CS00);
	 	TCCR2A =  (1<<COM2A1)|(1<<COM2A0)|(1<<COM2B1)|(1<<COM2B0)|(1<<WGM21)|(1<<WGM20);
	 	TCCR2B = (1<<CS00);
	 	OCR0A = 128;
	 	OCR0B = 128;
	 	OCR1A = 128;
	 	OCR1B = 128;
	 	OCR2B = 128;
	 	OCR2A = 128;
}

void ADC_init() {
	//ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// AREF = AVcc
	ADMUX = (1<<REFS0);
	
	// ADC Enable and prescaler of 128
	// 16000000/128 = 125000
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}
uint16_t ADC_read(uint8_t ch)
{
	// select the corresponding channel 0~7
	// ANDing with ’7? will always keep the value
	// of ‘ch’ between 0 and 7
	ch &= 0b00000111;  // AND operation with 7
	ADMUX = (ADMUX & 0xF8)|ch; // clears the bottom 3 bits before ORing
	
	// start single conversion
	// write ’1? to ADSC
	ADCSRA |= (1<<ADSC);
	
	// wait for conversion to complete
	// ADSC becomes ’0? again
	// till then, run loop continuously
	while(ADCSRA & (1<<ADSC));
	
	return (ADC);
}


//--------Task scheduler data structure---------------------------------------
typedef struct _task {
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	int (*TickFct)(int);
} task;
//--------End Task scheduler data structure-----------------------------------

//--------Shared/Global Variables----------------------------------------------------
unsigned char tmp = 0xFF;
unsigned char cnt = 0x00;
unsigned char manual = 0x00;
unsigned short x_axis = 0x0000;
unsigned short y_axis = 0x0000;

#define b1 (~PIND & 0x01)
#define b2 (~PIND & 0x02)
#define s (~PIND & 0x04)
//--------End Shared/Global Variables------------------------------------------------

//--------User defined FSMs---------------------------------------------------
enum SM1_States{Wait, Act} state1;

int j_Tick(int state1){
	x_axis = ADC_read(0);
	y_axis = ADC_read(1);
	
	switch(state1){
		case Wait:
		state1 = Act;
		break;
		case Act:
		state1 = Act;
		break;
		default:
		state1 = Wait;
		break;
	}
	switch(state1){
		case Wait:
		break;
		case Act:
		if(x_axis >= 950){//up
			if (OCR0A > 0)
			{
				OCR0A = OCR0A - 2;
				OCR0B = OCR0B - 2;
				OCR1A = OCR1A - 2;
				OCR1B = OCR1B - 2;
				OCR2A = OCR2A - 2;
				OCR2B = OCR2B - 2;
			}
		}
		else if(x_axis <= 50){ //down
			if (OCR0A < 253)
			{
				OCR0A = OCR0A + 2;
				OCR0B = OCR0B + 2;
				OCR1A = OCR1A + 2;
				OCR1B = OCR1B + 2;
				OCR2A = OCR2A + 2;
				OCR2B = OCR2B + 2;
			}
		}
		else if(y_axis >= 950){
		}
		else if(y_axis <= 50){
		}
		else{
		}
		break;
		default:
		break;
	}
	return state1;
}


enum DisplayState {init, wait, on, leave, swap, idle} d_state;
int d_Tick(){
	//Transitions
	switch(d_state){
		case init:
		d_state = wait;
		break;
		case wait:
		if (!(PIND & (1<<0)))
		{
			d_state = on;
		}
		else if(b2){
			d_state = swap;
		}
		else{
			d_state = wait;
		}
		break;
		case on:
		if (!(PIND & (1<<0)))
		{
			d_state = on;
		}
		
		else if(b2){
			d_state = swap;
		}
		else{
			d_state = leave;
		}
		break;
		case leave:
		if (!(PIND & (1<<0)))
		{
			d_state = on;
		}
		
		else if(b2){
			d_state = swap;
		}
		else if ((PIND & (1<<0)) && cnt >= 20)
		{
			d_state = wait;
		}
		else{
			d_state = leave;
		}
		break;
		case swap:
		if (b2){
			d_state = swap;
		}
		else if (manual == 0x00){
			d_state = idle;
		}
		else{
			d_state = wait;
		}
		break;
		case idle:
		if (b2)
		{
			d_state = swap;
		}
		else{
			d_state = idle;
		}
		break;
		default:
		d_state = init;
		break;
	}
	//Actions
	switch(d_state){
		case init:
		break;
		case wait:
		manual = 0x00;
		cnt = 0;
		break;
		case on:
		cnt = 0;
		transmit_data(tmp);
		break;
		case leave:
		cnt++;
		break;
		case swap:
		cnt = 0;
		break;
		case idle:
		manual = 0x01;
		break;
		default:
		break;
	}
	return d_state;
}

enum SwitchState {init2, automatic, power, off, swap2} s_state;
int s_Tick(){
	//Transitions
	switch(s_state){
		case init2:
		//s_state = init2;
		s_state = automatic;
		break;

		case automatic:
		if (b2)
		{
			s_state = swap2;
		}
		else
		{
			s_state = automatic;
		}
		break;

		case power:
		if (!s && !b2)
		{
			s_state = off;
		}
		else if (s && !b2)
		{
			s_state = power;
		}
		else{
			s_state = swap2;
		}
		break;
		
		case off:
		if (!s && !b2)
		{
			s_state = off;
		}
		else if (s && !b2)
		{
			s_state = power;
		}
		else{
			s_state = swap2;
		}
		break;
		
		case swap2:
		if (b2){
			s_state = swap2;
		}
		else if (!b2 && manual == 0x00){
			s_state = automatic;
		}
		else{
			s_state = off;
		}
		break;
		
		default:
		s_state = init2;
		break;
	}
	//Actions
	switch(s_state){
		case init2:
		break;

		case automatic:
		break;

		case power:
		transmit_data(0xCF);
		break;

		case off:
		break;

		case swap2:
		break;

		default:
		break;
	}
	return s_state;
}

// --------END User defined FSMs-----------------------------------------------

// Implement scheduler code from PES.
int main()
{
	DDRA = 0x00; //PORTA = 0x00;
	DDRB = 0xFF; //PORTB = 0x00;
	DDRC = 0xFF; //PORTC = 0x00;
	DDRD = 0x00; //PORTD = 0xF0;
	// Period for the tasks
	unsigned long int SMTick1_calc = 50;
	unsigned long int SMTick2_calc = 50;
	unsigned long int SMTick3_calc = 50;

	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(tmpGCD, SMTick1_calc);
	tmpGCD = findGCD(tmpGCD, SMTick2_calc);
	tmpGCD = findGCD(tmpGCD, SMTick3_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;
	unsigned long int SMTick3_period = SMTick3_calc/GCD;

	//Declare an array of tasks
	static task task1;
	static task task2;
	static task task3;
	task *tasks[] = {&task1, &task2, &task3};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMTick1_period;//Task Period.
	task1.elapsedTime = SMTick1_period;//Task current elapsed time.
	task1.TickFct = &d_Tick;//Function pointer for the tick.

	// Task 2
	task2.state = -1;//Task initial state.
	task2.period = SMTick2_period;//Task Period.
	task2.elapsedTime = SMTick2_period;//Task current elapsed time.
	task2.TickFct = &s_Tick;//Function pointer for the tick.
	
	// Task 3
	task3.state = -1;//Task initial state.
	task3.period = SMTick3_period;//Task Period.
	task3.elapsedTime = SMTick3_period;//Task current elapsed time.
	task3.TickFct = &j_Tick;//Function pointer for the tick.

	
	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();

	ADC_init();
	pwm_init();

	PORTC = SetBit(PORTC, 7, 1); //Set SRCLR to high
	PORTC = SetBit(PORTC, 5, 0); //Set RCLK to low

	//unsigned char a = 0xAA;
	//transmit_data(a);

	unsigned short i;
	while(1) {
		for ( i = 0; i < numTasks; i++ ) {
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
			PORTC = SetBit(PORTC, 5, 0); //Set RCLK to low
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
	return 0;
}
