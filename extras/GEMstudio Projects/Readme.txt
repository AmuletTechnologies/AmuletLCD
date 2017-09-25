Arduino as Master means your Arduino requests data from the Amulet module. This entails sending out a serial command and then waiting for a reply. This library will BLOCK all actions until the request is either responded to or times out an arbitrary number of times.


Arduino as Slave means your Arduino responds to requests sent by the Amulet module over the serial port. The Amulet state machine is polled in your code with the serialEvent() function. The local variables you use in the loop() need to be updated by the Amulet module using a command specified from GEMstudio, for example on a button or slider, in the form: 
Amulet:port.dataType(index).setValue(value), where: 
port can be uart0, uart1, uart2 (depending on your hardware), or usb, whichever you use to connect to your Arduino.
dataType is byte, word, or color (strings not yet supported in this library)
index is the offset into the array defined by setBytePointer, setWordPointer, or setColorPointer
value is the 8- 16- or 32-bit value to set.


Both Master and Slave scenarios can be used at the same time, as long as the serialEvent() method is called frequently enough.