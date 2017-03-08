/*
  Amulet.h - Library for communicating to GEMmodule over UART.
  Created by Brian Deters, February 13, 2017.
  Released into the public domain.
*/

#include "Arduino.h"
#include "AmuletLCD.h"

/**
* constructor
*/

AmuletLCD::AmuletLCD()
{
	_baud = 115200;      //default baud;
	_UART_State = 0;
	_BytesLength = 0;
	_WordsLength = 0;
	_ColorsLength = 0;
}

/**
* Start communication at specified baud rate
* @param baud uint32_t the communications rate specified in bits per second, default 115200
*/
void AmuletLCD::begin(uint32_t baud)
{
	_baud = baud;
	Serial.begin(baud);           // set up Serial library
}

/**
* Set up array for use with Amulet commands: Amulet:UARTn.byte(x).value()/setValue()
* @param ptr uint8_t* The memory used for the virtual dual port Byte array.
* @param ptrSize uint16_t The length of the local Byte array. Amulet InternalRAM max index is 255
*/
void AmuletLCD::setBytePointer(uint8_t * ptr, uint16_t ptrSize)
{
	_Bytes = ptr;
	_BytesLength = ptrSize;	
}

/**
* Set up array for use with Amulet commands: Amulet:UARTn.word(x).value()/setValue()
* @param ptr uint16_t* The memory used for the virtual dual port Word array.
* @param ptrSize uint16_t The length of the local word array. Amulet InternalRAM max index is 255
*/
void AmuletLCD::setWordPointer(uint16_t * ptr, uint16_t ptrSize)
{
	_Words = ptr;
	_WordsLength = ptrSize;
}

/**
* Set up array for use with Amulet commands: Amulet:UARTn.color(x).value()/setValue()
* @param ptr uint32_t* The memory used for the virtual dual port Word array.
* @param ptrSize uint16_t The length of the local color array. Amulet InternalRAM max index is 255
*/
void AmuletLCD::setColorPointer(uint32_t * ptr, uint16_t ptrSize)
{
	_Colors = ptr;
	_ColorsLength = ptrSize;
}

/**
* Read the Byte from the local array, which may or may not match the state of Amulet InternalRAM.Byte memory
* Expecting the Amulet Display to send a master command to set this value, so we dont block with a UART request.
* @param loc uint8_t the index into the local array
* @return uint8_t the indexed value of local buffer if loc < _BytesLength. Otherwise, 0;
*/
uint8_t AmuletLCD::getByte(uint8_t loc)
{
	if (loc < _BytesLength)
		return _Bytes[loc];
	else
		return 0;
}

/**
* Send out a serial command to set the Byte in the Amulet InternalRAM.Byte memory
* @param loc uint8_t the index into the local array
* @param value uint8_t the index into the local array
* @return int8_t On error, -1. No error, returns 0
*/
int8_t AmuletLCD::setByte(uint8_t loc, uint8_t value)
{
	if(Serial.availableForWrite() >= 6)
	{
		uint8_t setByteCommand[6] = {_AMULET_ADDRESS, _SET_BYTE, loc, value, 0, 0};
		uint16_t CRC = calcCRC(setByteCommand, 4);  //TODO: make a function to calculate AND append the CRC to the buffer
		setByteCommand[4] = CRC & 0xFF;
		setByteCommand[5] = (CRC >> 8) & 0xFF;
		Serial.write(setByteCommand,6);
		return 0;
		// TODO: set up response state machine
	}
	else
		return -1;
}

/**
* Read the Word from the local array, which may or may not match the state of Amulet InternalRAM.Word memory
* Expecting the Amulet Display to send a master command to set this value, so we dont block with a UART request.
* @param loc uint8_t the index into the local array
* @return uint16_t the indexed value of local buffer, if loc < _BytesLength. Otherwise, 0;
*/
uint16_t AmuletLCD::getWord(uint8_t loc)
{
	if (loc < _WordsLength)
		return _Words[loc];
	else
		return 0;
}

/**
* Send out a serial command to set the Word in the Amulet InternalRAM.Word memory
* @param loc uint8_t the index into the local _Words array
* @param value uint16_t the value to set _Words[loc] to.
* @return int8_t On error, -1. No error, returns 0
*/
int8_t AmuletLCD::setWord(uint8_t loc, uint16_t value)
{
	if(Serial.availableForWrite() >= 7)
	{
		uint8_t setWordCommand[7] = {_AMULET_ADDRESS,_SET_WORD,loc, uint8_t((value >> 8) & 0xFF), uint8_t(value & 0xFF), 0, 0};
		uint16_t CRC = calcCRC(setWordCommand, 5);  //TODO: make a function to calculate AND append the CRC to the buffer
		setWordCommand[5] = (uint8_t)(CRC & 0xFF);
		setWordCommand[6] = (uint8_t)((CRC >> 8) & 0xFF);
		Serial.write(setWordCommand,7);
		return 0;
		// TODO: set up response state machine
	}
	else
		return -1;
}

