/*
  Amulet.h - Library for communicating to GEMmodule over UART.
  Created by Brian Deters, February 13, 2017.
  Released into the public domain.
*/

#include "Arduino.h"
#include "AmuletLCD.h"

/**
* Constructor. Initializes state machine variables
*/

AmuletLCD::AmuletLCD(){
	_baud = 115200;      //default baud;
	_UART_State = 0;
	_BytesLength = 0;
	_WordsLength = 0;
	_ColorsLength = 0;
	_errorCount = 0;
	_retries = 11;
	_Timeout_ms = 200;
	_config = SERIAL_8N1;
    _ea = 0;
}

/**
* Start communication at specified baud rate with default configuration (SERIAL_8N1)
* @param baud uint32_t the communications rate specified in bits per second, default 115200
*/
void AmuletLCD::begin(uint32_t baud){
	_baud = baud;
	Serial.begin(baud);           // set up Serial library
}

/**
* Start communication at specified baud rate and configuration
* See http://www.arduino.cc/en/Serial/Begin for valid configuration Macros.
* @param baud uint32_t the communications rate specified in bits per second, default 115200
* @param config uint8_t Macro that sets data, parity, and stop bits, default SERIAL_8N1
* @param extended_address uint8_t controls number of address bytes in messages. 0 for 1 address byte, nonzero for 2 bytes. set nonzero to support >256 byte, word, or color variables.
*/
void AmuletLCD::begin(uint32_t baud, uint8_t config, uint8_t extended_address){
	_baud = baud;
	_config = config;
    if (extended_address)
        _ea = 1; //only 1 and 0 are valid.
    else
        _ea = 0;
	#ifdef ESP8266
	Serial.begin(baud, (SerialConfig)config);
	#else
	Serial.begin(baud, config);
	#endif
}



/**
* Set up array for use with Amulet commands: Amulet:UARTn.byte(x).value()/setValue()
* @param ptr uint8_t* The memory used for the virtual dual port Byte array.
* @param ptrSize uint16_t The length of the local Byte array. Amulet InternalRAM max index is 255
*/
void AmuletLCD::setBytePointer(uint8_t * ptr, uint16_t ptrSize){
	_Bytes = ptr;
	_BytesLength = ptrSize;	
}

/**
* Set up array for use with Amulet commands: Amulet:UARTn.word(x).value()/setValue()
* @param ptr uint16_t* The memory used for the virtual dual port Word array.
* @param ptrSize uint16_t The length of the local word array. Amulet InternalRAM max index is 255
*/
void AmuletLCD::setWordPointer(uint16_t * ptr, uint16_t ptrSize){
	_Words = ptr;
	_WordsLength = ptrSize;
}

/**
* Set up array for use with Amulet commands: Amulet:UARTn.color(x).value()/setValue()
* @param ptr uint32_t* The memory used for the virtual dual port Word array.
* @param ptrSize uint16_t The length of the local color array. Amulet InternalRAM max index is 255
*/
void AmuletLCD::setColorPointer(uint32_t * ptr, uint16_t ptrSize){
	_Colors = ptr;
	_ColorsLength = ptrSize;
}

/**
* Set up memory for  function callbacks for use with Amulet commands: Amulet:UARTn.invokeRPC(index)
* @param ptr functionPointer * The array used to store the function addresses
* @param ptrSize uint16_t The length of the local array. Amulet RPC max index is 255
*/
void AmuletLCD::setRPCPointer(RPC_Entry * ptr, uint16_t ptrSize){
	_RPCs = ptr;
	_RPCsLength = ptrSize;
}

/**
* Set up a single function callback for use with Amulet commands: Amulet:UARTn.invokeRPC(index)
* @param index uint8_t The index to store the RPC function. Amulet RPC max index is 255
* @param function functionPointer The name of the function
*/
void AmuletLCD::registerRPC(uint8_t index, functionPointer function){
	if (index < _RPCsLength)
		_RPCs[index].function = function;
}

/**
* Calls a function callback. Used by Amulet commands: Amulet:UARTn.invokeRPC(index)
* @param index uint8_t The index where the RPC function is stored. Amulet RPC max index is 255
*/
void AmuletLCD::callRPC(uint8_t index){
	if (index < _RPCsLength)
		(_RPCs[index].function)();
}

/**
* Read the Byte from the local array, which may or may not match the state of Amulet InternalRAM.Byte memory
* Expecting either the Amulet Display to send a master command to set this value, or you can use requestByte or requestBytes to update the values before reading.
* @param loc uint16_t the index into the local array
* @return uint8_t the indexed value of local buffer if loc < _BytesLength. Otherwise, 0;
*/
uint8_t AmuletLCD::getByte(uint16_t loc){
	if (loc < _BytesLength)
		return _Bytes[loc];
	else{
		setError();
		return 0;
	}
}

