
#include <p18f47j53.h>
#include "Prog39vf010.h"
#include "./USB/usb.h"
#include "./USB/usb_function_cdc.h"


/************************************
* Global Variables					*
************************************/
unsigned char   gState;
unsigned char 	gCmdMode;
unsigned int 	chip_id0, chip_id1;
unsigned int 	gAddress;
unsigned char 	gA16;
unsigned char 	gBank;
unsigned char 	gaddr[3];

PORTBYTE ShadowA;
PORTBYTE ShadowAMask;
PORTBYTE ShadowB;
PORTBYTE ShadowBMask;
PORTBYTE ShadowC;
PORTBYTE ShadowCMask;
PORTBYTE ShadowD;
PORTBYTE ShadowDMask;
PORTBYTE ShadowE;
PORTBYTE ShadowEMask;

PORTBYTE Addr0_7;
PORTBYTE Addr8_15;
PORTBYTE Addr16_23;

PORTBYTE DataBits;

#define OVERHEAD (2)

void delay_us(unsigned char us)
{
   if ( us <= OVERHEAD ) 
	return; // prevent underflow
   us  -= OVERHEAD ;             // overhead of function call in us.

   Nop();       // 1  extra overhead to make function overhead an even us.
   Nop();       // 1  add or remove Nop's as necessary.
   Nop();       // 1

   do   // loop needs to be 12 cycles so each cycle is 1us.
   {
      Nop();       // 1
      Nop();       // 1
      Nop();       // 1
      Nop();       // 1
      Nop();       // 1
      Nop();       // 1
      Nop();       // 1
      Nop();       // 1
      Nop();       // 1
      //ClrWdt();    // 1
   } while(--us);  // 3
}


void delay_ms(char ms)
{
	int count = ms * 4;
	for (count = ms*4; count > 0; count --)
	{
		delay_us(250);
	}
}


void SetAddress(char a16, int Address)//sets the address into the address latches
{
	Addr0_7.Byte = Address & 0xff;
	Addr8_15.Byte = (Address >> 8) & 0xff;
	Addr16_23.Byte = a16&1;

	// port B0-7 ->A0-A7
	ShadowB.Byte &= ~ShadowBMask.Byte;
	ShadowB.Byte |= Addr0_7.Byte & ShadowBMask.Byte;
	// Port A0-3 ->A8-A11
	ShadowA.Byte &= ~ShadowAMask.Byte;
	ShadowA.Byte |= Addr8_15.Byte & ShadowAMask.Byte;

	ShadowC.Byte &= ~ShadowCMask.Byte;
	ShadowC.B7 = Addr8_15.B4;
	ShadowC.B6 = Addr8_15.B5;
	ShadowC.B2 = Addr8_15.B6;
	ShadowC.B1 = Addr8_15.B7;
	ShadowC.B0 = Addr16_23.B0;


	LATA = ShadowA.Byte;
	LATB = ShadowB.Byte;
	LATC = ShadowC.Byte;

}


// _chip is 0 for none, 1, or 2
void SetControl( char _chip, char _dir, char _write )
{
	ShadowE.Byte |= ShadowEMask.Byte;
	ShadowE.Byte = ShadowEMask.Byte;
	switch (_chip)
	{
	case 0:
		break;
	case 1:
		ShadowE.B1 = 0;
		break;
	case 2:
		ShadowE.B0 = 0;
		break;
	}

	if (_dir == OUTPUT_ENABLE) //_OE
		ShadowE.B2 = 0;
	else
		ShadowE.B2 = 1;
	
	if (_write)
		ShadowA.B5 = 0;
	else
		ShadowA.B5 = 1;


	LATE = ShadowE.Byte;
	LATA = ShadowA.Byte;
}

void DisableChips(void)		//disable all memory chips signals
{
	SetControl( 0, OUTPUT_DISABLE, READ );
}