/**
* Utility function to calculate the MODBUS CRC of the given array.
* @param ptr uint8_t* the array to calculate
* @param count uint16_t the length of the array
* @return uint16_t The calculated CRC value.
*/
uint16_t AmuletLCD::calcCRC(uint8_t *ptr, uint16_t count)
{
   word crc = _CRC_SEED;    // initialize CRC
   uint16_t i;
   while (count-- > 0){
       crc = crc ^ *ptr++;
       for (i=8; i>0; i--){
           if (crc & 0x0001){
               crc = (crc >> 1) ^ _CRC_POLY;
           }
           else{
               crc >>= 1;
           }
       }
   }
   return crc;
}

/**
* Heartbeat function that manages the state machine with incoming serial data. This should be called periodically
* in your main loop or your own serialEvent() if supported by your board.
* https://www.arduino.cc/en/Tutorial/SerialEvent
* If arrays longer than the serial buffer are requested and you have blocking code, then consider peppering this command within your block
* Alternatively, call UART_State_Machine from a real UART interrupt 
*(advanced topic, additional libraries may be required, plus you need to make UART_State_Machine public)
*/
void AmuletLCD::serialEvent()
{
	while(Serial.available() > 0)
	{
		CRC_State_Machine(Serial.read());
  	}
}

/**
* Main state machine of the Amulet CRC protocol handler.
* @param b uint8_t the next serial byte to process.
*/
void AmuletLCD::CRC_State_Machine(uint8_t b){  
  //TODO: implement state "waiting for response from master command" , implement Array commands, Fix Set replies to new format.
  static uint16_t i;     //remaining bytes before CRC for known length commands (non-string)
  static int16_t count; //count of bytes left
  switch(_UART_State){
    case _RECIEVE_BEGIN:   //begin - look for a valid address
      if(b == _HOST_ADDRESS){
        _UART_State = _PARSE_OPCODE;
        _RxBuffer[_RxBufferLength++] = b;
        i = 1;
      }
      else{} //stay in this state
      break;
    case _PARSE_OPCODE:               //parse opcode to determine next state
      count = recieve_OpcodeParser(b);    
      if(count == -1){               //invalid opcode, reset state machine
        _RxBufferLength = 0;
        _UART_State = _RECIEVE_BEGIN;
      }
      else if(count == 0){           //variable length array or string
        _RxBuffer[_RxBufferLength++] = b;
        if(b == _SET_STRING){
          _UART_State = _VARIABLE_LENGTH_STRING;
        }
        else{
          _UART_State = _VARIABLE_LENGTH_ARRAY;
        }      
      }
      else if(count > 0){            //static length command
        _RxBuffer[_RxBufferLength++] = b;
        _UART_State = _STATIC_LENGTH;
      }   
      break;
    case _STATIC_LENGTH:              //fixed length command. increment i until count bytes recieved, then get CRC
        if(i < count){
          _RxBuffer[_RxBufferLength++] = b;
          i++;
        }
        else{
          _RxBuffer[_RxBufferLength++] = b;
          _UART_State = _GET_CRC1;
        }
      break;
    case _VARIABLE_LENGTH_ARRAY:  //set array command, next byte contains starting address
      _RxBuffer[_RxBufferLength++] = b;
      _UART_State = _ARRAY_START;
      break;
    case _VARIABLE_LENGTH_STRING:
      if(b != 0x00 || count == 0){
        _RxBuffer[_RxBufferLength++] = b;
        count++;
      }
      else{
        _RxBuffer[_RxBufferLength++] = b;
        count++;
        _UART_State = _GET_CRC1;
      }
      break;
    case _ARRAY_START:
      _RxBuffer[_RxBufferLength++] = b;
      switch(_RxBuffer[1]){              //calc # of bytes before CRC
        case _SET_BYTE_ARRAY:
          count = b;
          break;
        case _SET_WORD_ARRAY:
          count = 2 * b;
          break;
        case _SET_COLOR_ARRAY:
          count = 4 * b;
          break;
      }
      if(count > 0){
        _UART_State = _ARRAY_DATA;
      }
      else{
        _UART_State = _GET_CRC1;
      }
      break;
    case _ARRAY_DATA:
      if(i < count){
        _RxBuffer[_RxBufferLength++] = b;
        i++;
      }
      else{
        _RxBuffer[_RxBufferLength++] = b;
        _UART_State = _GET_CRC1;
      }    
      break;
    case _GET_CRC1:
      _RxBuffer[_RxBufferLength++] = b;
      _UART_State = _GET_CRC2;
      break;
    case _GET_CRC2:
      _RxBuffer[_RxBufferLength++] = b;
      _UART_State = _RECIEVE_BEGIN;
      processUARTCommand(_RxBuffer,_RxBufferLength);
	  _RxBufferLength = 0;
      break;
    default:
      _UART_State = 0;
      _RxBufferLength = 0;
      i = 0;
  }  
}

