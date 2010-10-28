/*
Copyright (c) 2007, Jim Studt

Updated to work with arduino-0008 and to include skip(OneWire* data) as of
2007/07/06. --RJL20

Modified to calculate the 8-bit CRC directly, avoiding the need for
the 256-byte lookup table to be loaded in RAM.  Tested in arduino-0010
-- Tom Pollard, Jan 23, 2008

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial data->portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Much of the code was inspired by Derek Yerger's code, though I don't
think much of that remains.  In any event that was..
    (copyleft) 2006 by Derek Yerger - Free to distribute freely.

The CRC code was excerpted and inspired by the Dallas Semiconductor 
sample code bearing this copyright.
//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial data->portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//--------------------------------------------------------------------------
*/

#include "OneWire.h"

#include <avr/io.h>




void initOneWire( uint8_t pinArg, OneWire* data)
{
    data->pin = pinArg;
    data->port = 0;
    data->bitmask =  1<<data->pin;
    data->outputReg = &PORTB;
    data->inputReg =&PINB;
    data->modeReg = &DDRB;
#if ONEWIRE_SEARCH
    reset_search(data);
#endif
}

//
// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
//
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
//
uint8_t reset(OneWire* data) {
    uint8_t r;
    uint8_t retries; 
    retries= 125;

    // wait until the wire is high... just in case
    pinMode(data->modeReg, data->pin,INPUT);
    do {
	if ( retries-- == 0) return 0;
	delayMicroseconds(2); 
    } while( !digitalRead(data->inputReg, data->pin));
    
    pinMode(data->modeReg,data->pin,OUTPUT);
    digitalWrite(data->outputReg,data->pin,0);   // pull low for 500uS
    delayMicroseconds(500);
    pinMode(data->modeReg,data->pin,INPUT);
    digitalWrite(data->outputReg,data->pin,HIGH); //enable pullup
   
    delayMicroseconds(65);
    r = digitalRead(data->inputReg,data->pin);
    delayMicroseconds(490);
    return r;
}

//
// Write a bit. data->port and bit is used to cut lookup time and provide
// more certain timing.
//
void write_bit(uint8_t v, OneWire* data) {
    static uint8_t lowTime[] = { 55, 5 };
    static uint8_t highTime[] = { 5, 55};
    
    v = (v&1);
    *(data->modeReg) |= data->bitmask;  // make data->pin an output, do first since we
                          // expect to be at 1
    *(data->outputReg) &= ~data->bitmask; // zero
    delayMicroseconds(lowTime[v]);
    *(data->outputReg) |= data->bitmask; // one, push data->pin up - important for
                           // parasites, they might start in here
    delayMicroseconds(highTime[v]);
}