/**
* Request the Byte from Amulet Display, and wait for a response.
* Copy the response into the local array at the same index.
* @param loc uint16_t the index into the Amulet and local array.
* @return int8_t true if correct response was received, false otherwise
*/
uint8_t AmuletLCD::requestByte(uint16_t loc)
{
	if(Serial.availableForWrite() >= 5+_ea){
		_GetByteReply = false;
        uint8_t command[6] = {_AMULET_ADDRESS, _GET_BYTE, 0, 0, 0, 0};
        uint8_t i;
        if (_ea) {
            command[2] = (uint8_t)(loc >> 8);
            i = 3;
        }
        else {
            i = 2;
        }            
        command[i++] = (uint8_t)(loc & 0xFF);
        appendCRC(command,i);
        i+=2;
		return send_command_blocking(command, i);
	}
	else{
		setError();
		return false;
	}
}

/**
* Request the Byte Array from Amulet Display, and wait for a response.
* Copy the response into the local array at the same indices.
* @param start uint16_t the first index into the Amulet and local array.
* @param count uint8_t the number of variables requested.
* @return int8_t true if correct response was received, false otherwise
*/
uint8_t AmuletLCD::requestBytes(uint16_t start, uint8_t count){
	if(Serial.availableForWrite() >= 6+_ea){
		_GetBytesReply = false;
        uint8_t command[7] = {_AMULET_ADDRESS, _GET_BYTE_ARRAY, 0, 0, 0, 0, 0};
        uint8_t i;
        if (_ea) {
            command[2] = (uint8_t)(start >> 8);
            i = 3;
        }
        else {
            i = 2;
        }            
        command[i++] = (uint8_t)(start & 0xFF);
        command[i++] = count;
        appendCRC(command,i);
        i+=2;
		return send_command_blocking(command, i);
	}
	else{
		setError();
		return false;
	}
}

/**
* Send out a serial command to set the Byte in the Amulet InternalRAM.Byte memory and wait for the response
* @param loc uint16_t the index into the Amulet Byte array
* @param value uint8_t the value to set
* @return int8_t true if correct response was received, false otherwise
*/
int8_t AmuletLCD::setByte(uint16_t loc, uint8_t value){
	setByte(loc, value, true);
}

/**
* Send out a serial command to set the Byte in the Amulet InternalRAM.Byte memory
* can optionally wait for response or just get out
* @param loc uint16_t the index into the Amulet Byte array
* @param value uint8_t the value to set
* @param waitForResponse uint8_t true will block until response is received or timeout occurs
* @return int8_t true if correct response was received or skipped, false otherwise
*/
int8_t AmuletLCD::setByte(uint16_t loc, uint8_t value, uint8_t waitForResponse){
    if(Serial.availableForWrite() >= 6+_ea){
        uint8_t command[7] = {_AMULET_ADDRESS, _SET_BYTE, 0, 0, 0, 0, 0};
        uint8_t i;
        if (_ea) {
            command[2] = (uint8_t)(loc >> 8);
            i = 3;
        }
        else {
            i = 2;
        }            
        command[i++] = (uint8_t)(loc & 0xFF);
        command[i++] = value;
        
        appendCRC(command,i);
        i+=2;
        if (waitForResponse){
            _SetByteReply = false;
            return send_command_blocking(command, i);
        }
        else{
            Serial.write(command,i);
            return true;
        }
    }
    else{
        setError();
        return false;
    }
}

/**
* Read the Word from the local array, which may or may not match the state of Amulet InternalRAM.Word memory
* Expecting either the Amulet Display to send a master command to set this value, or you can use requestWord or requestWords to update the values before reading.
* @param loc uint16_t the index into the local _Words array
* @return uint16_t the indexed value of local buffer, if loc < _WordsLength. Otherwise, 0;
*/
uint16_t AmuletLCD::getWord(uint16_t loc){
	if (loc < _WordsLength)
		return _Words[loc];
	else{
		setError();
		return 0;
	}
}

/**
* Request the Word from Amulet Display, and wait for a response.
* Copy the response into the local array at the same index.
* @param loc uint16_t the index into the Amulet and local array.
* @return int8_t true if correct response was received, false otherwise
*/
uint8_t AmuletLCD::requestWord(uint16_t loc){
	if(Serial.availableForWrite() >= 5+_ea){
		_GetWordReply = false;
        uint8_t command[6] = {_AMULET_ADDRESS, _GET_WORD, 0, 0, 0, 0};
        uint8_t i;
        if (_ea) {
            command[2] = (uint8_t)(loc >> 8);
            i = 3;
        }
        else {
            i = 2;
        }            
        command[i++] = (uint8_t)(loc & 0xFF);
        appendCRC(command,i);
        i+=2;
		return send_command_blocking(command, i);
	}
}

