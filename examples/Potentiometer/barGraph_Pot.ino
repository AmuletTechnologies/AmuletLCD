

#include <AmuletUART.h>


//initialize Virtual Dual Port RAM arrays:
// these are the memory registers that the UART is going to read and write through the Amulet protocol
byte myBytes[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
word myWords[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

const int analogPin = A0;   // the pin that the potentiometer is attached to

AmuletUART myModule(115200);  // initialize the AmuletUART module as a global

void setup()                    // run once, when the sketch starts
{
  Serial.begin(115200);  // setup Serial Connection
  myModule.setBytePointer(myBytes, sizeof(myBytes)); // Load memory address of RAM byte arrays
  myModule.setWordPointer(myWords, sizeof(myWords)); // Load memory address of RAM word arrays
}


void loop() {
    
     myModule.UART_recieve(); // Call the UART_recieve() function to begin slave protocol
     int sensorReading = analogRead(analogPin);
       if(Serial.availableForWrite() > 254) 
          myModule.setWord(0,sensorReading);
      
    
    
}



