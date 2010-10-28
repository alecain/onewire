CC=avr-gcc
CFLAGS=-g -O2 -Wall -mcall-prologues -mmcu=atmega8
OBJ2HEX=avr-objcopy 
UISP=avrdude 
TARGET=test
SOURCES=$(wildcard *.c) 
OBJS= $(SOURCES:.c=.o)

PARGS= -c avrisp -b 19200 
PORT =  /dev/`dmesg  | grep "FTDI"| grep attached | sed 's/.*\(ttyUSB.\).*/\1/'`


.PHONY: find
find :
	$(export TARGET=`grep -H "main(" *.c | cut -d. -f1`)

program : $(TARGET).hex find
	$(UISP) $(PARGS) -p m8  -v -P $(PORT) -U flash:w:$(TARGET).hex

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.obj : %.o $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.hex : %.obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

test: 
	screen $(PORT) 9600
	
clean :
	rm -f *.hex *.obj *.o

