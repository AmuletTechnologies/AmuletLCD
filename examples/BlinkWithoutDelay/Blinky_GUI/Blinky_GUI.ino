/*  This example uses a blocking Arduino-as-master command to read a value from the 
 *  Amulet module's bank of "InternalRAM" memory. It is based upon the BlinkWithoutDelay 
 *  example, this code does two things in the main loop:
 *  First, blink LED at some interval
 *  Second, update that interval with a 16-bit value requested from the Amulet module.
 *  Value ranges from 50 to 1000ms
   
   To make sure Amulet module sets the InternalRAM variable this code requests, 
   place this command in a slider control widget Href in GEMstudio:  	
   Amulet:InternalRAM.word(0).setValue(intrinsicValue)

*/

#include <AmuletLCD.h>

#define VDP_SIZE 32
//Virtual Dual Port memory used for communicating with Amulet Display
uint16_t AmuletWords[VDP_SIZE]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

AmuletLCD myModule;

unsigned long interval = 1000;        // declare interval rate set to 1 sec to begin.
unsigned long intervalUpdate = 500;   // update the value of interval every 500ms

// constants won't change. Used here to set a pin number :
const int ledPin =  LED_BUILTIN;// the number of the LED pin

// Variables will change :
int ledState = LOW;             // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis1 = 0;        // will store last time LED was updated
unsigned long previousMillis2 = 0;        // will store last time interval was updated

void setup() {
  //start communication with Amulet Display at default baud
  myModule.begin(115200);
  //register our local buffer with Amulet state machine
  myModule.setWordPointer(AmuletWords, VDP_SIZE);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  unsigned long currentMillis = millis();
  
  //check if it is time to update the LED
  if (currentMillis - previousMillis1 >= interval) {
    // save the last time you blinked the LED
    previousMillis1 += interval;
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }
  
  //check if it is time to update the interval
  if (currentMillis - previousMillis2 >= intervalUpdate) {
    //save the last time you updated the interval.
    previousMillis2 = currentMillis;
    //update the interval by requesting the value from the Amulet module.
    myModule.requestWord(0);
    interval = myModule.getWord(0);
  }
}

//This method automatically gets called if there is any serial data available
//http://www.arduino.cc/en/Tutorial/SerialEvent
void serialEvent() {
  myModule.serialEvent();  //send any incoming data to the Amulet state machine
}


