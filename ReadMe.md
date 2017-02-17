# Arduino Amulet UART Communication Library v1.0 #
http://github.com/Amulettechnologies/
ReadMe file  
Brian Deters  Feb 2017


## Introduction ##

The Amulet UART communication library for Arduino simplifies the communication between Arduino and any of the Amulet display modules. Amulet has developed it's own CRC based full-duplex serial communication protocol.  A typical message packet looks like:

![](http://www.amulettechnologies.com/images/jdownloads/downloadimages/Protocol.jpg)


The library abstracts out the having to learn various opcodes, the complexity of packetizing the communication and calculation of CRC. With this library, Arduino just needs to assign certain Amulet defined variables, and the variables will be read by the Amulet display automatically.  A Serial.Event call is used to call the library, so when there is communication on the serial BUS, the library does its "magic". 

If you want to know in detail how the Amulet protocol works, you can look at the source in the library.  The code is well documented with comments, to make it easy to understand. 

"Arduino External EEPROM Library" is licensed under Lesser General Public License 
 [(LGPL Version 2.1)](http://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html).

## Installation ##
To use the **Arduino Amulet UART Communication Library**:  
- Go to http://github.com/AmuletTechnologies/AmuletLCD, click the **Download ZIP** button and save the ZIP file to a convenient location on your PC.
- Uncompress the downloaded file.  This will result in a folder containing all the files for the library, that has a name that includes the branch name, usually **AmuletLCD-master**.
- Rename the folder to just **AmuletLCD**.
- Copy the renamed folder into the libraries folder under your Arduino installation directory. 

## Examples ##
The following example Arduino sketches are included with the **Amulet communication library**:
###  Button_GUI  

Reads the state of a checkbox widget placed on the Amulet display and uses the state to turn on/off the onboard LED. The variable AmuletBytes[0] gets assigned by the Amulet display and gets communicated over the serial port to Aruino.  The Arduino just uses the value directly to turn on/off the led.  The following two lines are all that's needed in the main Loop of the Arduino code.

**value = AmuletBytes[0]; </br>
  digitalWrite(13, value);**

###  ReadPOT_GUI  

The values of a POT is read by Arduino using the analog pin 0 (A0) and this value is communicated to the Amulet display. The communication to the Amulet display is handled all by one line of code in the main Loop.

**AmuletWords[0] = analogRead(0);** </br> 


The AmuletWords variable is used rather than AmuletBytes because the POT value goes from 0 to 1023.


## GEMstudio Software ##
Amulet offers free software to program the Amulet modules. The software says it is a trial version, but for GUI projects under 5 pages, the software is full featured. All you need is to register.   [Free GEMstudio](http://www.amulettechnologies/index.php/sales/try-software).  The GEMstudio project files, Button_GUI.gemp and ReadPOT__GUI.gemp are included with this library under the folder called extras.  
