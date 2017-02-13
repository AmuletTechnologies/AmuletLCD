#include <AmuletUART.h>

//initialize Virtual Dual Port RAM arrays:
// these are the memory registers that the UART is going to read and write through the Amulet protocol

word myWords[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

AmuletUART myModule(115200);  // initialize the AmuletUART module as a global

void setup()                    // run once, when the sketch starts
{
  Serial.begin(115200);  // setup Serial Connection
  pinMode(13, OUTPUT);
  myModule.setBytePointer(myBytes, sizeof(myBytes)); // Load memory address of RAM byte arrays
  myModule.setWordPointer(myWords, sizeof(myWords)); // Load memory address of RAM word arrays
}

void loop()
{
   if (Serial.available()){ 
      myModule.UART_recieve(); // Call the UART_recieve() function to begin slave protocol
      digitalWrite(13, myWords[13]);
         
      delay(1000);
   }
  }

