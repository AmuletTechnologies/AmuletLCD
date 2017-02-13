/*
  Amulet.h - Library for communicating to GEMmodule over UART.
  Created by Daniel M. Gopen, August, 26 2014.
  Released into the public domain.
*/

#ifndef AmuletUART_h
#define AmuletUART_h

#include "Arduino.h"

class AmuletUART
{
  public:
	AmuletUART(int baud);  
	void begin(int baud);
	void setWordPointer(word * ptr, int ptrSize);
	void setBytePointer(byte * ptr, int ptrSize);
	void setColorPointer(unsigned long * ptr, int ptrSize);

    int getByte(byte loc);
    int setByte(byte loc, int value);

	int getWord(byte loc);
	int setWord(byte loc, int value);

	unsigned long getColor(byte loc);
	unsigned long setColor(byte loc, int value);
	
    void UART_recieve();
	
	private:
		//initialize Virtual Dual Port RAM arrays:
		byte * _Bytes; 
		int _BytesLength; 
		word * _Words;
		int _WordsLength;	
		unsigned long * _Colors;
		int _ColorsLength;
		// no string support at this time

		int _baud;

		byte _buffer[128];        //UART buffer on Amulet chip is actually 0x400 bytes.
		int _bufferLength;
		int _UART_State;

		//State machine states

		word calcCRC(byte *ptr, int count);
		void setup();                    // run once, when the sketch starts
//		void UART_recieve();	
		void UART_State_Machine(byte b);
		int recieve_OpcodeParser(byte b);	
		boolean checkCRC(byte *buf, int bufLen);
		void processUARTCommand(byte *buf, int bufLen);
		void SetCmdReply(byte OPCODE);
	
};

// byte definition for MODBUS node addresses
#define _HOST_ADDRESS            2
#define _AMULET_ADDRESS          1

//State machine states
#define _RECIEVE_BEGIN           0
#define _PARSE_OPCODE            1
#define _STATIC_LENGTH           2
#define _VARIABLE_LENGTH_ARRAY   3
#define _VARIABLE_LENGTH_STRING  4
#define _ARRAY_START             5
#define _ARRAY_DATA              6
#define _GET_CRC1                7
#define _GET_CRC2                8

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



//CRC calculation constants
#define _CRC_SEED                0xFFFF
#define _CRC_POLY                0xA001

#endif