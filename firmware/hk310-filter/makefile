.PHONY: all
all: hk310-filter hw-test


#CFLAGS:=--opt-code-size --use-non-free -mpic14 -p16f1825 
CFLAGS:=--no-peep --use-non-free -mpic14 -p16f1825 
LDFLAGS:=--no-peep --use-non-free -mpic14 -p16f1825 

#############################################################################
.PHONY: hk310-filter
hk310-filter: hk310-filter.hex

hk310-filter.hex: hk310-filter.o uart.o keyboard-matrix.o 
	sdcc $(LDFLAGS) -o $@ $^

#############################################################################
.PHONY: hw-test
hw-test: hw-test.hex

hw-test.hex: hw-test.o keyboard-matrix.o uart.o
	sdcc $(LDFLAGS) -o $@ $^



#############################################################################
hk310-filter.c: uart.h
hw-test.c: uart.h keyboard-matrix.h
keyboard-matrix.c: keyboard-matrix.h
uart.c: uart.h

#############################################################################
.c.o:
	sdcc $(CFLAGS) -c $<


clean :
	$(RM) *.hex
	$(RM) *.lst
	$(RM) *.map
	$(RM) *.cod
	$(RM) *.o
	$(RM) *.asm
	
