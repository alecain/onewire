

#ifndef pins_h
#define pins_h

#include <inttypes.h>

#define INPUT 0
#define OUTPUT 1
#define LOW 0
#define HIGH 1

#define OSCSPEED	8000000 	/* in Hz */

void pinMode(volatile uint8_t * port,int pin, uint8_t status);

void digitalWrite(volatile uint8_t * port,int pin, uint8_t status);

uint8_t digitalRead(volatile uint8_t * port,int pin);

void delayMicroseconds(unsigned long int us);

#endif
