/* ========================================
 *   OTRSP
 *   
 *   Sample code for decoding AUX messages from Logging programs that can use the OTRSP protocol.
 *   See http://www.k1xm.org/OTRSP for protocol details.
 *   
 *   K7MDL 8/24/2020 for Arduino 
 *   
 *   This code looks at the serial port for AUX commands from logger programs such as N1MM+ and Logger32 
 *   which use the OTRSP protocol to send out serial messages to radios typically intended for SO2R operation. 
 *   One of the message types is the AUX message which is a BCD value (per radio) for the purpose of switching 
 *   things like antennas or transverters. For N1MM+ use the table in the Configurer Antennas tab. See the N1MM+
 *   documentation how to set this up.  
 *   
 *   This code goes as far as validating the message and validating it, then writes it to digital GPIO pins. 
 *   At this point you can choose to pass it on to the outside world as 4 bit BCD value on 4 GPIO pins (per radio) 
 *   or use as many at 16 GPIO pins per radio for a parallel IO approach. This is useful to control a SP6T coax 
 *   switch for example, or select a transverter's enable line on band change.
 *   
 *   There is also code here (currently not active, needs to be adapted to Arduino) to pass on the CW keying and PTT keying, in this example, from a USB line.  
 *   This is not straight forward depending on what CPU model you choose. CW and PTT are signaled by DTR and RTS
 *   and on CPUs like the Nano, DTR is used to reset the CPU to bootload software updates from the PC.
 *   On something like a Nano you can remove the cap on the reset line (requiring a manual reset to upload software)
 *   or likely easier to use a USB to UART TTL converter connected to one of your CPU serial port that has the DTR
 *   and RTS control lines. Not all converters do, or have both.
 *   
 *   You could take the PTT signal and 'AND' it with the BCD value in the code here to route PTT to a transverter 
 *   or amplifier. This can be enhanced further to be sequenced.
 * 
 *   It is also possible for the PC program to do a query where we are supposed to return the status. 
 *   We are not handing any queries yet, so far it does not seem to matter if they go unanswered.
 *   
*/

/* ========================================
 *   OTRSP Band Decoder for AUx command parsing (only)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Declarations
int OTRSP(void);

// Vars
static byte AuxNum1, AuxNum2;

// Uncomment next line to see Serial Monitor debug statements
#define DEBUG

#define FALSE 0
#define TRUE 1
#define AUXCMDLEN 4
#define BANDCMDLEN 12
// define Aux Port pins - 4 for BCD.  Change if using more than 4 parallel IO.
// AUX1 output pins on Nano
#define AUX1_0  5       
#define AUX1_1  6
#define AUX1_2  7
#define AUX1_3  8
// AUX2 output pins on Nano
#define AUX2_0  9
#define AUX2_1  10
#define AUX2_2  11
#define AUX2_3  12

void setup() {
    pinMode(AUX1_0, OUTPUT);
    pinMode(AUX1_1, OUTPUT);
    pinMode(AUX1_2, OUTPUT);
    pinMode(AUX1_3, OUTPUT);

    pinMode(AUX2_0, OUTPUT);
    pinMode(AUX2_1, OUTPUT);
    pinMode(AUX2_2, OUTPUT);
    pinMode(AUX2_3, OUTPUT);

    // initialize serial:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
}

void loop() {
  int ret16, ret1;
 
/*
  // N1MM CW and PTT and AUX message handling  - On the Nano DTR resets the CPU for the bootloader 
  This section of code was lifted from a PSoC5 project so still needs to be adapted to Arduino.

  ret16 = Serial_USB_GetLineControl();  // look for DTR and RTS for N1MM control of CW and PTT. 
                                        // Can only happen on USB serial since the external USB UART
                                        //    has only TX and RX lines connected to CPU

  // Check state of USB Serial Port DTR register for N1MM CW keying state
  if ((uint8_t)ret16 & Serial_USB_LINE_CONTROL_DTR)                    
      CW_Key_Out_Write(1);          
  else
      CW_Key_Out_Write(0);                       

  // Check state of USB Serial Port RTS register for N1MM PTT state
  if ((uint8_t)ret16 & Serial_USB_LINE_CONTROL_RTS)     
      PTT_Out_Write(1);        
  else
      PTT_Out_Write(0);       
*/

  ret1 = OTRSP();   // set Aux output pins and change bands to match
  if (ret1)
  {
      #ifdef DEBUG
      Serial.print("Band Decode Output Values: Radio 1 = ");
      Serial.print(AuxNum1);
      Serial.print("  Radio 2 = "); 
      Serial.println(AuxNum2);
      #endif
  }  
  delay(500);
}