/**
* Request the Word Array from Amulet Display, and wait for a response.
* Copy the response into the local array at the same indices.
* @param start uint16_t the first index into the Amulet and local array.
* @param count uint8_t the number of variables requested.
* @return int8_t true if correct response was received, false otherwise
*/
uint8_t AmuletLCD::requestWords(uint16_t start, uint8_t count){
	if(Serial.availableForWrite() >= 6+_ea){
		_GetWordsReply = false;
        uint8_t command[7] = {_AMULET_ADDRESS, _GET_WORD_ARRAY, 0, 0, 0, 0, 0};
        uint8_t i;
        if (_ea) {
            command[2] = (uint8_t)(start >> 8);
            i = 3;
        }
        else {
            i = 2;
        }            
        command[i++] = (uint8_t)(start & 0xFF);
        command[i++] = count;
        
        appendCRC(command,i);
        i+=2;
		return send_command_blocking(command, i);
	}
}

/**
* Send out a serial command to set the Word in the Amulet InternalRAM.Word memory and wait for a response
* @param loc uint16_t the index into the Amulet word array
* @param value uint16_t the value to set
* @return int8_t true if response was received, false otherwise
*/
int8_t AmuletLCD::setWord(uint16_t loc, uint16_t value){
	return setWord(loc, value, true);
}


/**
* Send out a serial command to set the Word in the Amulet InternalRAM.Word memory
* can optionally wait for response or just get out
* @param loc uint16_t the index into the Amulet Word array
* @param value uint16_t the value to set
* @param waitForResponse uint8_t true will block until response is received or timeout occurs
* @return int8_t true if correct response was received or skipped, false otherwise
*/
int8_t AmuletLCD::setWord(uint16_t loc, uint16_t value, uint8_t waitForResponse){
	if(Serial.availableForWrite() >= 7+_ea){
		uint8_t command[8] = {_AMULET_ADDRESS,_SET_WORD,0,0,0,0,0,0};
        uint8_t i;
        if (_ea) {
            command[2] = (uint8_t)(loc >> 8);
            i = 3;
        }
        else {
            i = 2;
        }            
        command[i++] = (uint8_t)(loc & 0xFF);
        command[i++] = (uint8_t)(value >> 8);
        command[i++] = (uint8_t)(value & 0xFF);
        
		appendCRC(command,i);
        i+=2;
		if (waitForResponse){
			_SetWordReply = false;
			return send_command_blocking(command, i);
		}
		else{
			Serial.write(command,i);
			return true;
		}
	}
	else
	{
		setError();
		return false;
	}
}

/**
* Send out a serial command to set the Color in the Amulet InternalRAM.Color memory and wait for a response
* @param loc uint16_t the index into the Amulet color array
* @param value uint32_t the value to set
* @return int8_t true if correct response was received, false otherwise
*/
int8_t AmuletLCD::setColor(uint16_t loc, uint32_t value){
	return setColor(loc, value, true);
}

/**
* Send out a serial command to set the Color in the Amulet InternalRAM.Color memory
* can optionally wait for response or just get out
* @param loc uint16_t the index into the Amulet Color array
* @param value uint32_t the value to set
* @param waitForResponse uint8_t true will block until response is received or timeout occurs
* @return int8_t true if correct response was received or skipped, false otherwise
*/
int8_t AmuletLCD::setColor(uint16_t loc, uint32_t value, uint8_t waitForResponse){
	if(Serial.availableForWrite() >= 9+_ea){
        uint8_t command[10] = {_AMULET_ADDRESS,_SET_COLOR};
        uint8_t i;
        if (_ea) {
            command[2] = (uint8_t)(loc >> 8);
            i = 3;
        }
        else {
            i = 2;
        }            
        command[i++] = (uint8_t)(loc & 0xFF);
        command[i++] = (uint8_t) (value >> 24);
        command[i++] = (uint8_t)((value >> 16) & 0xFF);
        command[i++] = (uint8_t)((value >> 8)  & 0xFF);
        command[i++] = (uint8_t) (value        & 0xFF);
        
		appendCRC(command,i);
        i+=2;
		if (waitForResponse){
			_SetColorReply = false;
			return send_command_blocking(command, i);
		}
		else{
			Serial.write(command,i);
			return true;
		}
	}
	else{
		setError();
		return false;
	}
}


/**
* Read the Color from the local array, which may or may not match the state of Amulet InternalRAM.Color memory
* Expecting either the Amulet Display to send a master command to set this value, or you can use requestColor or requestColors to update the values before reading.
* @param loc uint16_t the index into the local Color array
* @return uint16_t the indexed value of local buffer, if loc < _ColorsLength. Otherwise, 0;
*/
uint32_t AmuletLCD::getColor(uint16_t loc){
	if (loc < _ColorsLength)
		return _Colors[loc];
	else{
		setError();
		return 0;
	}
}


