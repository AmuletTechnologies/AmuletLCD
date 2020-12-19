/*
  Amulet.h - Library for communicating to Amulet OS over UART via the "CRC" communications protocol
  Copyright (c) 2017 Amulet Technologies. All rights reserved.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  Reworked on 10 Feb 2017 by Brian Deters
 */

#ifndef AmuletLCD_h
#define AmuletLCD_h

#include "Arduino.h"

// Define the buffer lengths here, if the user hasn't set their own.
// This is long enough for most messages. 
// Change to send/receive long arrays.
// Max is 0x400 bytes.
#ifndef AMULET_TX_BUF_LEN
#define AMULET_TX_BUF_LEN    64
#endif
#ifndef AMULET_RX_BUF_LEN
#define AMULET_RX_BUF_LEN    64
#endif

#ifndef MAX_STRING_LENGTH
#define MAX_STRING_LENGTH    25
#endif

// Invalid script reply value reset upon invoking callScript. 
// Useful to allow non-blocking function of callScript to determine if reply has been received yet.
// Define your own value if you want to use this for a valid value.
#ifndef INVALID_SCRIPT_REPLY
#define INVALID_SCRIPT_REPLY 0x80000000
#endif

/**
* typedef used by RPC_Entry.
*/
typedef void (* functionPointer) ();
/**
* struct used to make Amulet RPC setup simpler.
*/
typedef struct {
	functionPointer function;
} RPC_Entry;

/**
* A class used to manage the UART state machine between the Amulet display and Arduino.
*/
class AmuletLCD
{
  public:
    AmuletLCD();  
    void begin(uint32_t baud);
	void begin(uint32_t baud, uint8_t config, uint8_t extended_address);
    void setWordPointer(uint16_t * ptr, uint16_t ptrSize);
    void setBytePointer(uint8_t * ptr, uint16_t ptrSize);
    void setColorPointer(uint32_t * ptr, uint16_t ptrSize);
	void setRPCPointer(RPC_Entry * ptr, uint16_t ptrSize);
	void registerRPC(uint8_t index, functionPointer function);

    uint8_t getByte(uint16_t loc);
	uint8_t requestByte(uint16_t loc);
	uint8_t requestBytes(uint16_t start, uint8_t count);
    int8_t setByte(uint16_t loc, uint8_t value);
	int8_t setByte(uint16_t loc, uint8_t value, uint8_t waitForResponse);

    uint16_t getWord(uint16_t loc);
	uint8_t requestWord(uint16_t loc);
	uint8_t requestWords(uint16_t start, uint8_t count);
    int8_t setWord(uint16_t loc, uint16_t value);
	int8_t setWord(uint16_t loc, uint16_t value, uint8_t waitForResponse);

    uint32_t getColor(uint16_t loc);
	uint8_t requestColor(uint16_t loc);
	uint8_t requestColors(uint16_t start, uint8_t count);
    int8_t setColor(uint16_t loc, uint32_t value);
	int8_t setColor(uint16_t loc, uint32_t value, uint8_t waitForResponse);
	
	int8_t setString(uint16_t loc, const char * str);
    int8_t setString(uint16_t loc, const char * str, uint8_t waitForResponse);
	uint8_t requestString(uint16_t start, uint8_t * destination_buffer, uint16_t buffer_length);
	
    int8_t callScript(const char * fname, uint8_t waitForResponse);
    int8_t callScript(const char * fname);
    int32_t scriptReply();
	
    uint32_t readError();
    void serialEvent();
	
    private:
        //Virtual Dual Port RAM arrays:
        uint8_t * _Bytes; 
        uint16_t _BytesLength;    //max length = 32768
        uint16_t * _Words;
        uint16_t _WordsLength;   //max length = 32768
        uint32_t * _Colors;
        uint16_t _ColorsLength;  //max length = 32768
		RPC_Entry * _RPCs;
		uint16_t _RPCsLength;    //max length = 256
		
