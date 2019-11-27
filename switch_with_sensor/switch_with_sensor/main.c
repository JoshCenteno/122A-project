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

#define b1 (~PIND & 0x01)
#define b2 (~PIND & 0x02)
#define s (~PIND & 0x04)
//--------End Shared/Global Variables------------------------------------------------

//--------User defined FSMs---------------------------------------------------
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
		else if ((PIND & (1<<0)) && cnt >= 3)
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
		PORTB = 0x00;
		manual = 0x00;
		cnt = 0;
		transmit_data(0x00);
		break;
		case on:
		cnt = 0;
		transmit_data(tmp);
		break;
		case leave:
		cnt++;
		transmit_data(0xAA);
		break;
		case swap:
		cnt = 0;
		break;
		case idle:
		PORTB = 0x01;
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
			s_state = on;
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
			s_state = on;
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
		//transmit_data(0x66);
		transmit_data(0xFF);
		break;

		case off:
		//transmit_data(0x88);
		transmit_data(0x00);
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
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0x00; PORTD = 0xFF;
	// Period for the tasks
	unsigned long int SMTick1_calc = 500;
	unsigned long int SMTick2_calc = 500;


	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(tmpGCD, SMTick1_calc);
	tmpGCD = findGCD(tmpGCD, SMTick2_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;

	//Declare an array of tasks
	static task task1;
	static task task2;
	task *tasks[] = {&task1, &task2};
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
	
	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();

	PORTC = SetBit(PORTC, 7, 1); //Set SRCLR to high
	PORTC = SetBit(PORTC, 5, 0); //Set RCLK to low

	unsigned char a = 0xAA;
	transmit_data(a);

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
