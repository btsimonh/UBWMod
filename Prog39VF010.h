#ifndef _UPLOADER_H_
#define _UPLOADER_H_

/************************************
* Constants and defines        		*
************************************/
#define TITLE				"I-Cybie Uploader 1.0"
#define	WARNING				"Absolutely NO warraties of any kind."
#define FIRM_VER			'1'


//CMD Modes
#define	CMD_IDLE			0x00
#define CMD_WR				0x50	//'P'
#define CMD_RD				0x70	//'p'
#define	CMD_ERASE			'E'
#define	CMD_VERSION			'V'
#define	CMD_ID				'I'
#define	CMD_DEBUG			'D'

#define OUTPUT_DISABLE 1
#define OUTPUT_ENABLE 0

#define WRITE 1
#define READ 0

/************************************
* defines for ports		*
************************************/

union PORTBYTE_UNION
{
  struct tag_Byte{
    unsigned Byte:8;
  };
  struct tag_Bits{
    unsigned B0:1;
    unsigned B1:1;
    unsigned B2:1;
    unsigned B3:1;
    unsigned B4:1;
    unsigned B5:1;
    unsigned B6:1;
    unsigned B7:1;
  };
};

typedef union PORTBYTE_UNION PORTBYTE;

void main_programmer(void);

/************************************
* Hardware abstraction routines		*
************************************/
void DisableChips(void);		//disable all memory chips signals
void SetAddress(char a16, int Address);//sets the address into the address latches
void WriteData(char _bank, char a16, int Address, char _value);//Writes the data to the defined address
char ReadData(char _bank, char a16, int Address);//Reads the data from the defined address
char ReadByte(char _bank);
/************************************
* SST39VF010 specific code			*
* based on SST documentation		*
************************************/
int Check_SST_39VF010(char bank);
void Erase_Entire_Chip(char bank);
void Program_One_Byte (char bank, char a16, int Address, char value);
void Program_two_Bytes (char a16, int Address, char value0, char value1);

/************************************
* Serial communication				*
************************************/
void send_byte(char value);		//send byte to serial port
unsigned char receive_byte(void);	//receive byte from the serial port
void send_hex(char valor);		//send byte to serial port as a 2 char hex string(00-ff)
void send_lfcr(void);
void send_msg(char *text);
void send_msg_crlf(char *text);
void msg(char *text);

/************************************
* EEPROM read/write					*
************************************/

/************************************
* Uploader Routines					*
************************************/
void erase_chips();
void send_version(void);
void read_chip(void);	//reads entire chip and sends to serial
void write_chip(void);//writes serial data to chip
void debug_mode(void);
char GetByte( char *Buf );

void AddOutput( char c );
void FlushOutput(void );

#endif //_UPLOADER_H_