  #include <avr/io.h>
#include <avr/interrupt.h>
#include <bit.h>
#include <timer.h>
#include <stdio.h>
#include <util/delay.h>

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

//--------Task scheduler data structure---------------------------------------
// Struct for Tasks represent a running process in our simple real-time operating system.
typedef struct _task {
    /*Tasks should have members that include: state, period,
        a measurement of elapsed time, and a function pointer.*/
    signed char state; //Task's current state
    unsigned long int period; //Task period
    unsigned long int elapsedTime; //Time elapsed since last task tick
    int (*TickFct)(int); //Task tick function
} task;

//--------End Task scheduler data structure-----------------------------------

//--------Shared Variables----------------------------------------------------
unsigned short x_axis = 0x0000;
unsigned short y_axis = 0x0000;

//--------End Shared Variables------------------------------------------------
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

void pwm_init(){
	 	TCCR0A = (1<<COM0A1)|(1<<COM0A0)|(1<<COM0B1)|(1<<COM0B0)|(1<<WGM01)|(1<<WGM00);
	 	TCCR0B = (1<<CS00);
	 	OCR0A = 128;
		OCR0B = 128;
}

//--------User defined FSMs---------------------------------------------------
enum SM1_States{Wait, Act} state1;

int SM1Tick1(int state1){
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
				//PORTB = 0x02; 
				if (OCR0A > 0)
				{
					OCR0A--;
					OCR0B--;
				}
				_delay_ms(50);
			}
			else if(x_axis <= 50){ //down
				//PORTB = 0x04;
				if (OCR0A < 255)
				{
					OCR0A++;
					OCR0B++;
				}
				_delay_ms(50);
			}
			else if(y_axis >= 950){ 
				//PORTB = 0x01;
			}
			else if(y_axis <= 50){
				//PORTB = 0x08;
			}
			else{
				//PORTB = 0x00;
			}
			break;
		default:
			break;
	}
	return state1;
}
// --------END User defined FSMs-----------------------------------------------

// Implement scheduler code from PES.
int main()
{
DDRA = 0x00; PORTA = 0xFF;
DDRB = 0xFF; PORTB = 0x00;


// Period for the tasks
unsigned long int SMTick1_calc = 50;

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
task1.TickFct = &SM1Tick1;//Function pointer for the tick.

// Set the timer and turn it on
TimerSet(GCD);
TimerOn();
ADC_init();
pwm_init();
unsigned short i; // Scheduler for-loop iterator
while(1) {
    // Scheduler code
    for ( i = 0; i < numTasks; i++ ) {
        // Task is ready to tick
        if ( tasks[i]->elapsedTime == tasks[i]->period ) {
            // Setting next state for task
            tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
            // Reset the elapsed time for next tick.
            tasks[i]->elapsedTime = 0;
        }
        tasks[i]->elapsedTime += 1;
    }
	while(!TimerFlag);
	TimerFlag = 0;
}

// Error: Program should not exit!
return 0;
}