//Writes the data to the defined address
//Remember that the cartridge has 2 chips with 17 address bits.
//Then, the physical address is built with an "int" plus one bit (a16)
//Bank defines the chip itself
void WriteData(char _chip, char a16, int Address, char _value)
{
	PORTBYTE value;
	PORTBYTE out;

	value.Byte = _value;

	out.B0 = value.B7;
	out.B1 = value.B6;
	out.B2 = value.B5;
	out.B3 = value.B4;
	out.B4 = value.B3;
	out.B5 = value.B2;
	out.B6 = value.B1;
	out.B7 = value.B0;

	SetAddress(a16, Address);
	TRISD = 0;					//set port for address output
	LATD = out.Byte;
	SetControl( _chip, OUTPUT_DISABLE, WRITE );
	delay_us(20);
	// disable chips
	SetControl( 0, OUTPUT_DISABLE, READ );
	TRISD = 0xFF;
}

//Reads the data from the defined address
//see WriteData above for details
char ReadData(char _chip, char a16, int Address)
{
	PORTBYTE value;
	PORTBYTE out;
	SetAddress(a16, Address);
	TRISD = 0xFF;
	SetControl( _chip, OUTPUT_ENABLE, READ );
	value.Byte = PORTD;
	SetControl( 0, OUTPUT_DISABLE, READ );

	out.B0 = value.B7;
	out.B1 = value.B6;
	out.B2 = value.B5;
	out.B3 = value.B4;
	out.B4 = value.B3;
	out.B5 = value.B2;
	out.B6 = value.B1;
	out.B7 = value.B0;
	
	return out.Byte;
}

//Reads the data from the current address
char ReadByte(char _chip)
{
	PORTBYTE value;
	PORTBYTE out;
	TRISD = 0xFF;
	SetControl( _chip, OUTPUT_ENABLE, READ );
	value.Byte = PORTD;
	SetControl( 0, OUTPUT_DISABLE, READ );

	out.B0 = value.B7;
	out.B1 = value.B6;
	out.B2 = value.B5;
	out.B3 = value.B4;
	out.B4 = value.B3;
	out.B5 = value.B2;
	out.B6 = value.B1;
	out.B7 = value.B0;

	return out.Byte;
}



/************************************
* SST39VF010 specific code			*
************************************/

char  Vendor_ID;
char  Device_ID;

unsigned short Check_SST_39VF010(char chip)
{
//	char  Vendor_ID;
//    char  Device_ID;
    unsigned short  ReturnStatus = 0;

	WriteData(chip, 0, 0x5555, 0xAA);
    WriteData(chip, 0, 0x2AAA, 0x55);
    WriteData(chip, 0, 0x5555, 0x90);
	delay_us(20); 
      
    Vendor_ID  = ReadData(chip, 0, 0); // read manufacturer ID, should be BFh for SST device
    Device_ID  = ReadData(chip, 0, 1); // read device ID, should be D5h for SST39VF010
    

	//ReturnStatus = Vendor_ID;
	//ReturnStatus = Vendor_ID * 256;
	//ReturnStatus = ReturnStatus | Device_ID;

    WriteData(chip, 0, 0x1234, 0xF0);        
    delay_us(20); 
    return ReturnStatus;
}

void Erase_Entire_Chip(char chip)
{
	WriteData(chip, 0, 0x5555, 0xAA);
	WriteData(chip, 0, 0x2AAA, 0x55);
	WriteData(chip, 0, 0x5555, 0x80);
	WriteData(chip, 0, 0x5555, 0xAA);
	WriteData(chip, 0, 0x2AAA, 0x55);
	WriteData(chip, 0, 0x5555, 0x10);
	delay_ms(150);
}

void Program_One_Byte (char chip, char a16, int Address, char value)
{
	WriteData(chip, 0, 0x5555, 0xAA);
	WriteData(chip, 0, 0x2AAA, 0x55);
	WriteData(chip, 0, 0x5555, 0xA0);
	WriteData(chip, a16, Address, value);
	delay_us(22);
}