/* ========================================
 *  OTRSP parsing
 *  Parses serial port commands from N1MM Logging program to control antennas and transverter
 *  via BCD or parallel GPIO port pins.
 *  Only using the AUX commands not the whole SO2R list of commands so not using the whole SO2R
 *  list of possible commands and queries
 *  Created by K7MDL July 27, 2020 for RF Wattmeter to enable antenna switching , transverter
 *  selection, PTT, CW, and band labeling on the LCD, especially for radios without native transverter support. 
 * ========================================
*/

/*
Convert AUX command from N1MM to 4 bit BCD
Command format is AUXxnn fixed width. x is the radio number, usually 1.   Example is AUX103 for radio #1 with BCD output of 0x03.
*/
int OTRSP()
{     
    char c;
    int i;
    static byte AuxNum;
    char AuxCmd[20] = {};
    char AuxCmd0[20] = {};
    char AuxCmd1[20] = {}, AuxCmd2[20] = {};
    // char BndCmd1[BANDCMDLEN], BndCmd2[BANDCMDLEN];
    
    //AuxNum1 = AuxNum2 = 0;  // Global values also used to update status on LCD
    if (Serial.available() > 0)
    {
        AuxCmd[0] = Serial.read();       // Letter A
        if (AuxCmd[0] == 'A')   // AUXxYYy\r
        {       
            // Got A, keep going               
            AuxCmd[1] = Serial.read();  // Letter U
            AuxCmd[2] = Serial.read();  // Letter X                
            // Looking only for 2 commands, BAND and AUX.  
            if (strncmp(AuxCmd,"AUX", 3) == 0)   // process AUX1 where 1 is the radio number.  Each radio has a 4 bit BCD value
            {     
                // Got an AUX commands, store rest of fixed length message
                AuxCmd[3] = Serial.read();  // Number 1 or 2 for radio 1 or radio 2
                AuxCmd[4] = Serial.read();  // 1st of 2 digits for ASCII value 0-15
                AuxCmd[5] = Serial.read();  // 2nd of 2 digits for ACSII value 0-15.  Convert to a number later
                AuxCmd[6] = Serial.read();  // get the \r for validation
                AuxCmd[7] = '\0';           // null terminate the string

                if (AuxCmd[6] == '\r') // if we find \r then we have a valid full command string, else bail
                {
                    // Got a valid formatted AUX message, no process it
                    AuxCmd[6] = '\0';  // replace the /r with NULL

                    AuxNum = atoi(&AuxCmd[4]);  // Convert the ASCII BCD value to a number 0-15
                    if (AuxNum > 15)
                    {
                        #ifdef DEBUG
                        Serial.println("Out of Range AUX field value");
                        #endif
                        return 0;  // Acceptable values are 0-15.  Do not want to errantly change outputs on bad input.
                    }
                    AuxNum &= 15;   // Ensure we only act on a value range of 0-15

                    if (AuxCmd[3] == '1')  // process AUX comand for radio 1
                    {                          
                        AuxNum1 = AuxNum;   // Value to send to Radio 1 BCD output pins
                        #ifdef DEBUG                       
                        Serial.print("AUX1 = ");
                        Serial.println(AuxNum1);  
                        #endif

                        // ToDo: 
                        // This sample writes out the BCD value to 4 pins assigned to AUX1
                        // we need to either write out BCD as done here or convert to a parallel port format if desired with more GPIO pins
                        #ifdef DEBUG
                        Serial.println(AuxNum & 1);
                        #endif
                        digitalWrite(AUX1_0, AuxNum & 1);
                        AuxNum = AuxNum >> 1;
                        #ifdef DEBUG
                        Serial.println(AuxNum & 1);
                        #endif
                        digitalWrite(AUX1_1, AuxNum & 1);
                        AuxNum = AuxNum >> 1;
                        #ifdef DEBUG
                        Serial.println(AuxNum & 1);
                        #endif
                        digitalWrite(AUX1_2, AuxNum & 1);
                        AuxNum = AuxNum >> 1;
                        #ifdef DEBUG
                        Serial.println(AuxNum & 1);
                        #endif
                        digitalWrite(AUX1_3, AuxNum & 1);                       
                        
                        for (i=0; i < 20; i++)  // clear the buffer                             
                            AuxCmd[i] = '\0';                                                          
                        return 1;
                    }                  
                    else if (AuxCmd[3] == '2')   // process AUX comand for radio 2
                    {                           
                        AuxNum2 = AuxNum;  // Value to send to Radio 2 BCD output pins
                        #ifdef DEBUG
                        Serial.print("AUX2 = ");
                        Serial.println(AuxNum2);  // write the value for debug/info on the serial port
                        #endif
                        // ToDo: 
                        // This sample writes out the BCD value to 4 pins assigned to AUX2
                        // we need to either write out BCD as done here or convert to a parallel port format if desired with more GPIO pins
                        #ifdef DEBUG
                        Serial.println(AuxNum & 1);
                        #endif
                        digitalWrite(AUX2_0, AuxNum & 1);
                        AuxNum = AuxNum >> 1;
                        #ifdef DEBUG
                        Serial.println(AuxNum & 1);
                        #endif
                        digitalWrite(AUX2_1, AuxNum & 1);
                        AuxNum = AuxNum >> 1;
                        #ifdef DEBUG
                        Serial.println(AuxNum & 1);
                        #endif
                        digitalWrite(AUX2_2, AuxNum & 1);
                        AuxNum = AuxNum >> 1;
                        #ifdef DEBUG
                        Serial.println(AuxNum & 1);
                        #endif
                        digitalWrite(AUX2_3, AuxNum & 1);   
                        for (i=0; i< 20; i++)
                            AuxCmd[i] = '\0' ;
                        return 1; 
                    }
                }
            }
/*                
            // Code from the PSoC5 code - plan to delete this section if it is proven it is not useful or BAND is never used.  
            // It could be useful as a band change message that follows the radio exactly.

            // Look for BAND commands from N1MM - so far have not seen any - these are just for catching them, they cause no changes
            if (strncmp(AuxCmd,"BAND1",5) == 0)   // process BAND1 where 1 is the radio number.  Each radio has a 4 bit BCD value
            {
                // This will be the bottom frequency (in MHz) of the current radio band.  ie 3.5MHz for 3875KHz
                sprintf(BndCmd1,"%s", &AuxCmd[5]);
                AuxCmd[0] = '\0';
                return(0);  // TODO = passing band MHZ to a CouplerNUM  Search Band values
            }
            else if (strncmp(AuxCmd0,"BAND2",5) == 0)   // process AUX comand for radio 2.
            {
                sprintf(BndCmd2,"%s", &AuxCmd[5]);
                AuxCmd[0] = '\0';
                return(0); 
            }
*/          
            #ifdef DEBUG
            Serial.println("Invalid Message Decode");
            #endif
        }
    }
    return 0;   // nothing processed
}

/* [] END OF FILE */
