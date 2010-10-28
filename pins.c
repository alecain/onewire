#include "pins.h"
#include <inttypes.h>


void pinMode(volatile uint8_t* port,int pin, uint8_t status){

	if (status){
	*(port) |= 1<<pin;  //output
	}
	else{
	    *(port) &= ~(1<<pin); // input
	}
}

void digitalWrite(volatile uint8_t* port,int pin, uint8_t status){
	if (status){
		*(port) |= 1<<pin;  //output
	}
	else{
	    	*(port) &= ~(1<<pin); // input
	}
}

uint8_t digitalRead(volatile uint8_t * port,int pin){
	return *(port) &= 1<<pin;

}

void delayMicroseconds(unsigned long int us)
{
	// calling avrlib's delay_us() function with low values (e.g. 1 or
	// 2 microseconds) gives delays longer than desired.
	//delay_us(us);

	// for the 8 MHz internal clock on the ATmega168

	// for a one- or two-microsecond delay, simply return.  the overhead of
	// the function calls takes more than two microseconds.  can't just
	// subtract two, since us is unsigned; we'd overflow.
	if (--us == 0)
		return;
	if (--us == 0)
		return;

	// the following loop takes half of a microsecond (4 cycles)
	// per iteration, so execute it twice for each microsecond of
	// delay requested.
	us <<= 1;
    
	// partially compensate for the time taken by the preceeding commands.
	// we can't subtract any more than this or we'd overflow w/ small delays.
	us--;


	// busy wait
	__asm__ __volatile__ (
		"1: sbiw %0,1" "\n\t" // 2 cycles
		"brne 1b" : "=w" (us) : "0" (us) // 2 cycles
	);
}