/**
* Utility function to caluculate the length of a command based upon the opcode.
* Array command length can be caluclated after getting the count, and string commands look for a Null.
* @param b uint8_t The opcode
* @return uint8_t The number of bytes left before the CRC
*/
int8_t AmuletLCD::recieve_OpcodeParser(uint8_t b){
  
  // start with fixed length functions (full length known from just the opcode)
  // getbyte     getword    getstring  getcolor   getLabel    invokeRPC
  if(b==_GET_BYTE || b==_GET_WORD || b==_GET_STRING || b==_GET_COLOR || b==_GET_LABEL|  b==_INVOKE_RPC){ 
    return 1;
  }
  else if(b==_SET_BYTE || b==_GET_BYTE_ARRAY ||  b==_GET_WORD_ARRAY || b==_GET_COLOR_ARRAY){ //set: Byte. getarray: byte, word, color.
    return 2;
  }
  else if(b==_SET_WORD){
    return 3;
  }
  else if(b==_SET_COLOR){
    return 5;
  }
  else if(b==_SET_STRING || b==_SET_BYTE_ARRAY || b==_SET_WORD_ARRAY || b==_SET_COLOR_ARRAY){  // variable length array
    return 0;
  }
  else {   //invalid opcode. 
    return -1; 
  }
}



/**
* Utility function to confirm the CRC is valid.
* Split the buffer into two parts: 
*  1. Everyting up to but not including the last two bytes
*  2. The last two bytes
* Calculate the CRC on #1 and make sure it matches #2
* #2 is little endian, so swap the bytes
* @param buf uint8_t* The buffer containing the message
* @param bufLen uint16_t The length of the command in the buffer
* @return boolean True if the calculated CRC matches the one in the message. Otherwise False.
*/
boolean AmuletLCD::checkCRC(uint8_t *buf, uint16_t bufLen){
  uint16_t messageCRC = ((uint16_t)buf[bufLen-1] << 8) + (uint16_t)buf[bufLen-2];
  uint16_t calculatedCRC = calcCRC(buf,bufLen-2);
  if(messageCRC == calculatedCRC){
    return true;
  }
  else{
    return false;
  }
}

