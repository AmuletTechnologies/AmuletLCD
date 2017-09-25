# Arduino Amulet UART Communication Library v1.1 #


[Complete library Help Files](https://amulettechnologies.github.io/AmuletLCD/)

## Introduction ##

The Amulet UART communication library for Arduino simplifies the communication between Arduino and any of the Amulet display modules. Amulet has developed it's own CRC based full-duplex serial communication protocol.  A typical message packet looks like:

![](http://www.amulettechnologies.com/images/jdownloads/downloadimages/Protocol.jpg)


The library abstracts out the having to learn various opcodes, the complexity of packetizing the communication and calculation of CRC. With this library, Arduino just needs to assign certain Amulet defined variables, and the variables will be read by the Amulet display automatically.  A serialEvent() call is used to update the state machine, so when there is communication on the serial BUS, the library does its "magic". 

If you want to know in detail how the Amulet protocol works, you can look at the source in the library.  The code is well documented with comments, to make it easy to understand. 

"Arduino Amulet UART Communication Library" is licensed under Lesser General Public License 
 [(LGPL Version 2.1)](http://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html).

## Installation ##
To use the **Arduino Amulet UART Communication Library**:  
- Go to http://github.com/AmuletTechnologies/AmuletLCD, click the **Download ZIP** button and save the ZIP file to a convenient location on your PC.
- Uncompress the downloaded file.  This will result in a folder containing all the files for the library, that has a name that includes the branch name, usually **AmuletLCD-master**.
- Rename the folder to just **AmuletLCD**.
- Copy the renamed folder into the libraries folder under your Arduino installation directory. 

## Examples ##
The GEMstudio project files for these examples can be found in the extras folder of the library.
The following examples are included with the **Amulet communication library**:

###  Blinky_GUI  - Arduino as Slave.

A slider GUI on the Amulet display is used to control the blink rate of the onboard LED of the Arduino Uno.  The display passes the value to variable, AmuletWords[0]. The range of values go from 0 to 500.  The Arduino updates AmuletWords[0] as the slider changes.

    void loop() {
		interval = AmuletWords[0];		//slider value from display 
		digitalWrite(13, HIGH);  		// set the LED on
		delay(interval);              	// wait for interval sec.
		digitalWrite(13, LOW);    		// set the LED off
		delay(interval);              	// wait for interval sec.
	}
  
###  Button_GUI  - Arduino as Slave.

A check box GUI on the Amulet display in the form of an on/off switch controls the state of the onboard LED of the Arduino. The byte value, either 0x00 (off) or 0x01 (on) gets communicated to Aduino, within the variable, AmuletBytes[0]. That same byte gets read back by an ImageSequence widget on the Amulet display to mirror the output of the Arduino's onboard LED.

    void loop() {
       	value = AmuletBytes[0];
    	digitalWrite(13, value);
    } 
  

###  ReadPOT_GUI  - Arduino as Slave.

The values of a POT is read by Arduino using the analog pin 0 (A0) and this value is communicated to the Amulet display by the assignment of AmultWords[0]. 


    void loop() {
       	AmuletWords[0] = analogRead(0);
    }

The return value of analogRead ranges from 0 to 1023. This is reflected in the min and max parameters of the Bargraph Widget in the corresponding GEMstudio demo.

###  BlinkWithoutDelay  - Arduino as Master.

The interval which the onboard LED blinks is determined by the value of an InternalRAM word variable. This is similar to Blinky_GUI, except that in this case, the Arduino is the master so it will request the variable from the Amulet module and wait for a response. This uses the stock BlinkWithoutDelay example, adding a second task in the main loop. The first task blinks the LED as some interval. The second task updates that interval with the value returned from the Amulet module.

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

## GEMstudio Software ##
Amulet offers free software to program the Amulet modules. The software says it is a trial version, but is fully featured for GUI projects under 5 pages. You just need to register on the website.   [Free GEMstudio](http://www.amulettechnologies/index.php/sales/try-software).  