/**
* Request the Color from Amulet Display, and wait for a response.
* Copy the response into the local array at the same index.
* @param loc uint16_t the index into the Amulet and local array.
* @return int8_t true if correct response was received, false otherwise
*/
uint8_t AmuletLCD::requestColor(uint16_t loc){
	if(Serial.availableForWrite() >= 5+_ea){
		_GetColorReply = false;
        uint8_t command[6] = {_AMULET_ADDRESS, _GET_COLOR, 0, 0, 0, 0};
        uint8_t i;
        if (_ea) {
            command[2] = (uint8_t)(loc >> 8);
            i = 3;
        }
        else {
            i = 2;
        }            
        command[i++] = (uint8_t)(loc & 0xFF);
        appendCRC(command,i);
        i+=2;
		return send_command_blocking(command, i);
	}
}


/**
* Request the Color Array from Amulet Display, and wait for a response.
* Copy the response into the local array at the same indices.
* @param start uint16_t the first index into the Amulet and local array.
* @param count uint8_t the number of variables requested.
* @return int8_t true if correct response was received, false otherwise
*/
uint8_t AmuletLCD::requestColors(uint16_t start, uint8_t count){
	if(Serial.availableForWrite() >= 6+_ea){
		_GetColorsReply = false;
        uint8_t command[7] = {_AMULET_ADDRESS, _GET_COLOR_ARRAY};
        uint8_t i;
        if (_ea) {
            command[2] = (uint8_t)(start >> 8);
            i = 3;
        }
        else {
            i = 2;
        }            
        command[i++] = (uint8_t)(start & 0xFF);
        command[i++] = count;
        appendCRC(command,i);
        i+=2;
		return send_command_blocking(command, i);
	}
}

/**
* Send out a serial command to set a String in the Amulet InternalRAM.String memory
* can optionally wait for response or just get out as soon as serial buffer populated
* @param start uint16_t the first index into the Amulet array.
* @param str const char * the source string
* @param waitForResponse uint8_t true will block until response is received or timeout occurs
* @return int8_t true if correct response was received, false otherwise
*/
int8_t AmuletLCD::setString(uint16_t loc, const char * str, uint8_t waitForResponse){
    uint16_t len = strlen(str);
    if(Serial.availableForWrite() >= MAX_STRING_LENGTH+5+_ea){
        _SetStringReply = false;
        uint16_t i, j = 0;
        //command = slave address + opcode + 8/16bit address + string + null + CRC
        uint8_t command[MAX_STRING_LENGTH+6] = {_AMULET_ADDRESS, _SET_STRING, 0};
        if (_ea) {
            command[2] = (uint8_t)(loc >> 8);
            i=3;
        }
        else {
            i=2;
        }
        command[i++] = (uint8_t)(loc & 0xFF);
        //sanity check to not overflow buffer
        if (MAX_STRING_LENGTH < len)
            len = MAX_STRING_LENGTH;
        
        while (len > 0){
            command[i++] = str[j++];
            len--;
        }
        command[i++] = 0;
        appendCRC(command,i);
        i+=2;
        
        if (waitForResponse){
            return send_command_blocking(command, i);
        }
        else{
            Serial.write(command,i);
            return true;
        }
    }
}

/**
* Send out a serial command to set a String in the Amulet InternalRAM.String memory
* Will always wait for response if there is room in the serial buffer.
* @param start uint16_t the first index into the Amulet array.
* @param str const char * the source string
* @return int8_t true if correct response was received, false otherwise
*/
int8_t AmuletLCD::setString(uint16_t loc, const char * str){
    setString(loc, str, 1);
}

uint8_t AmuletLCD::requestString(uint16_t loc, uint8_t * destination_buffer, uint16_t buffer_length) {
	if(Serial.availableForWrite() >= 5+_ea){
		_GetStringReply = false;
		_GetStringDest = destination_buffer;
		_GetStringLen = buffer_length;
        uint8_t command[6] = {_AMULET_ADDRESS, _GET_STRING, 0, 0, 0, 0};
        uint8_t i;
        if (_ea) {
            command[2] = (uint8_t)(loc >> 8);
            i = 3;
        }
        else {
            i = 2;
        }            
        command[i++] = (uint8_t)(loc & 0xFF);
        appendCRC(command,i);
        i+=2;
		return send_command_blocking(command, i);
	}
}


/**
* Send out a serial command to call a GEMscript public function that exists on the current page.
* This includes "@" functions like "@load" and "@init" that do not require a parameter.
* It can optionally wait for response or just get out as soon as serial buffer populated
* @param str const char * the name of the script to call, max 32 characters
* @param waitForResponse uint8_t true will block until response is received or timeout occurs
* @return int8_t true if correct response was received or a response was not requested, false otherwise
*/
int8_t AmuletLCD::callScript(const char* fname, uint8_t waitForResponse){
    if(Serial.availableForWrite() >= 37){
        _scriptReply = INVALID_SCRIPT_REPLY;
        _InvokeGEMscriptReply = false;
        //longest GEMscript method name is 32 bytes,  + null, slave addr, opcode and 2-byte CRC = 37
        uint8_t command[37] = {_AMULET_ADDRESS, _INVOKE_GEMSCRIPT, 0}; 
        uint8_t i=2;
        if (strlen(fname) > 32)
            return -1;
        while(*fname !=0)
            command[i++] = *fname++;
        
        command[i++] = 0;
        appendCRC(command,i);
        i+=2;
        if (waitForResponse){
            return send_command_blocking(command, i);
        }
        else{
            Serial.write(command,i);
            return true;
        }
    }
    else{
        setError();
        return false;
    }
}