//
// Read a bit. data->port and bit is used to cut lookup time and provide
// more certain timing.
//
uint8_t read_bit( OneWire* data) {
    uint8_t r;
    
    *(data->modeReg) |= data->bitmask;    // make data->pin an output, do first since we expect to be at 1
    *(data->outputReg) &= ~data->bitmask; // zero
    delayMicroseconds(1);
    *(data->modeReg) &= ~data->bitmask;     // let data->pin float, pull up will raise
    delayMicroseconds(5);          // A "read slot" is when 1mcs > t > 2mcs
    r = ( *(data->inputReg) & data->bitmask) ? 1 : 0; // check the bit
    delayMicroseconds(50);        // whole bit slot is 60-120uS, need to give some time
    

    return r;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// data->pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the data->pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
void write(uint8_t v, uint8_t power, OneWire* data) {
    uint8_t bitmask;
    
    for (bitmask = 0x01; bitmask; bitmask <<= 1) {
	write_bit( (bitmask & v)?1:0,data);
    }
    if ( !power) {
	pinMode(data->modeReg,data->pin,INPUT);
	digitalWrite(data->outputReg,data->pin,1);
    }
}

//
// Read a byte
//
uint8_t read(OneWire* data) {
    uint8_t bitmask;
    uint8_t r = 0;
    
    for (bitmask = 0x01; bitmask; bitmask <<= 1) {
	if ( read_bit(data)) r |= bitmask;
    }
    return r;
}

//
// Do a ROM select
//
void select( uint8_t rom[8], OneWire* data)
{
    int i;

    write(0x55,0,data);         // Choose ROM

    for( i = 0; i < 8; i++) write(rom[i],0,data);
}

//
// Do a ROM skip
//
void skip(OneWire* data)
{
    write(0xCC,0,data);         // Skip ROM
}

void depower(OneWire* data)
{
    pinMode(data->modeReg,data->pin,INPUT);
}

#if ONEWIRE_SEARCH

//
// You need to use this function to start a search again from the beginning.
// You do not need to do it for the first search, though you could.
//
void reset_search(OneWire* data)
{
    uint8_t i;
    
    data->searchJunction = -1;
    data->searchExhausted = 0;
    for( i = 7; ; i--) {
	data->address[i] = 0;
	if ( i == 0) break;
    }
}

//
// Perform a search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// data->address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its data->address is copied to newAddr.  Use reset_search(OneWire* data) to
// start over.
// 
uint8_t search(uint8_t *newAddr, OneWire* data)
{
    uint8_t i;
    char lastJunction = -1;
    uint8_t done = 1;
    
    if ( data->searchExhausted) return 0;
    
    if ( !reset( data)) return 0;
    write( 0xf0, 0,data);
    
    for( i = 0; i < 64; i++) {
	uint8_t a = read_bit( data );
	uint8_t nota = read_bit( data );
	uint8_t ibyte = i/8;
	uint8_t ibit = 1<<(i&7);
	
	if ( a && nota) return 0;  // I don't think this should happen, this means nothing responded, but maybe if
	// something vanishes during the search it will come up.
	if ( !a && !nota) {
	    if ( i == data->searchJunction) {   // this is our time to decide differently, we went zero last time, go one.
		a = 1;
		data->searchJunction = lastJunction;
	    } else if ( i < data->searchJunction) {   // take whatever we took last time, look in data->address
		if ( data->address[ ibyte]&ibit) a = 1;
		else {                            // Only 0s count as pending junctions, we've already exhasuted the 0 side of 1s
		    a = 0;
		    done = 0;
		    lastJunction = i;
		}
	    } else {                            // we are blazing new tree, take the 0
		a = 0;
		data->searchJunction = i;
		done = 0;
	    }
	    lastJunction = i;
	}
	if ( a) data->address[ ibyte] |= ibit;
	else data->address[ ibyte] &= ~ibit;
	
	write_bit( a,data);
    }

    if ( done) data->searchExhausted = 1;
    for ( i = 0; i < 8; i++) newAddr[i] = data->address[i];
    return 1;  
}
#endif

#if ONEWIRE_CRC
// The 1-Wire CRC scheme is described in Maxim Application Note 27:
// "Understanding and Using Cyclic Redundancy Checks with Maxim iButton Products"
//

#if ONEWIRE_CRC8_TABLE
// This table comes from Dallas sample code where it is freely reusable, 
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static uint8_t dscrc_table[] = {
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//
// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (note: this might better be done without to
// table, it would probably be smaller and certainly fast enough
// compared to all those delayMicrosecond(OneWire* data) calls.  But I got
// confused, so I use this table from the examples.)  
//
uint8_t crc8( uint8_t *addr, uint8_t len, OneWire* data)
{
    uint8_t i;
    uint8_t crc = 0;
    
    for ( i = 0; i < len; i++) {
	crc  = dscrc_table[ crc ^ addr[i] ];
    }
    return crc;
}
#else
//
// Compute a Dallas Semiconductor 8 bit CRC directly. 
//
uint8_t crc8( uint8_t *addr, uint8_t len, OneWire* data)
{
    uint8_t i, j;
    uint8_t crc = 0;
    
    for (i = 0; i < len; i++) {
        uint8_t inbyte = addr[i];
        for (j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}
#endif

#if ONEWIRE_CRC16
static short oddparity[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

//
// Compute a Dallas Semiconductor 16 bit CRC. I have never seen one of
// these, but here it is.
//
unsigned short crc16(unsigned short *data, unsigned short len, OneWire* data)
{
    unsigned short i;
    unsigned short crc = 0;
    
    for ( i = 0; i < len; i++) {
	unsigned short cdata = data[len];
	
	cdata = (cdata ^ (crc & 0xff)) & 0xff;
	crc >>= 8;
	
	if (oddparity[cdata & 0xf] ^ oddparity[cdata >> 4]) crc ^= 0xc001;
	
	cdata <<= 6;
	crc ^= cdata;
	cdata <<= 1;
	crc ^= cdata;
    }
    return crc;
}
#endif

#endif