void Program_two_Bytes (char a16, int Address, char value0, char value1)
{
	WriteData(1, 0, 0x5555, 0xAA);
	WriteData(2, 0, 0x5555, 0xAA);
	
	WriteData(1, 0, 0x2AAA, 0x55);
	WriteData(2, 0, 0x2AAA, 0x55);	
	
	WriteData(1, 0, 0x5555, 0xA0);
	WriteData(2, 0, 0x5555, 0xA0);	
	
	WriteData(1, a16, Address, value0);
	WriteData(2, a16, Address, value1);
	
	delay_us(22);
}


/********************************************************************/
/************************************
* Uploader Routines					*
************************************/
void erase_chips()
{
	Erase_Entire_Chip(1);
	Erase_Entire_Chip(2);
	AddOutput( 'e' );
}

void send_version(void)
{
	AddOutput( 'w' );
	AddOutput( FIRM_VER );
}

void read_chip_hex(void)	//reads entire chip and sends to serial
{
	char aux = 0;
	int numBytesRead = 0;
	char Buff[1];

	//clears chip address buffer
	gAddress = 0;
	for (gA16 = 0; gA16<=1; gA16++)
	{
		gAddress = 0;
		while(1)
		{
			char tmp = 0;
			
			if (!(gAddress & 0x1f))
			{
				send_lfcr();
				numBytesRead = GetByte( &tmp );
				if (numBytesRead)						//if receive something
				{
					aux = tmp;
					if (aux == '!')
						break;
				}
			}
				
			send_hex(ReadData(1, gA16, gAddress));		
			send_hex(ReadByte(2));		
			
			gAddress++;
			if (gAddress == 0)
				break;
		}

		if (aux == '!')
			break;
	}
}

void read_chip(void)
{
	char aux = 0;
	int numBytesRead = 0;
	char Buff[1];

	//clears chip address buffer
	gAddress = 0;
	for (gA16 = 0; gA16<=1; gA16++)
	{
		gAddress = 0;
		while(1)
		{
			send_byte(ReadData(1, gA16, gAddress));		
			send_byte(ReadByte(2));		
			
			gAddress++;
			if (gAddress == 0)
				break;
		}

		if (aux == '!')
			break;
	}
}

void write_chip(void)
{
	char bufer[2];
	int numBytesRead = 0;
	char tmp;
	gAddress = 0;

	
	for (gA16 = 0; gA16<=1; gA16++)
	{
		while(1)
		{
			bufer[0] = receive_byte();
/*			if (bufer[0] == '!')
			{
				bufer[0] = receive_byte();
				if (bufer[0] != '!')
				{
					break;
				}
			}	*/
			bufer[1] = receive_byte();
/*			if (bufer[1] == '!')
			{
				bufer[1] = receive_byte();
				if (bufer[1] != '!')
				{
					break;
				}
			}	*/

			Program_two_Bytes (gA16, gAddress, bufer[0], bufer[1]);

			send_byte( ReadByte(1) );
			send_byte( ReadByte(2) );
			FlushOutput();

			gAddress++;

//			if (!(gAddress & 0xf))
//				send_lfcr();
			
			if (gAddress == 0)
				break;
		}
	}
}





/************************************
* Serial communication				*
************************************/
void send_byte(char value)
{
	AddOutput( value );
}


unsigned char receive_byte(void)
{
	int numBytesRead = 0;
	char Buff[2];

	char cnt;
	do
	{ 
		cnt = GetByte( Buff );
    	CDCTxService();
	} while (cnt == 0);

	return Buff[0];
}


void send_hex(char valor)
{
	char buf[2];
	char aux;
	aux = valor >> 4;	//hi nibble
	if (aux<10)
		buf[0] = aux + '0';
	else
		buf[0] = aux + 87;	//a-f
		
	aux = valor & 0x0F;	//lo nibble
	if (aux<10)
		buf[1] = aux + '0';
	else
		buf[1] = aux + 87;	//a-f

	AddOutput( buf[0] );
	AddOutput( buf[1] );
}

