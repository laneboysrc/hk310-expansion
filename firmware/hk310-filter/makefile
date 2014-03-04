.PHONY: all
all: hk310-filter


#############################################################################
.PHONY: hk310-filter
hk310-filter: hk310-filter.hex

hk310-filter.hex: hk310-filter.o uart.o
	sdcc --opt-code-size --use-non-free -mpic14 -p16f1825 -o $@ $^


#############################################################################
.c.o:
	sdcc --opt-code-size --use-non-free -mpic14 -p16f1825 -c $<


clean :
	$(RM) *.hex
	$(RM) *.lst
	$(RM) *.map
	$(RM) *.cod
	$(RM) *.o
	$(RM) *.asm
	
