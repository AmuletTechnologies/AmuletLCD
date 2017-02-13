/*
  Amulet.h - Library for communicating to GEMmodule over UART.
  Created by Daniel M. Gopen, August, 26 2014.
  Released into the public domain.
*/

#include "Arduino.h"
// #include <avr/interrupt.h>
#include "AmuletUART.h"

AmuletUART::AmuletUART(int baud)
{
	_baud = 115200;		//default baud;
	_buffer[128];        //UART buffer on Amulet chip is actually 0x400 bytes.
	_bufferLength;
	_UART_State = 0;
	_BytesLength = 0;
}

void AmuletUART::begin(int baud)
{
	_baud = baud;
	Serial.begin(baud);           // set up Serial library
}

void AmuletUART::setBytePointer(byte * ptr, int ptrSize)
{
	_Bytes = ptr;
	_BytesLength = ptrSize;	
}

void AmuletUART::setWordPointer(word * ptr, int ptrSize)
{
	_Words = ptr;
	_WordsLength = ptrSize;
}

void AmuletUART::setColorPointer(unsigned long * ptr, int ptrSize)
{
	_Colors = ptr;
	_ColorsLength = ptrSize;
}

int AmuletUART::getByte(byte loc)
{
	// TODO: UART Master commands
	return 0;
}

int AmuletUART::setByte(byte loc, int value)
{
	// TODO: UART Master commands	
	return 0;
}

int AmuletUART::getWord(byte loc)
{
	// TODO: UART Master commands
	return 0;
}

int AmuletUART::setWord(byte loc, int value)
{
	if(Serial.availableForWrite() >= 7)
	{
		byte setWordCommand[7] = {_AMULET_ADDRESS,_SET_WORD,loc, byte((value >> 8) & 0xFF), byte(value & 0xFF), 0, 0};
		word CRC = calcCRC(setWordCommand, 5);  //TODO: make a function to calculate AND append the CRC to the buffer
		setWordCommand[5] = CRC & 0xFF;
		setWordCommand[6] = (CRC >> 8) & 0xFF;
		Serial.write(setWordCommand,7);
		return 0;
		// TODO: set up response state machine
	}
	else
		return -1;
}


