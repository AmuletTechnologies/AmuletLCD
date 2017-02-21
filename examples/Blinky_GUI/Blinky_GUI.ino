/* Read a slider value from the LCD GUI.  Values range is 100 to 2000  (.1sec to 2 sec) 
*  Arduino will read this value and use it to determine the blink rate
*  To make Amulet request the send the variable, place this command in slider control widget 
*  Href in GEmstudio:  	Amulet:uart1.word(0).setValue(intrinsicValue)

*/

#include <AmuletLCD.h>

#define VDP_SIZE 32
//Virtual Dual Port memory used for communicating with Amulet Display 
uint16_t AmuletWords[VDP_SIZE]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

AmuletLCD myModule;

unsigned long interval = 100;        // declare interval rate set to 1 sec.

void setup() {
  //start communication with Amulet Display at default baud
  myModule.begin(115200);
  //register our local buffer with Amulet state machine
  myModule.setWordPointer(AmuletWords,VDP_SIZE);
  pinMode(13, OUTPUT);  
}

void loop() {
  //store analog data in local buffer and wait for Amulet to poll that buffer.
    interval = AmuletWords[0];
	digitalWrite(13, HIGH);  		// set the LED on
	delay(interval);              	// wait for interval sec.
	digitalWrite(13, LOW);    		// set the LED off
	delay(interval);              	// wait for interval sec.
  
}

//This method automatically gets called if there is any serial data available
//http://www.arduino.cc/en/Tutorial/SerialEvent
void serialEvent() {
    myModule.serialEvent();  //send any incoming data to the Amulet state machine
}


