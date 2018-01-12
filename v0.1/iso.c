/*
Copyright (C) Trampas Stern  name of author

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*******************************************************************
 *	File: iso.c
 *
 * 	Copyright ©,  Trampas Stern. All Rights Reserved.	  
 *  Date: 7/23/2006	4:23:08 PM
 *******************************************************************/

#include "iso.h"
#include "time.h"
#include <util/delay.h>
#include "checksum.h"
#include "usart.h"
#include "lcd.h"
#include "switchio.h"

#define ISO_BUFFER_SIZE 10

#define K_OUT_DDR DDRB
#define K_OUT_PORT PORTB
#define K_OUT_PIN  1
#define K_IN_DDR DDRB
#define K_IN_PORT PORTB
#define K_IN_PIN  0

//invert the logic such that we set based on interface level
#define K_OUT(x) {if(x) BIT_SET(K_OUT_PORT,K_OUT_PIN); else BIT_CLEAR(K_OUT_PORT,K_OUT_PIN);}
#define K_IN BIT_TEST(PINB,0)

#define L_OUT_DDR DDRB
#define L_OUT_PORT PORTB
#define L_OUT_PIN  5

//invert the logic such that we set based on interface level
#define L_OUT(x) {if(x) BIT_SET(L_OUT_PORT,L_OUT_PIN); else BIT_CLEAR(L_OUT_PORT,L_OUT_PIN);}

//#define TX_PIN(x) {if(txPins==1) K_OUT(x); else if(txPins==2) K_OUT(x); else {K_OUT(x); L_OUT(x);}}
#define TX_PIN(x)  {K_OUT(x);}
#define RX_PIN		K_IN

//we  need state machine inside this file
#define ISO_UNKNOWN	0	//we are disconnected or unknown state
#define ISO_CONNECT 1	//we are connected and baud is correct rate

#define ISO_PRESCALE 8  //Timer prescaler

UBYTE iso_getAuto(UBYTE *data, UBYTE len, UWORD time_out_ms,UBYTE autoBaud );

UBYTE iso_state=ISO_UNKNOWN;

UINT16 Baud;

extern UBYTE Lcd_pos;


/******************************************************************
 ** iso_init
 *
 *  DESCRIPTION:
 *		Sets up the ISO interface 
 *		
 *
 *  Create: 7/23/2006	4:23:14 PM - Trampas Stern
 *******************************************************************/
INT iso_init(void)
{
	//Set up the data direction registers 
	BIT_SET(K_OUT_DDR,K_OUT_PIN); //K_OUT pin is output
	BIT_SET(L_OUT_DDR,L_OUT_PIN); //K_OUT pin is output
	BIT_CLEAR(K_IN_DDR,K_IN_PIN); //K_IN pin is input
	K_OUT(1);
	L_OUT(1);
    //setup timer control
	TCCR1A=0x00;  //disable all compare
	TCCR1B=0x02; //clkio/8, no prescaling
		
	return 0;
}

/*
//lets attach header and CRC and send a message
//we will return length of response
UBYTE iso_send(UBYTE *resp, UBYTE len, UBYTE *msg, UBYTE msg_len, UBYTE address)
{
	UBYTE keys[2];
	UBYTE ret,i;
	UBYTE message[20];

	//fprintf_P(&usart_out,PSTR("Connect timer %lu\n\r",connect_timer(0)));
	if (connect_timer(0)>300)
		iso_state=ISO_UNKNOWN;
	if (msg_len>6)
	{
//		printf("Err:ISOmsg<8B\n\r");
		return 0;
	}
	//if we are not connected then lets connect to the ECM
	if (iso_state==ISO_UNKNOWN)
	{
		//call the 5baud connect
		ret=iso_5baud(keys,2,address);

		if (ret>=2)
		{
			//we are connected
			iso_state=ISO_CONNECT;
			//connect_timer(1); //reset timer
		}else
		{
			fprintf_P(&usart_out,PSTR("Err:ISOconnect\n\r"));
			return 0;
		}
	}
	//OK we need to build the message
	message[0]=0x68;
	message[1]=0x6A;
	message[2]=0xF1;

	for(i=0; i<msg_len; i++)
	{
		message[3+i]=msg[i];
	}

	i=3+i;
	message[i]=checksum(message,i);
	i++;

	//now we can send message
	ret=iso_put(message,i,ISO_P3_MIN);
	if(ret!=i)
	{
		fprintf_P(&usart_out,PSTR("Err:ISOmsg send\n\r"));
		iso_state= ISO_UNKNOWN;
		return 0;
	}
	//now lets get response
	ret=iso_get(resp,len,ISO_P3_MAX);

	if (ret)
		connect_timer(1); //reset connection timer

	return ret;
}
	

UBYTE iso_get(UBYTE *data, UBYTE len, UWORD time_out_ms)
{
  	return iso_getAuto(data,len,time_out_ms,0);
}
*/
 
