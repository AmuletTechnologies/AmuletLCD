#include <AmuletLCD.h>



/* Amulet display sends command to Arduino to store the checkbox state (0x00 or 0x01) at byte address location 0.
* To send the correct variable to the Arduino, place this command in a checkbox widget's Href in GEMstudio;
* Amulet:uart1.byte(0).setValue(intrinsicValue)
*
* The GEMstudio project file is located in the AmuletLCD library directory under folder called "extras"
*/


#define VDP_SIZE 32
//Virtual Dual Port memory used for communicating with Amulet Display 
uint8_t AmuletBytes[VDP_SIZE]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

AmuletLCD myModule;
byte value;
void setup() {
	//Arduino Uno On Board LED defined on Pin 13
  pinMode(13, OUTPUT);
  //start communication with Amulet Display at default baud         
  myModule.begin(115200);
 
  //register our local buffer with Amulet state machine
  myModule.setBytePointer(AmuletBytes,VDP_SIZE); 
}

void loop() {
    value = AmuletBytes[0];
	  digitalWrite(13, value);
  	delay(100);         
  }

//This method automatically gets called if there is any serial data available
//http://www.arduino.cc/en/Tutorial/SerialEvent
void serialEvent() {
    myModule.serialEvent();  //send any incoming data to the Amulet state machine
}
