# OTRSP BAND DECODER for Arduino
Arduino Band Decoder (only) Example for OTRSP Protocol Serial Messages
   
Sample code for decoding AUX messages from Logging programs that can use the OTRSP protocol.
   
   K7MDL 8/24/2020 for Arduino 
   
   This code looks at the serial port for AUX commands from logger programs such as N1MM+ and Logger32 
   which use the OTRSP protocol to send out serial messages to radios typically intended for SO2R operation. 
   One of the message types is the AUX message which is a BCD value (per radio) for the purpose of switching 
   things like antennas or transverters. For N1MM+ use the table in the Configurer Antennas tab. See the N1MM+
   documentation how to set this up.  
   
   This code goes as far as validating the message and validating it, then writes it to digital GPIO pins. 
   At this point you can choose to pass it on to the outside world as 4 bit BCD value on 4 GPIO pins (per radio) 
   or use as many at 16 GPIO pins per radio for a parallel IO approach. This is useful to control a SP6T coax 
   switch for example, or select a transverter's enbable line on band change.
   
   There is also code here to pass on the CW keying and PTT keying, in this example, from a USB line.  
   This is not straight forward depending on what CPU model you choose. CW and PTT are signaled by DTR and RTS
   and on CPUs like the Nano, DTR is used to reset the CPU to bootload software updates from the PC.
   On something like a Nano you can remove the cap on the reset line (requiring a manual reset to upload software)
   or likely easier to use a USB to UART TTL converter connected to one of your CPU serial port that has the DTR
   and RTS control lines. Not all converters do, or have both.
   
   You could take the PTT signal and 'AND' it with the BCD value in the code here to route PTT to a transverter 
   or amplifier. This can be enhanced further to be sequenced.