//Well I need an Async get char function
UBYTE iso_getAuto(UBYTE *data, UBYTE len, UWORD time_out_ms,UBYTE autoBaud )
{
	UBYTE temp;	
	UBYTE index;
	TIME t;
	UINT8 i;

 	index=0;
	K_OUT(1);	   //make sure bus is high... 
	
	while(index<len)
	{
	/*
		GetTime(&t);
		while(K_IN==0)
		{
			if ((UINT16)GetElaspMs(&t)>ISO_P2_MAX )
			{
				switch_lcd_wait();
				fprintf_P(&lcd_out,PSTR("Err:ISOgetbusy"));
				_delay_ms(200);
				fprintf_P(&usart_out,PSTR("Err:ISOget busy\n\r"));
				return index;
			}
		}	
	*/
	
	//Get baud rate using Autobaud, then read in the message
		GetTime(&t);

		while(K_IN==1) //K-line high in default state
		{
			if ((UINT16)GetElaspMs(&t)>time_out_ms )
			{
				if (index==0)
				{
					//switch_lcd_wait();
	//				fprintf_P(&lcd_out,PSTR("No start bit"));
					_delay_ms(200);
	//				fprintf_P(&usart_out,PSTR("No start bit\n\r"));
				}
				return index;
			}
		}
		TCNT1=0;		
		while(K_IN==0);	//do nothing, move on when K goes high/end of start bit
		if (autoBaud == 1)
		{
			Baud=TCNT1;
		}
		TCNT1=0;	
		while(TCNT1<(0.5*Baud));//move 1/2 way through 1st bit
	
		//now clock in bits	
		temp=0;
		for(i=0; i<8; i++)
		{
			temp=temp>>1;
			if (K_IN)
			{
				temp=temp | 0x80;
			}
			TCNT1=0;		
			while(TCNT1<Baud);//move to half way of second bit
		}	   
		*data=temp;
		//data++; not needed first run
		index++;
		time_out_ms=ISO_W2_MAX;
	}
	//	switch_lcd_wait();
	//	fprintf_P(&lcd_out,PSTR("aBaud cnt:%u \n\r"), Baud );
		_delay_ms(200);
	//	fprintf_P(&usart_out,PSTR("Timer autoBaud count = %u \n\r"), Baud );
		return index;
}