/**
* Once a master command has been received, the proper data structure needs to be set/retrieved and the proper response sent back.
* @param buf uint8_t* The received command
* @param bufLen uint16_t The length of the command in the buffer
*/
void AmuletLCD::processUARTCommand(uint8_t *buf, uint16_t bufLen){
  uint8_t _TxBufferLength = 0;
  uint16_t returnCRC = 0;
  uint8_t i = 0;
  _TxBuffer[0] = _HOST_ADDRESS; //all slave commands start with Host ID, then Opcode
  _TxBuffer[1] =  buf[1]; //the reply opcode is the same as the initial command
  
  //TODO: start by checking if its a master command or slave response. For slave response, if CRC, opcode, or payload dont match, we need to send again.
  
  if(checkCRC(buf,bufLen)){ //For Recieving a Master command, first verify the CRC is good.
    switch(buf[1]){
      case _GET_BYTE:
        //_TxBuffer[0] = _HOST_ADDRESS;  //already set above, put here for clarity
        //_TxBuffer[1] = _GET_BYTE;      //already set above, put here for clarity
        _TxBuffer[2] = buf[2];
        _TxBuffer[3] = _Bytes[buf[2]];
        returnCRC= calcCRC(_TxBuffer,4);
        _TxBuffer[4] = returnCRC & 0xFF;
        _TxBuffer[5] = (returnCRC >> 8) & 0xFF;
		Serial.write(_TxBuffer,6);
        break;
      case _GET_WORD:
        //_TxBuffer[0] = _HOST_ADDRESS;  //already set above, put here for clarity
        //_TxBuffer[1] = _GET_WORD;      //already set above, put here for clarity
        _TxBuffer[2] = buf[2];
        _TxBuffer[3] = (_Words[buf[2]] >> 8) & 0xFF;  //MSB first for data
        _TxBuffer[4] = _Words[buf[2]] & 0xFF;
        returnCRC= calcCRC(_TxBuffer,5);
        _TxBuffer[5] = returnCRC & 0xFF;             //LSB first for CRC
        _TxBuffer[6] = (returnCRC >> 8) & 0xFF;
		Serial.write(_TxBuffer,7);
        break;
		
      case _GET_STRING:
	  /*  STRINGS commented out for now. Want to rework from multi- to single-dimentional array to match Amulet InternalRAM structure.
        _TxBuffer[0] = _HOST_ADDRESS;
        _TxBuffer[1] = _GET_STRING;
        _TxBuffer[2] = buf[2];
        _TxBufferLength = 3;
        while(_Strings[buf[2]][i] != 0x00){               //append UTF-8 string
          _TxBuffer[_TxBufferLength++] = Strings[buf[2]][i];
          i++;
        }
        _TxBuffer[_TxBufferLength++] = 0x00; //append  NULL terminator
        returnCRC= calcCRC(_TxBuffer,_TxBufferLength);
        _TxBuffer[_TxBufferLength++] = returnCRC & 0xFF;
        _TxBuffer[_TxBufferLength++] = (returnCRC >> 8) & 0xFF;
 	    Serial.write(_TxBuffer,_TxBufferLength);
        break;
		*/
		//Just reply with blank string for now
		_TxBuffer[2] = buf[2];
		_TxBuffer[3] = 0;
		_TxBufferLength = 4;
		returnCRC= calcCRC(_TxBuffer,_TxBufferLength);
        _TxBuffer[_TxBufferLength++] = returnCRC & 0xFF;
        _TxBuffer[_TxBufferLength++] = (returnCRC >> 8) & 0xFF;
		Serial.write(_TxBuffer,_TxBufferLength);
        break;
      case _GET_COLOR:
        //_TxBuffer[0] = _HOST_ADDRESS;  //already set above, put here for clarity
        //_TxBuffer[1] = _GET_COLOR;     //already set above, put here for clarity
        _TxBuffer[2] = buf[2];
        _TxBuffer[3] = (_Colors[buf[2]] >> 24) & 0xFF;  //MSB first for data
        _TxBuffer[4] = (_Colors[buf[2]] >> 16) & 0xFF;
        _TxBuffer[5] = (_Colors[buf[2]] >>  8) & 0xFF;
        _TxBuffer[6] = _Colors[buf[2]] & 0xFF;
        returnCRC= calcCRC(_TxBuffer,7);
        _TxBuffer[7] = returnCRC & 0xFF;             //LSB first for CRC
        _TxBuffer[8] = (returnCRC >> 8) & 0xFF;
        Serial.write(_TxBuffer,9);
        
        break;
      case _GET_BYTE_ARRAY:
        //TODO: Implement _GET_BYTE_ARRAY
        break;
      case _GET_WORD_ARRAY:
        //TODO: Implement _GET_WORD_ARRAY
        break;
      case _GET_COLOR_ARRAY:
        //TODO: Implement _GET_COLOR_ARRAY
        break;
      case _SET_BYTE:
        _Bytes[buf[2]] = buf[3];
        SetCmd_Reply(_SET_BYTE);
        break;
      case _SET_WORD:
        _Words[buf[2]] = word(buf[3],buf[4]);
        SetCmd_Reply(_SET_WORD);
        break;
      case _SET_STRING:
        //TODO: need to actually write string to memory, see getString comments above
        SetCmd_Reply(_SET_STRING);
        break;
      case _SET_COLOR:
        _Colors[buf[2]] = ((long(buf[3]) << 24) | (long(buf[4]) << 16) | (buf[5] << 8) | buf[6]);        
        SetCmd_Reply(_SET_COLOR);
        break;
      case _SET_BYTE_ARRAY:
              //TODO: need to actually write array to memory
        SetCmd_Reply(_SET_BYTE_ARRAY);
        break;
      case _SET_WORD_ARRAY:
              //TODO: need to actually write array to memory
        SetCmd_Reply(_SET_WORD_ARRAY);
        break;
      case _SET_COLOR_ARRAY:
              //TODO: need to actually write array to memory
        SetCmd_Reply(_SET_COLOR_ARRAY);
        break;
      case _INVOKE_RPC:
        //TODO: write RPC handler here
        SetCmd_Reply(_INVOKE_RPC);
        break;
    }
  }
  //else for Receive master command - CRC mismatch: do nothing. Amulet will resend after timeout
}

/**
* Send reply to a Set command, which all have a similar structure
* @param OPCODE uint8_t The type of message that was received
*/
void AmuletLCD::SetCmd_Reply(uint8_t OPCODE){
  uint8_t buffer[4];
  buffer[0] = _HOST_ADDRESS;
  buffer[1] = OPCODE;
  uint16_t returnCRC = calcCRC(buffer,2);
  buffer[2] = returnCRC & 0xFF;
  buffer[3] = (returnCRC >> 8) & 0xFF;
  Serial.write(buffer,4);
}
