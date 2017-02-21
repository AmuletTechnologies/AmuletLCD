# Arduino Amulet UART Communication Library v1.0 #
http://github.com/Amulettechnologies/
ReadMe file  
Brian Deters  Feb 2017


## Introduction ##

The Amulet UART communication library for Arduino simplifies the communication between Arduino and any of the Amulet display modules. Amulet has developed it's own CRC based full-duplex serial communication protocol.  A typical message packet looks like:

![](http://www.amulettechnologies.com/images/jdownloads/downloadimages/Protocol.jpg)


The library abstracts out the having to learn various opcodes, the complexity of packetizing the communication and calculation of CRC. With this library, Arduino just needs to assign certain Amulet defined variables, and the variables will be read by the Amulet display automatically.  A Serial.Event call is used to call the library, so when there is communication on the serial BUS, the library does its "magic". 

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
The following examples are included with the **Amulet communication library**:
###  Blinky_GUI  

A slider GUI on the Amulet display is used to control the blink rate of the onboard LED of the Arduino Uno.  The display passes the value to variable, AmuletWords[0]. The range of values go from 0 to 500.  The Arduino updates AmuletWords[0] as the slider changes.

    void loop() {
		interval = AmuletWords[0];		//slider value from display 
		digitalWrite(13, HIGH);  		// set the LED on
		delay(interval);              	// wait for interval sec.
		digitalWrite(13, LOW);    		// set the LED off
		delay(interval);              	// wait for interval sec.
	}
  
###  Button_GUI  

A check box GUI on the Amulet display in the form of an on/off switch controls the state of the onboard LED of the Arduino. The byte value, either 0x00 (off) or 0x01 (on) gets communicated to Aduino, within the variable, AmuletBytes[0]. 

    void loop() {
       	value = AmuletBytes[0];
    	digitalWrite(13, value);
      	delay(100);
    } 
  

###  ReadPOT_GUI  

The values of a POT is read by Arduino using the analog pin 0 (A0) and this value is communicated to the Amulet display by the assignment of AmultWords[0]. 


    void loop() {
       	AmuletWords[0] = analogRead(0);
     	 delay(100);
    }

The AmuletWords variable is used rather than AmuletBytes because the POT value goes from 0 to 1023.


## GEMstudio Software ##
Amulet offers free software to program the Amulet modules. The software says it is a trial version, but for GUI projects under 5 pages, the software is full featured. You just need to register on their website.   [Free GEMstudio](http://www.amulettechnologies/index.php/sales/try-software).  The GEMstudio project files for the 3 examples can be found in the extras folder of the library.