UBYTE iso_getb(UBYTE *data, UBYTE len, UWORD time_out_ms)
{
	UBYTE temp;	
	UBYTE index;
	UINT8 i;

 	index=0;
	K_OUT(1);	   //make sure bus is high... 

	while(index<len)
	{
		while(K_IN==1); //K-line high in default state
		TCNT1=0;		
		while(TCNT1<69);//move 1/2 way through 1st bit
		//now clock in bits	
		temp=0;
		for(i=0; i<8; i++)
		{
			temp=temp>>1;
			if (K_IN)
			{
				temp=temp | 0x80;
			}
			TCNT1=0;		
			while(TCNT1<46);//move to half way of second bit
		}	   
		*data=temp;
		//data++; not needed first run
		index++;
	}
		return index;
}

		
//sends out a char at 5-baud rate
UWORD iso_5baud_putc(UBYTE data)
{ 
	UBYTE i;

	//first drive low for start bit
	K_OUT(0);
	L_OUT(0);
	_delay_ms(764);//200ms
	for(i=0; i<8; i++)
	{
		if (data & 0x01)
		{
			K_OUT(1);
			L_OUT(1);
		} else
		{
			K_OUT(0);
			L_OUT(0);
		}
		data=data>>1;
		_delay_ms(764);//200ms
	}
	//output stop bit
	K_OUT(1);
	L_OUT(1);
	_delay_ms(764);//200ms stop bit

	return 0;
}
	
			
/*
//preform 5 baud initlization
//returns the time for the baud rate
UWORD iso_5baud(UBYTE *keys, UBYTE len, UBYTE address)
{
	UBYTE data;
	UBYTE i,temp;
	TIME t;
	

	//first make sure that the bus is high for at least W0
	while(GetElaspMs(&t)<=ISO_W0_MIN)
	{
		if (K_IN==0)
			return 0;
	}
	fprintf_P(&usart_out,PSTR("5Bd addr=%x\n\r"),address);
	iso_5baud_putc(address);

	data=0;
	temp=iso_getAuto(&data,1,ISO_W1_MAX*2,1); //auto baud
	//printf("got %d %x\n\r",temp,data);
	if (data!=0x55)
	{
		fprintf_P(&usart_out,PSTR("Bad 5Bd ini %u %X\n\r"),(int)temp,(int)data);  
		return 0;
	}

	//TODO handle none irregular baud rates...
	data=iso_get(keys,len,ISO_W2_MAX);
	if (data>=2)
	{
		temp=~keys[1];
		iso_put(&temp,1,ISO_W4_MIN);  
		if (address==0x33)
		{
			temp=0;
			i=iso_get(&temp,1,ISO_W4_MAX*2);
			if (i==0 ||  temp!=0xCC)
			{
//				fprintf_P(&usart_out,PSTR("Err:ISO 5 baud fail 0x%X 0x%X %d\n\r"),keys[1],temp,i);
				return 0; 
			}
		}
	} else
	{
		fprintf_P(&usart_out,PSTR("Bad keys\n\r"));
	}	 
	iso_state=ISO_CONNECT;
	return data;
}

*/
UBYTE iso_putb(UBYTE *data, UBYTE len, UWORD idle_wait)
{
	UBYTE bits;
	UBYTE temp;
	UBYTE index;
	TIME t;
	//make sure bus is high
	GetTime(&t);
	while(K_IN==0);		  
	while((UINT16)GetElaspMs(&t)<idle_wait)
	{
		if (!K_IN)
		{
			//GetTime(&t);
//			fprintf_P(&usart_out,PSTR("Err:ISO Put\n\r"));
			return 0;
		}
   	}

    index=0;
	while(index<len)
	{
		UBYTE mask;
		temp=data[index];
		mask=0x01;
		//drive with start bit, then send data
		K_OUT(0);
   		TCNT1=0;
		while(TCNT1<46);
		TCNT1=0;
		for (bits=0; bits<8; bits++)
		{
			if (temp & mask)
			{
				K_OUT(1);
			}else
			{
				K_OUT(0);
			}		
			mask=mask<<1;
			while(TCNT1<46);
			TCNT1=0;
		}
		//now we need to send stop bit
		K_OUT(1);
		TCNT1=0;
		while(TCNT1<46);
		index++;
	}
	return index;
}
/*
UBYTE iso_put(UBYTE *data, UBYTE len, UWORD idle_wait)
{
	UBYTE bits;
	UBYTE temp;
	UBYTE index;
	TIME t;
		
	//setup timer control
	TCCR1A=0x00;  //disable all compare
	TCCR1B=0x0; //clkio/1


	//make sure bus is high
	GetTime(&t);

	while(K_IN==0);		  
			  
	while((UINT16)GetElaspMs(&t)<idle_wait)
	{
		if (!K_IN)
		{
			//GetTime(&t);
			fprintf_P(&usart_out,PSTR("Err:ISO Put\n\r"));
			return 0;
		}
   	}
	//fprintf_P(&usart_out,PSTR("ms timer %lu\n\r",timer_ms));

	//for each bit.... 
    index=0;
	while(index<len)
	{
		UBYTE mask;
		temp=data[index];
		mask=0x01;
		//drive with start bit, then send data
		K_OUT(0);
   		TCNT1=0;
		while(TCNT1<Baud);
		TCNT1=0;
		for (bits=0; bits<8; bits++)
		{
			if (temp & mask)
			{
				K_OUT(1);
			}else
			{
				K_OUT(0);
			}		
			mask=mask<<1;
			while(TCNT1<Baud);
			TCNT1=0;
		}

		//now we need to send stop bit
		K_OUT(1);
		TCNT1=0;
		while(TCNT1<Baud);

		index++;
		if(index>=len)
		{
			break;
		}
		
		//now wait to send next byte
		GetTime(&t);
		while((UINT16)GetElaspMs(&t)<ISO_P4_MIN)
		{
			if (!K_IN)
			{
//				fprintf_P(&usart_out,PSTR("Err:ISO put fail2\n\r"));
								return 0;
			}

		}

	}
	return index;
}
		
*/
	
		