word AmuletUART::calcCRC(byte *ptr, int count)
{
   word crc = _CRC_SEED;    // initialize CRC
   int i;
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

void AmuletUART::UART_recieve()
{
	while(Serial.available() > 0)
	{
		UART_State_Machine(Serial.read());
  	}
}

void AmuletUART::UART_State_Machine(byte b){  //TODO: implement state "waiting for response from master command" , implement Array commands, Fix Set replies to new format.
  static int i;
  static int index; //first index of data
  static int count; //count of bytes left
  switch(_UART_State){
    case _RECIEVE_BEGIN:   //begin - look for a valid address
      if(b == _HOST_ADDRESS){
        _UART_State = _PARSE_OPCODE;
        _buffer[_bufferLength++] = b;
        i = 1;
      }
      else{} //stay in this state
      break;
    case _PARSE_OPCODE:               //parse opcode to determine next state
      count = recieve_OpcodeParser(b);    
      if(count == -1){               //invalid opcode, reset state machine
        _bufferLength = 0;
        _UART_State = _RECIEVE_BEGIN;
      }
      else if(count == 0){           //variable length array or string
        _buffer[_bufferLength++] = b;
        if(b == _SET_STRING){
          _UART_State = _VARIABLE_LENGTH_STRING;
        }
        else{
          _UART_State = _VARIABLE_LENGTH_ARRAY;
        }      
      }
      else if(count > 0){            //static length command
        _buffer[_bufferLength++] = b;
        _UART_State = _STATIC_LENGTH;
      }   
      break;
    case _STATIC_LENGTH:              //fixed length command. increment i until count btyes recieved, then get CRC
        if(i < count){
          _buffer[_bufferLength++] = b;
          i++;
        }
        else{
          _buffer[_bufferLength++] = b;
          _UART_State = _GET_CRC1;
        }
      break;
    case _VARIABLE_LENGTH_ARRAY:  //set array command, next byte contains starting address
      _buffer[_bufferLength++] = b;
      _UART_State = _ARRAY_START;
      break;
    case _VARIABLE_LENGTH_STRING:
      if(b != 0x00 || count == 0){
        _buffer[_bufferLength++] = b;
        count++;
//Serial.println("String");
      }
      else{
        _buffer[_bufferLength++] = b;
        count++;
        _UART_State = _GET_CRC1;
      }
    
      break;
    case _ARRAY_START:
      _buffer[_bufferLength++] = b;
      switch(_buffer[1]){              //calc # of bytes before CRC
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
//Serial.println(count);
      if(count > 0){
        _UART_State = _ARRAY_DATA;
      }
      else{
        _UART_State = _GET_CRC1;
      }
      break;
    case _ARRAY_DATA:
      if(i < count){
        _buffer[_bufferLength++] = b;
        i++;
      }
      else{
        _buffer[_bufferLength++] = b;
        _UART_State = _GET_CRC1;
      }    
      break;
    case _GET_CRC1:
      _buffer[_bufferLength++] = b;
      _UART_State = _GET_CRC2;
      break;
    case _GET_CRC2:
      _buffer[_bufferLength++] = b;
      _UART_State = _RECIEVE_BEGIN;
      processUARTCommand(_buffer,_bufferLength);
      _bufferLength = 0;
      break;
    default:
      _UART_State = 0;
      _bufferLength = 0;
      i = 0;
  }  
}

int AmuletUART::recieve_OpcodeParser(byte b){
  
  // start with fixed length functions (full length known from just the opcode)
  //getbyte     getword    getstring  getcolor   getLabel    invokeRPC
  if(b==_GET_BYTE || b==_GET_WORD || b==_GET_STRING || b==_GET_COLOR || b==_GET_LABEL|  b==_INVOKE_RPC){ 
    return 1;
  }
  else if(b==_SET_BYTE || b==_GET_BYTE_ARRAY ||  b==_GET_WORD_ARRAY || b==_GET_COLOR_ARRAY){ //set: Byte. getarray: byte, word, color.
    return 2;
  }
  else if(b==_SET_WORD){                                   //setWord 
    return 3;
  }
  else if(b==_SET_COLOR){                                   //setColor  
    return 5;
  }
  else if(b==_SET_STRING || b==_SET_BYTE_ARRAY || b==_SET_WORD_ARRAY || b==_SET_COLOR_ARRAY){  // variable length array
    return 0;
  }
  else {   //invalid opcode. 
    return -1; 
  }
}


/*
Method: checkCRC

functionality:
Split the buffer into two parts: 
  1. Everyting up to but not including the last two bytes
  2. The last two bytes
Calculate the CRC on #1 and make sure it matches #2
#2 is little endian, so I call word(h,l) to get it in an easier to use format.
*/
boolean AmuletUART::checkCRC(byte *buf, int bufLen){
  word messageCRC = word(buf[bufLen-1],buf[bufLen-2]);
  word calculatedCRC = calcCRC(buf,bufLen-2);
  if(messageCRC == calculatedCRC){
    return true;
  }
  else{
    return false;
  }
}

void AmuletUART::processUARTCommand(byte *buf, int bufLen){
  byte reply[128]; //
  byte replyLength = 0;
  word returnCRC = 0;
  byte i = 0;
 
  //TODO: start by checking if its a master command or slave response. For slave response, if CRC, opcode, or payload dont match, we need to send again.
  if(checkCRC(buf,bufLen)){ //For Recieving a Master command, first verify the CRC is good.
    switch(buf[1]){
      case _GET_BYTE:
        reply[0] = _HOST_ADDRESS;
        reply[1] = _GET_BYTE;
        reply[2] = buf[2];
        reply[3] = _Bytes[buf[2]];
        returnCRC= calcCRC(reply,4);
        reply[4] = returnCRC & 0xFF;
        reply[5] = (returnCRC >> 8) & 0xFF;
		Serial.write(reply,6);
		Serial.println("GET BYTE");
        break;
      case _GET_WORD:
        reply[0] = _HOST_ADDRESS;
        reply[1] = _GET_WORD;
        reply[2] = buf[2];
        reply[3] = (_Words[buf[2]] >> 8) & 0xFF;  //MSB first for data
        reply[4] = _Words[buf[2]] & 0xFF;
        returnCRC= calcCRC(reply,5);
        reply[5] = returnCRC & 0xFF;             //LSB first for CRC
        reply[6] = (returnCRC >> 8) & 0xFF;
		Serial.write(reply,7);
        break;
//       case _GET_STRING:
//         reply[0] = _HOST_ADDRESS;
//         reply[1] = _GET_STRING;
//         reply[2] = buf[2];
//         replyLength = 3;
//         while(_Strings[buf[2]][i] != 0x00){               //append UTF-8 null-terminated string
//           reply[replyLength++] = Strings[buf[2]][i];
//           i++;
//         }
//         reply[replyLength++] = 0x00;
//         returnCRC= calcCRC(reply,replyLength);
//         reply[replyLength++] = returnCRC & 0xFF;
//         reply[replyLength++] = (returnCRC >> 8) & 0xFF;
// Serial.write(reply,replyLength);
//         break;
      case _GET_COLOR:
        reply[0] = _HOST_ADDRESS;
        reply[1] = _GET_COLOR;
        reply[2] = buf[2];
        reply[3] = (_Colors[buf[2]] >> 24) & 0xFF;  //MSB first for data
        reply[4] = (_Colors[buf[2]] >> 16) & 0xFF;
        reply[5] = (_Colors[buf[2]] >>  8) & 0xFF;
        reply[6] = _Colors[buf[2]] & 0xFF;
        returnCRC= calcCRC(reply,7);
        reply[7] = returnCRC & 0xFF;             //LSB first for CRC
        reply[8] = (returnCRC >> 8) & 0xFF;
        Serial.write(reply,9);
        
        break;
      case _GET_BYTE_ARRAY:
//Serial.println("BYTE_ARRAY");
        break;
      case _GET_WORD_ARRAY:
//Serial.println("WORD_ARRAY");
        break;
      case _GET_COLOR_ARRAY:
//Serial.println("COLOR_ARRAY");
        break;
      case _SET_BYTE:
        _Bytes[buf[2]] = buf[3];
        SetCmdReply(_SET_BYTE);
        break;
      case _SET_WORD:
        _Words[buf[2]] = word(buf[3],buf[4]);
        SetCmdReply(_SET_WORD);
        break;
      case _SET_STRING:
        //TODO: need to actually write string to memory
         //     Serial.println("String"); //debug
        SetCmdReply(_SET_STRING);
        break;
      case _SET_COLOR:
        _Colors[buf[2]] = ((long(buf[3]) << 24) | (long(buf[4]) << 16) | (buf[5] << 8) | buf[6]);        
        SetCmdReply(_SET_COLOR);
        break;
      case _SET_BYTE_ARRAY:
              //TODO: need to actually write array to memory
        SetCmdReply(_SET_BYTE_ARRAY);
        break;
      case _SET_WORD_ARRAY:
              //TODO: need to actually write array to memory
        SetCmdReply(_SET_WORD_ARRAY);
        break;
      case _SET_COLOR_ARRAY:
              //TODO: need to actually write array to memory
        SetCmdReply(_SET_COLOR_ARRAY);
        break;
      case _INVOKE_RPC:
        //TODO: write RPC handler here
        SetCmdReply(_INVOKE_RPC);
        break;
    }
  }
  //else for Recieve master command - CRC mismatch: do nothing. Amulet will resend after timeout
}

//Since all Set commands have a similar reply, condense code into a function all here:
void AmuletUART::SetCmdReply(byte OPCODE){
  byte reply[4];
  reply[0] = _HOST_ADDRESS;
  reply[1] = OPCODE;
  word returnCRC = calcCRC(reply,2);
  reply[2] = returnCRC & 0xFF;
  reply[3] = (returnCRC >> 8) & 0xFF;
  Serial.write(reply,4);
  }