void send_lfcr(void)
{
	AddOutput( 0x0d );
	AddOutput( 0x0a );
}

void send_msg(char *text)
{
	int cnt = 0;
	while (text[cnt])
	{
		AddOutput( text[cnt] );
		cnt++;
	}

	FlushOutput();
}

void send_msg_crlf(char *text)
{
	int cnt = 0;
	while (text[cnt])
	{
		AddOutput( text[cnt] );
		cnt++;
	}

	AddOutput( 0x0d );
	AddOutput( 0x0a );

	FlushOutput();
}

char waittext[] = "Press a key...";
void wait_key(void)
{
	int cnt;
	char tmp;
	send_msg(waittext);
	do
	{ 
		cnt = GetByte( &tmp );
	} while (cnt == 0);
	send_lfcr();

	FlushOutput();
}



/***********************************
* Init routine                     *
* Sets I/O Ports and other         *
* initialization                   *
***********************************/
void on_init( void )
{
	// empty our shadow registers
	ShadowA.Byte = 0;
	ShadowAMask.Byte = 0x0f;
	ShadowB.Byte = 0;
	ShadowBMask.Byte = 0xff;
	ShadowC.Byte = 0;
	ShadowCMask.Byte = 0;
	ShadowD.Byte = 0;
	ShadowDMask.Byte = 0xff;
	ShadowE.Byte = 0;
	ShadowEMask.Byte = 0x07;

	Addr0_7.Byte = 0;
	Addr8_15.Byte = 0;
	Addr16_23.Byte = 0;

	DataBits.Byte = 0;

	// IO ports setup
	TRISA = 0xC0;
	LATA = ShadowA.Byte;
	TRISB = 0x00;
	LATB = ShadowB.Byte;
	TRISC = 0x00;
	LATC = ShadowC.Byte;
	TRISD = 0xff;
	LATD = ShadowD.Byte;
	TRISE = 0xf8;
	LATE = ShadowE.Byte;

	// everything digital
	ANCON0 = 0xff;
	ANCON1 = 0x0f;

}

char title_text[] = TITLE;
char warning_text[] = WARNING;
char debug_text[] = "Debug Mode";
/***************************************************************************/
void main_programmer()
{
	unsigned char i=0;
	char bufer;
	gState = 0;

	on_init();
	
	send_msg_crlf(title_text);
	send_msg_crlf(warning_text);

	while(1)
	{		
		FlushOutput();
		bufer = receive_byte();
		if (bufer)
		{
			switch(bufer)
			{
				case CMD_DEBUG:
					send_lfcr();
					send_msg_crlf(debug_text);
					debug_mode();
					break;
					
				case CMD_VERSION:	//firmware version number
					send_version();
					break;
				case CMD_ID:		//chip IDs					
					chip_id0 = Check_SST_39VF010(1);	//checks chip on bank 0
					send_hex( Vendor_ID );
					send_hex( Device_ID );
					send_byte('.');
					chip_id1 = Check_SST_39VF010(2);	//checks chip on bank 1
					send_hex( Vendor_ID );
					send_hex( Device_ID );
					//send_hex(chip_id0 >> 8);			//manufacturer ID
					//send_hex(chip_id0 & 0xFF);			//Device ID
					//send_hex(chip_id1 >> 8);			//manufacturer ID
					//send_hex(chip_id1 & 0xFF);			//Device ID					
					break;

				case CMD_ERASE:		//erase chips
					erase_chips();
					break;
				case CMD_RD:		//read chips
					read_chip();
					break;
				case CMD_WR:		//write chips
					write_chip();
					break;
				default:
					send_lfcr();
					send_byte('?');	//unknown command			
					send_lfcr();
					break;
			}
		}
	}
}



