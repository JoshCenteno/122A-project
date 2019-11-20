/*
 * manual_test.c
 *
 * Created: 11/17/2019 5:43:57 PM
 * Author : Joshua
 */ 

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

#define s (~PIND & 0x01)
#define b2 (~PINB & 0x02)
//--------End Shared/Global Variables------------------------------------------------
//--------User defined FSMs---------------------------------------------------
// enum DisplayState {Start, automatic, on, off, swap} state;
// int d_Tick(){
// 	//Transitions
// 	switch(state){
// 			case Start:  // Initial transition
// 			state = automatic;
// 			break;
// 			case automatic:
//				if (b2)
// 				{
// 					state = swap;
// 				} 
// 				else
// 				{
// 					state = automatic;
// 				}
// 			break;
// 			case on:
// 			if (!s)// 			{
// 				state = off;
// 			}
//			else if (b2)
// 			{
// 				state = swap;
// 			}
// 			else{
// 				state = on;
// 			}
// 			break;
// 			
// 			case off:
// 			if (!s)
// 			{
// 				state = off;
// 			}
//			else if (b2)
// 			{
// 				state = swap;
// 			}
// 			else{
// 				state = on;
// 			}
// 			break;
// 			case swap:
// 				if (!b2 && manual == 0x01)
// 				{
// 					state = automatic;
// 				}
// 				else if (!b2 && manual == 0x00)
// 				{
// 					state = off;
// 				} 
// 				else
// 				{
// 					state = swap;
// 				}
// 			break;
// 			default:
// 			break;
// 			
// 	}
// 	//Actions
// 	switch(state){
// 		case Start:
// 		break;
// 		
// 		case automatic:
// 		manual = 0x00;
// 		break;
// 		
// 		case on:
// 		tmp = 0xFF;
// 		transmit_data(tmp);
// 		break;
// 		
// 		case  off:
// 		manual = 0x01;
// 		tmp = 0x00;
// 		transmit_data(tmp);
// 		break;
// 		
// 		case swap:
// 		break;
// 		
// 		default:
// 		break;
// 		
// 	}
// 	return state;
// }
enum DisplayState {Start, son, soff} state;
int d_Tick(){
	//Transitions
	switch(state){
		case Start:  // Initial transition
		state = soff;
		break;
		case son:
		if (!s)
		{
			state = soff;
		}
		else{
			state = son;
		}
		break;
		
		case soff:
		if (!s)
		{
			state = soff;
		}
		else{
			state = son;
		}
		break;
		default:
		break;
	}
	//Actions
	switch(state){
		case Start:
		break;
		case son:
		tmp = 0xFF;
		transmit_data(tmp);
		break;
		
		case  soff:
		tmp = 0x00;
		transmit_data(tmp);
		break;		
		default:
		break;
		
	}
	return state;
}

// --------END User defined FSMs-----------------------------------------------

// Implement scheduler code from PES.
int main()
{
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0x00; PORTB = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;

	// Period for the tasks
	unsigned long int SMTick1_calc = 1000;

	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(tmpGCD, SMTick1_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;


	//Declare an array of tasks
	static task task1;
	task *tasks[] = {&task1};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMTick1_period;//Task Period.
	task1.elapsedTime = SMTick1_period;//Task current elapsed time.
	task1.TickFct = &d_Tick;//Function pointer for the tick.

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