/**
* Send out a serial command to call a GEMscript public function that exists on the current page.
* This includes "@" functions like "@load" and "@init" that do not require a parameter.
* It will wait for a response, whose value can be retrieved by calling scriptReply
* @param str const char * the name of the script to call, max 32 characters
* @param waitForResponse uint8_t true will block until response is received or timeout occurs
* @return int8_t true if correct response was received or a response was not requested, false otherwise
*/
int8_t AmuletLCD::callScript(const char* fname){
    return callScript(fname, 1);
}

/**
* returns the value of the last callScript method reply.
* @return int32_t valid only after reply is received, so do not depend on this value upon return of callScript if waitForResponse was not true.
*/
int32_t AmuletLCD::scriptReply(){
    return _scriptReply;
}

/**
* Utility function for all blocking master messages.
* Will handle timeouts and retries.
* @param command uint8_t * the array containing the command to send.
* @param length uint16_t the number of bytes to send
* @return int8_t true if correct response was received, false otherwise
*/
uint8_t AmuletLCD::send_command_blocking(uint8_t * command, uint16_t length)
{
	uint8_t tryNumber = 0;
	while (1){
		Serial.write(command,length);
		uint32_t startTime = millis();
		while (millis() - startTime < _Timeout_ms){
			serialEvent();
			switch (command[1])
			{
				case _GET_BYTE:
					if (_GetByteReply)
						return true;
					break;
				case _GET_WORD:
					if (_GetWordReply)
						return true;
					break;
				case _GET_STRING:
				if (_GetStringReply)
						return true;
					break;
				case _GET_COLOR:
					if (_GetColorReply)
						return true;
					break;
				case _GET_BYTE_ARRAY:
					if (_GetBytesReply)
						return true;
					break;
				case _GET_WORD_ARRAY:
					if (_GetWordsReply)
						return true;
					break;
				case _GET_COLOR_ARRAY:
					if (_GetColorsReply)
						return true;
					break;
//				case _GET_RPC:
//				case _GET_LABEL:
				case _SET_BYTE:
					if (_SetByteReply)
						return true;
					break;
				case _SET_WORD:
					if (_SetWordReply)
						return true;
					break;
				case _SET_STRING:
                    if (_SetStringReply)
						return true;
					break;
				case _SET_COLOR:
					if (_SetColorReply)
						return true;
					break;
				case _SET_BYTE_ARRAY:
					if (_SetBytesReply)
						return true;
					break;
				case _SET_WORD_ARRAY:
					if (_SetWordsReply)
						return true;
					break;
				case _SET_COLOR_ARRAY:
					if (_SetColorsReply)
						return true;
					break;
                case _INVOKE_GEMSCRIPT:
					if (_InvokeGEMscriptReply)
						return true;
					break;
				default:
					return false;
			}
		}
		if (tryNumber < _retries){
			tryNumber++;
		}
		else{
			setError();
			return false;
		}
	}
}