char exit_text[] = "exiting...";
char _OE_text[] = "_OE";
char _WE_text[] = "_WE";
char CE0_text[] = "CE0";
char CE1_text[] = "CE1";
char addr_text1[] = "A: 0.";
char addr_text2[] = "A: 1.0000H";
char H_text[] = "H";

void debug_mode(void)
{
	char bufer = 0;
	char i;
	unsigned int address;
	while (bufer != 'X')
	{
		FlushOutput();
		bufer = receive_byte();
		switch (bufer)
		{
			/*case 'H':
			case 'h':
			case '?':	//help
				send_msg_crlf("Commands:");
				send_msg_crlf("W - WE test");
				send_msg_crlf("O - OE test");
				send_msg_crlf("0 - CE0 test");
				send_msg_crlf("1 - CE1 test");
				send_msg_crlf("A - Address test");
				//send_msg_crlf("H,h or ? - This help screen");
				send_msg_crlf("X - Exit debug mode");
				break;*/
			case 'X':	//exit
				send_msg_crlf(exit_text);
				break;
			case 'W':	//write enable
				SetControl( 0, OUTPUT_DISABLE, WRITE );
				send_msg(_WE_text);
				wait_key();
				DisableChips();		//disable all memory chips signals
				break;
			case 'O':	//output enable
				SetControl( 0, OUTPUT_ENABLE, READ );
				send_msg_crlf(_OE_text);
				wait_key();
				DisableChips();		//disable all memory chips signals
				break;
			case '0':	//CE0
				SetControl( 1, OUTPUT_DISABLE, READ );
				send_msg_crlf(CE0_text);
				wait_key();
				DisableChips();		//disable all memory chips signals
				break;
			case '1':	//CE1
				SetControl( 2, OUTPUT_DISABLE, READ );
				send_msg_crlf(CE1_text);
				wait_key();				
				DisableChips();		//disable all memory chips signals
				break;
			case 'A':	//adress bus test
				address = 1;
				for(i=0;i<16;i++)
				{
					SetAddress(0,address);
					send_msg_crlf(addr_text1);
					send_hex(address >> 8);
					send_hex(address & 0xFF);
					send_msg_crlf(H_text);
					wait_key();
					address = address << 1;
				}
				SetAddress(1,0);
				send_msg_crlf(addr_text2);
				break;
			case 'D':	//dtaa bus test
				TRISD = 0;					//set port for address output
				LATD = 0x55;
				wait_key();
				LATD = 0xAA;
				wait_key();
				TRISD = 0xff;					//set port for address output
				break;

			case 'd':	//dump chips
				read_chip_hex();	//reads entire chip and sends to serial
				break;
		}
	}		
}



char InputBuffer[64];
char InputBufferLen = 0;
char InputPos = 0;

char GetByte( char *Buf )
{
	char Res = 0;
	if (InputPos >= InputBufferLen)
	{
		InputBufferLen = getsUSBUSART(InputBuffer, 64);
		InputPos = 0;
	}

	if (InputPos < InputBufferLen)
	{
		*Buf = InputBuffer[InputPos];
		InputPos++;
		Res = 1;
	}

	return Res;
}




char OutputBuffer[64];
char OutputPos = 0;

void AddOutput( char c )
{
	if (OutputPos >= 64)
	{
		while (!USBUSARTIsTxTrfReady())
		{
		   	CDCTxService();
		}
		putUSBUSART(OutputBuffer, OutputPos);
		CDCTxService();
		OutputPos = 0;
	}
	OutputBuffer[OutputPos++] = c;
}



void FlushOutput( )
{
	if (OutputPos)
	{
		while (!USBUSARTIsTxTrfReady())
		{
		   	CDCTxService();
		}
		putUSBUSART(OutputBuffer, OutputPos);
		CDCTxService();
		OutputPos = 0;
	}
}

