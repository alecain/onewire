/*	Sample program for Olimex AVR-P28 with ATMega8 processor
 *	Echoes back the received characters on the uart. In order to work,
 *	connect the RX pad with PD1(pin 3) and TX pad with PD0(pin 2)
 *	Compile with AVRStudio+WinAVR (gcc version 3.4.6)
 */


#include "avr/io.h"
#include "OneWire.h"
#include <inttypes.h>

OneWire device;


void Initialize(void)
{
	PORTB = 0x0;
	PORTC = 1<<5;	/* turn the LED off */
	PORTD = 0x0;

	DDRB = 0x0;
	DDRC = 1<<5;	/* PC5 as output - the LED is there */
	DDRD = 0x0;



}

void InitUART(uint32_t baud)	/* here a simple int will not suffice*/	
{
	int baudrate=(OSCSPEED/16/baud-1); /* as per pg. 133 of the user manual */
	/* Set baud rate */
	UBRRH = (unsigned char)(baudrate>>8);
	UBRRL = (unsigned char)baudrate;
	/* Enable Receiver and Transmitter */
	UCSRB = (1<<RXEN)|(1<<TXEN);
	/* Set frame format: 8data, 1stop bit */
	UCSRC = (1<<URSEL)|(3<<UCSZ0);
	
}

unsigned char UARTReceive(void)
{
	if (UCSRA & (1<<RXC))
		return UDR;
	else	/* no data pending */
		return 0;
}

void UARTTransmit(unsigned char data)
{
	while (!( UCSRA & (1<<UDRE)) );
	/* Put data into buffer, sends the data */
	UDR = data;
}

/*	state = 0 -> Led Off
 *	state = 1 -> Led On
 *	state !=[0,1] -> Led Toggle 
 */
void LedSet(unsigned char state)
{
	switch (state)
	{
		case 0:
			PORTC &= ~(1<<5);
			break;
		case 1:
			PORTC |= 1<<5;
			break;
		default:
			if (PORTC & 1<<5) 
				PORTC &= ~(1<<5);
			else
				PORTC |= 1<<5;
	}
	
}

void print(char* buff){
	int i;
	i=0;
	while(buff[i]!=0){
		UARTTransmit(buff[i]);
		i++;
	}
} 

void printNum(uint8_t n, uint8_t  base){

	unsigned char buf[3]; // Assumes 8-bit chars. 
	unsigned long i;
	i=0;

	if (n == 0) {
		UARTTransmit('0');
		UARTTransmit('0');
		return;
	} 

	while (n>0) {
		buf[i++] = n % base;
		buf[i+1]=0;
		n /= base;
	}

	for (i=2; i > 0; i--)
		UARTTransmit((char) (buf[i - 1] < 10 ?
		'0' + buf[i - 1] :
		'A' + buf[i - 1] - 10));
}


int main(void)
{
	unsigned char ch;
	int i;

	uint8_t addr[8]= {1,1,1,1,1,1,1,1};;
	

	Initialize();
	InitUART(9600);
	LedSet(1);

	initOneWire(1,&device);

	while (1)
	{
		ch=UARTReceive();
		if (ch){
			print("recieved:");
			UARTTransmit(ch);

			if(search(addr,&device)){
				print("\r\n");
				print("New:");
			}
			print("\r\n");
			printNum(read_bit( &device),16);
			print("\r\n");

			for (i=0;i<8;i++){
				printNum(addr[i],16);	
				//delayMicroseconds(10001);
				print(" ");
			}

			reset_search(&device);
		}
		print(".");
		for (i=0;i<100;i++)
		delayMicroseconds(10000);


	}
	return 0;
}