/**
* Utility function to calculate the MODBUS CRC of the given array.
* @param ptr uint8_t* the array to calculate
* @param count uint16_t the length of the array
* @return uint16_t The calculated CRC value.
*/
uint16_t AmuletLCD::calcCRC(uint8_t *ptr, uint16_t count){
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

void AmuletLCD::appendCRC(uint8_t *ptr, uint16_t count){
	uint16_t CRC = calcCRC(ptr, count);
	ptr[count] = CRC & 0xFF;
	ptr[count+1] = (CRC >> 8) & 0xFF;
}

/**
* Heartbeat function that manages the state machine with incoming serial data. This should be called periodically
* in your main loop or your own serialEvent() if supported by your board.
* https://www.arduino.cc/en/Tutorial/SerialEvent
* If arrays longer than the serial buffer are requested and you have blocking code, then consider peppering this command within your block
* Alternatively, call UART_State_Machine from a real UART interrupt 
*(advanced topic, additional libraries may be required, plus you need to make UART_State_Machine public)
*/
void AmuletLCD::serialEvent(){
	while(Serial.available() > 0){
		CRC_State_Machine(Serial.read());
  	}
}

/**
* Main state machine of the Amulet CRC protocol handler.
* @param b uint8_t the next serial byte to process.
*/
void AmuletLCD::CRC_State_Machine(uint8_t b){  
  static uint16_t i;     //remaining bytes before CRC for known length commands (non-string)
  static int16_t count; //count of bytes left
  switch(_UART_State){
    case _RECIEVE_BEGIN:   //begin - look for a valid address
        if ((b == _HOST_ADDRESS)||(b == _AMULET_ADDRESS)) {
            _UART_State = _PARSE_OPCODE;
            _RxBuffer[_RxBufferLength++] = b;
            i = 1;
        if (b == _AMULET_ADDRESS)
            _reply = true;  //this is a reply to a previous Arduino-as-master Get or Set command.
        else
            _reply = false; //this is a new Amulet-as-master command
        }  
        //else{} //stay in this state
        break;
    case _PARSE_OPCODE:               //parse opcode to determine next state
        count = recieve_OpcodeParser(b);  	  
        if (count == -1) {               //invalid opcode, reset state machine
            _RxBufferLength = 0;
            _UART_State = _RECIEVE_BEGIN;
        }
        else if (count == -2) {           //Reply to SET cmd
            count = 0; //there is no data
            _RxBuffer[_RxBufferLength++] = b;
            _UART_State = _GET_CRC1;
        }
        else if (count == 0) {           //variable length array or string
            _RxBuffer[_RxBufferLength++] = b;
            if ((b == _SET_STRING) || (b == _GET_STRING)) {
                if (_ea)
                    _UART_State = _VARIABLE_LENGTH_STRING_ADDR1;
                else
                    _UART_State = _VARIABLE_LENGTH_STRING_ADDR2;
            }
            else{
                if (_ea)
                    _UART_State = _VARIABLE_LENGTH_ARRAY_ADDR1;
                else
                    _UART_State = _VARIABLE_LENGTH_ARRAY_ADDR2;
            }      
        }
        else if (count > 0) {            //static length command
            _RxBuffer[_RxBufferLength++] = b;
            _UART_State = _STATIC_LENGTH;
        }   
        break;
    case _STATIC_LENGTH:              //fixed length command. increment i until count bytes received, then get CRC
        if (i < count) {
            _RxBuffer[_RxBufferLength++] = b;
            i++;
        }
        else {
            _RxBuffer[_RxBufferLength++] = b;
            _UART_State = _GET_CRC1;
        }
        break;
    case _VARIABLE_LENGTH_ARRAY_ADDR1:  //array command, next byte contains starting address
        _RxBuffer[_RxBufferLength++] = b;
        _UART_State = _VARIABLE_LENGTH_ARRAY_ADDR2;
        break;
    case _VARIABLE_LENGTH_ARRAY_ADDR2:  //array command, next byte contains starting address
        _RxBuffer[_RxBufferLength++] = b;
        _UART_State = _ARRAY_START;
        break;
    case _VARIABLE_LENGTH_STRING_ADDR1:
        _RxBuffer[_RxBufferLength++] = b;
        _UART_State = _VARIABLE_LENGTH_STRING_ADDR2;
        break;
    case _VARIABLE_LENGTH_STRING_ADDR2:
        _RxBuffer[_RxBufferLength++] = b;
        _UART_State = _VARIABLE_LENGTH_STRING;
        break;
    case _VARIABLE_LENGTH_STRING:
        if (b != 0x00 || count == 0) {
            _RxBuffer[_RxBufferLength++] = b;
            count++;
        }
        else {
            _RxBuffer[_RxBufferLength++] = b;
            count++;
            _UART_State = _GET_CRC1;
        }
        break;
    case _ARRAY_START:
      _RxBuffer[_RxBufferLength++] = b;
      switch(_RxBuffer[1]) {              //calc # of bytes before CRC
        case _SET_BYTE_ARRAY:
		case _GET_BYTE_ARRAY:  //should only get here when receiving a reply, not a master message from Amulet.
          count = b;
          break;
        case _SET_WORD_ARRAY:
		case _GET_WORD_ARRAY:
          count = 2 * b;
          break;
        case _SET_COLOR_ARRAY:
		case _GET_COLOR_ARRAY:
          count = 4 * b;
          break;
      }
      if (count > 0) {
        _UART_State = _ARRAY_DATA;
      }
      else{
        _UART_State = _GET_CRC1;
      }
      break;
    case _ARRAY_DATA:
      if (i < count) {
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
* Utility function to calculate the length of a command based upon the opcode.
* Array command length can be calculated after getting the count, and string commands look for a Null.
* @param b uint8_t The opcode
* @return uint8_t The number of bytes left before the CRC
*/
int8_t AmuletLCD::recieve_OpcodeParser(uint8_t b){
    if (_reply)
    {
        switch (b)
        {
            case _GET_BYTE:
                return 2+_ea;
            case _GET_WORD:
                return 3+_ea;
            case _GET_COLOR:
                return 5+_ea;
            case _GET_BYTE_ARRAY:
            case _GET_WORD_ARRAY:
            case _GET_COLOR_ARRAY:
			case _GET_STRING:
                return 0;
            case _SET_BYTE:
            case _SET_WORD:
            case _SET_COLOR:
            case _SET_STRING:
                return -2; //No packet data follows opcode in a reply to a SET command, just CRC 
            case _INVOKE_GEMSCRIPT:
                return 4;
        }
    }
    //else - not a reply
    if (b==_GET_BYTE || b==_GET_WORD || b==_GET_STRING || b==_GET_COLOR || b==_GET_LABEL) {
        return 1+_ea;
    }
    if (b==_INVOKE_RPC){ 
        return 1;
    }
    else if (b==_SET_BYTE || b==_GET_BYTE_ARRAY ||  b==_GET_WORD_ARRAY || b==_GET_COLOR_ARRAY){ //set: Byte. getarray: byte, word, color.
        return 2+_ea;
    }
    else if (b==_SET_WORD){
        return 3+_ea;
    }
    else if (b==_SET_COLOR){
        return 5+_ea;
    }
    else if (b==_SET_STRING || b==_SET_BYTE_ARRAY || b==_SET_WORD_ARRAY || b==_SET_COLOR_ARRAY){  // variable length array
        return 0;
    }
    else {   //invalid opcode. 
        return -1; 
    }
}



/**
* Utility function to confirm the CRC is valid.
* Split the buffer into two parts: 
*  1. Everything up to but not including the last two bytes
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
	uint16_t start;
	uint16_t count = buf[3+_ea];
	uint8_t * arrayPtr;
	uint8_t * strPtr;
	uint8_t * srcPtr;
	uint8_t temp1, temp2, temp3;
    if (_ea)
        start = (buf[2] << 8) + buf[3];
    else
        start = buf[2];
	//Serial.write(buf,bufLen); //DEBUG
  if(checkCRC(buf,bufLen)){ //first verify the CRC is good.
	if (_reply){  
		switch(buf[1]){
		  case _GET_BYTE:
			_Bytes[start] = buf[3+_ea];
			_GetByteReply = true;
			break;
		  case _GET_WORD:
			_Words[start] = word(buf[3+_ea],buf[4+_ea]);
			_GetWordReply = true;
			break;
		  case _GET_STRING:
			strPtr = _GetStringDest;
			srcPtr = buf+3+_ea;
			for (uint16_t i = 0; i < _GetStringLen && 0 != *srcPtr; i++) {
				*strPtr++ = *srcPtr++;
			}
			*strPtr = 0; //finish will null.
			_GetStringReply = true;
			
			break;
		  case _GET_COLOR:
			_Colors[start] = ((long(buf[3+_ea]) << 24) | (long(buf[4+_ea]) << 16) | (long(buf[5+_ea]) << 8) | buf[6+_ea]);
			_GetColorReply = true;
			break;
		  case _GET_BYTE_ARRAY:
		    //start = buf[2]; already done above
			//count = buf[3]; already done above
			buf += (4+_ea);//increment the buffer pointer to the first valid data
			arrayPtr = _Bytes + start;
			if (((uint16_t)start + count) < _BytesLength){ //make sure new array fits into local buffer.
				while(count){
					*arrayPtr++ = *buf++;
					count--;
				}
			}
			else{
				setError();//TODO Array overflow error
			}
			_GetBytesReply = true;
			break;
		  case _GET_WORD_ARRAY:
			//start = buf[2]; already done above
			//count = buf[3]; already done above
			buf += (4+_ea);//increment the buffer pointer to the first valid data
			arrayPtr = ((uint8_t *)_Words) + (start*2);  //words are 2 bytes each
			if (((uint16_t)start + count) < _WordsLength){ //make sure new array fits into local buffer.
				while(count){  //copy array, swapping byte order
					temp1 = *buf++;
					*arrayPtr++ = *buf++;
					*arrayPtr++ = temp1;
					count--;
				}
			}
			else{
				setError();//TODO Array overflow error
			}
			_GetWordsReply = true;
			break;
		  case _GET_COLOR_ARRAY:
			//start = buf[2]; already done above
			//count = buf[3]; already done above
			buf += (4+_ea);//increment the buffer pointer to the first valid data
			arrayPtr = ((uint8_t *)_Colors) + (start*4);  //colors are 4 bytes each
			if (((uint16_t)start + count) < _ColorsLength){ //make sure new array fits into local buffer.
				while(count){  //copy array, swapping byte order
					temp1 = *buf++;
					temp2 = *buf++;
					temp3 = *buf++;
					*arrayPtr++ = *buf++;
					*arrayPtr++ = temp3;
					*arrayPtr++ = temp2;
					*arrayPtr++ = temp1;
					count--;
				}
			}
			else{
				setError();//TODO Array overflow error
			}
			_GetColorsReply = true;
			break;
		  case _SET_BYTE:
			_SetByteReply = true;
			break;
		  case _SET_WORD:
		    _SetWordReply = true;
			break;
		  case _SET_STRING:
			_SetStringReply = true;
			break;
		  case _SET_COLOR:
		    _SetColorReply = true;
			break;
		  case _SET_BYTE_ARRAY:
		    _SetBytesReply = true;
			break;
		  case _SET_WORD_ARRAY:
		    _SetWordsReply = true;
			break;
		  case _SET_COLOR_ARRAY:
		    _SetColorsReply = true;
			break;
          case _INVOKE_GEMSCRIPT:
            _InvokeGEMscriptReply = true;
            _scriptReply = ((long(buf[2]) << 24) | (long(buf[3]) << 16) | (long(buf[4]) << 8) | buf[5]);
            break;
		}
	}
    else{
	  _TxBuffer[0] = _HOST_ADDRESS; //all slave commands start with Host ID, then Opcode
	  _TxBuffer[1] =  buf[1]; //the reply opcode is the same as the initial command
	
		switch(buf[1]){
		  case _GET_BYTE:
			//_TxBuffer[0] = _HOST_ADDRESS;  //already set above, put here for clarity
			//_TxBuffer[1] = _GET_BYTE;      //already set above, put here for clarity
            i=2;
            if (_ea){
                _TxBuffer[i++] = buf[2];
                _TxBuffer[i++] = buf[3];
            }
            else {
                _TxBuffer[i++] = buf[2];
            }
            _TxBuffer[i++] = _Bytes[start];
			returnCRC= calcCRC(_TxBuffer,i);
			_TxBuffer[i++] = returnCRC & 0xFF;
			_TxBuffer[i++] = (returnCRC >> 8) & 0xFF;
			Serial.write(_TxBuffer,i);
			break;
		  case _GET_WORD:
			//_TxBuffer[0] = _HOST_ADDRESS;  //already set above, put here for clarity
			//_TxBuffer[1] = _GET_WORD;      //already set above, put here for clarity
			i=2;
            if (_ea){
                _TxBuffer[i++] = buf[2];
                _TxBuffer[i++] = buf[3];
                
            }
            else {
                _TxBuffer[i++] = buf[2];
            }
            _TxBuffer[i++] = (_Words[start] >> 8) & 0xFF;//MSB first for data
			_TxBuffer[i++] = _Words[start] & 0xFF;
			returnCRC= calcCRC(_TxBuffer,i);
			_TxBuffer[i++] = returnCRC & 0xFF;             //LSB first for CRC
			_TxBuffer[i++] = (returnCRC >> 8) & 0xFF;
			Serial.write(_TxBuffer,i);
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
			i=2;
            if (_ea){
                _TxBuffer[i++] = buf[2];
                _TxBuffer[i++] = buf[3];
                
            }
            else {
                _TxBuffer[i++] = buf[2];
            }
			_TxBuffer[i++] = 0;
			returnCRC= calcCRC(_TxBuffer,i);
			_TxBuffer[i++] = returnCRC & 0xFF;
			_TxBuffer[i++] = (returnCRC >> 8) & 0xFF;
			Serial.write(_TxBuffer,i);
			break;
		  case _GET_COLOR:
			//_TxBuffer[0] = _HOST_ADDRESS;  //already set above, put here for clarity
			//_TxBuffer[1] = _GET_COLOR;     //already set above, put here for clarity
			i=2;
            if (_ea){
                _TxBuffer[i++] = buf[2];
                _TxBuffer[i++] = buf[3];
                
            }
            else {
                _TxBuffer[i++] = buf[2];
            }
            _TxBuffer[i++] = (_Colors[start] >> 24) & 0xFF;//MSB first for data
            _TxBuffer[i++] = (_Colors[start] >> 16) & 0xFF;
            _TxBuffer[i++] = (_Colors[start] >>  8) & 0xFF;
			_TxBuffer[i++] =  _Colors[start]        & 0xFF;

			returnCRC= calcCRC(_TxBuffer,i);
			_TxBuffer[i++] = returnCRC & 0xFF;             //LSB first for CRC
			_TxBuffer[i++] = (returnCRC >> 8) & 0xFF;
			Serial.write(_TxBuffer,i);
			
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
			_Bytes[start] = buf[3+_ea];
			SetCmd_Reply(_SET_BYTE);
			break;
		  case _SET_WORD:
			_Words[start] = word(buf[3+_ea],buf[4+_ea]);
			SetCmd_Reply(_SET_WORD);
			break;
		  case _SET_STRING:
			//TODO: need to actually write string to memory, see getString comments above
			SetCmd_Reply(_SET_STRING);
			break;
		  case _SET_COLOR:
			_Colors[start] = ((long(buf[3+_ea]) << 24) | (long(buf[4+_ea]) << 16) | (buf[5+_ea] << 8) | buf[6+_ea]);        
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
			SetCmd_Reply(_INVOKE_RPC);
			callRPC(buf[2]);
			break;
		}
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

/**
* Read the current error status, then reset the status.
* @return the current error count
*/
uint32_t AmuletLCD::readError(){
	uint32_t ec = _errorCount;
	_errorCount = 0;
	return ec;
}

/**
* Set the current error status.
*/
void AmuletLCD::setError(){
	_errorCount++;
}