		uint8_t   _ea; // extended address
		uint32_t  _Timeout_ms;
		uint8_t   _retries;
        uint32_t  _baud;
		uint8_t   _config;
		uint32_t  _errorCount;
		uint32_t  _lastError;
		uint8_t   _reply;
        int32_t   _scriptReply;
		uint8_t * _GetStringDest;
		uint16_t  _GetStringLen;
        
		volatile uint8_t _GetByteReply;
		volatile uint8_t _GetWordReply;
		volatile uint8_t _GetColorReply;
		volatile uint8_t _GetStringReply;
		volatile uint8_t _GetBytesReply;
		volatile uint8_t _GetWordsReply;
		volatile uint8_t _GetColorsReply;
		
		volatile uint8_t _SetByteReply;
		volatile uint8_t _SetWordReply;
		volatile uint8_t _SetColorReply;
		volatile uint8_t _SetStringReply;
		volatile uint8_t _SetBytesReply;
		volatile uint8_t _SetWordsReply;
		volatile uint8_t _SetColorsReply;
		volatile uint8_t _InvokeGEMscriptReply;

        uint8_t _RxBuffer[AMULET_RX_BUF_LEN];
        uint8_t _TxBuffer[AMULET_TX_BUF_LEN];
        uint16_t _RxBufferLength;
		uint16_t _TxBufferLength;
        uint16_t _UART_State;
		
		uint8_t send_command_blocking(uint8_t * command, uint16_t length);
        uint16_t calcCRC(uint8_t *ptr, uint16_t count);
		void appendCRC(uint8_t *ptr, uint16_t count);
        void setup();                    // run once, when the sketch starts    
        void CRC_State_Machine(uint8_t b);
        int8_t recieve_OpcodeParser(uint8_t b);    
        boolean checkCRC(uint8_t *buf, uint16_t bufLen);
        void processUARTCommand(uint8_t *buf, uint16_t bufLen);
        void SetCmd_Reply(uint8_t OPCODE);
		void callRPC(uint8_t index);
		void setError();
    
};

// byte definition for MODBUS node addresses
#define _HOST_ADDRESS            2 
#define _AMULET_ADDRESS          1

//State machine states
#define _RECIEVE_BEGIN                  0
#define _PARSE_OPCODE                   1
#define _STATIC_LENGTH                  2
#define _VARIABLE_LENGTH_ARRAY_ADDR1    3
#define _VARIABLE_LENGTH_ARRAY_ADDR2    4
#define _ARRAY_START                    5
#define _ARRAY_DATA                     6
#define _VARIABLE_LENGTH_STRING         7
#define _VARIABLE_LENGTH_STRING_ADDR1   8
#define _VARIABLE_LENGTH_STRING_ADDR2   9
//#define _VARIABLE_LENGTH_STRING_GET_ADDR1 10
//#define _VARIABLE_LENGTH_STRING_GET_ADDR2 11
#define _GET_CRC1                       10
#define _GET_CRC2                       11

//OPCODES:
#define _GET_BYTE                0x20
#define _GET_WORD                0x21
#define _GET_STRING              0x22
#define _GET_COLOR               0x23
#define _GET_BYTE_ARRAY          0x24
#define _GET_WORD_ARRAY          0x25
#define _GET_COLOR_ARRAY         0x26
#define _GET_RPC                 0x27
#define _GET_LABEL               0x28
#define _SET_BYTE                0x30
#define _SET_WORD                0x31
#define _SET_STRING              0x32
#define _SET_COLOR               0x33
#define _SET_BYTE_ARRAY          0x34
#define _SET_WORD_ARRAY          0x35
#define _SET_COLOR_ARRAY         0x36
#define _INVOKE_RPC              0x37
#define _INVOKE_GEMSCRIPT        0x52



//CRC calculation constants
#define _CRC_SEED                0xFFFF
#define _CRC_POLY                0xA001

#endif