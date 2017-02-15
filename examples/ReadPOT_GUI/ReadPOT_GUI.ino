
/* Read a Potentiometer on Analog input 0 and store in local buffer waiting for Amulet to request the data.
*  To make Amulet request the proper variable, place this command in a view widget's (bargraph, numericField, etc) Href in GEmstudio:
*  Amulet:uart1.word(0).value()
* For other ports, change uart1 to your selected port.
* 
* The GEMstudio project file is located in the AmuletLCD library directory under folder called "extras". Examples of numeric field, 
* bargraph and image sequence are shown.

#include <AmuletLCD.h>

#define VDP_SIZE 32
//Virtual Dual Port memory used for communicating with Amulet Display 
uint16_t AmuletWords[VDP_SIZE]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

AmuletLCD myModule;

void setup() {
  //start communication with Amulet Display at default baud
  myModule.begin(115200);
  //register our local buffer with Amulet state machine
  myModule.setWordPointer(AmuletWords,VDP_SIZE);
}

void loop() {
  //store analog data in local buffer and wait for Amulet to poll that buffer.
  AmuletWords[0] = analogRead(0);
  delay(100);
}

//This method automatically gets called if there is any serial data available
//http://www.arduino.cc/en/Tutorial/SerialEvent
void serialEvent() {
    myModule.serialEvent();  //send any incoming data to the Amulet state machine
}
