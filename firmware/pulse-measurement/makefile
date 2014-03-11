.PHONY: all
all: pulse-measurement


#############################################################################
.PHONY: pulse-measurement
pulse-measurement: pulse-measurement.hex

pulse-measurement.hex: pulse-measurement.o servo-input.o uart-output.o tlc5940.o
	sdcc --use-non-free -mpic14 -p16f1825 -o $@ $^


#############################################################################
.c.o:
	sdcc --use-non-free -mpic14 -p16f1825 -c $<

#############################################################################
#tlc5940.o: tlc5940.s
#	gpasm -c -o $@ $<


clean :
	$(RM) *.hex
	$(RM) *.lst
	$(RM) *.map
	$(RM) *.cod
	$(RM) *.o
	$(RM) *.asm
	
