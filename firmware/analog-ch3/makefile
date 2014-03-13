.PHONY: all
all: analog-ch3


CFLAGS:=--opt-code-size --use-non-free -mpic14 -p12f1840 -I../hk310-filter -DPIC12F1840
LDFLAGS:=--use-non-free -mpic14 -p12f1840 

#############################################################################
.PHONY: analog-ch3
analog-ch3: analog-ch3.hex

analog-ch3.hex: main.o hk310-filter.o uart.o
	sdcc $(LDFLAGS) -o $@ $^

#############################################################################
main.c: ../hk310-filter/uart.h ../hk310-filter/hk310-filter.h
hk310-filter.c: ../hk310-filter/hk310-filter.h
uart.c: ../hk310-filter/uart.h

hk310-filter.o: ../hk310-filter/hk310-filter.c
	sdcc $(CFLAGS) -c $<

uart.o: ../hk310-filter/uart.c
	sdcc $(CFLAGS) -c $<

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
	
