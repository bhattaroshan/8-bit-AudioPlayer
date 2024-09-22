

#ifdef F_CPU 
#undef F_CPU
#endif

#define F_CPU 8000000UL

#include<avr/io.h>
#include<util/delay.h>
#include<avr/interrupt.h>
#include"pff.h"
#include"diskio.h"
#include"lcd.h"

FATFS fs;
FILINFO fno;
DIR dir;
BYTE res;
char buffer1[512],buffer2[512];
WORD br;

#define PRINT(MSG) LCDGotoXY(0,0);LCDWriteString(MSG)
#define EXIT return 0

volatile uint8_t counter=0;
volatile uint16_t song_cnt=0;
volatile uint8_t start_reading=1;

volatile uint8_t bfck=1,bfwh=1,first_run=0;

uint32_t readsize=0,totalsize=0;
uint16_t length=0,currlength=0,prevlength=0;

uint8_t tmin,tsec,cmin,csec;

uint16_t filecounter=0;

#define PLAYING 0
#define PAUSED  1
#define STOPPED 2

uint8_t bStatus=PLAYING;

uint8_t volume=0;

void display_info()
{
 		  totalsize=fno.fsize;
		  length=totalsize/16000;
		  tmin=length/60;
		  tsec=length-(tmin*60);

		  LCDClear();
		  LCDGotoXY(0,0);
		  LCDWriteString(fno.fname);
		  
		  LCDGotoXY(14,3);
		  LCDWriteString("/");
 	 	  LCDWriteInt(tmin,2);
		  LCDWriteString(":");
		  LCDWriteInt(tsec,2);
}


void rewind()
{
 	uint16_t tempcounter;
	
	TIMSK&=~(1<<TOIE0);
    song_cnt=0;
    counter=0;
	readsize=0;
	start_reading=1;
	first_run=0;
	bfck=1;
	bfwh=1;

	if(filecounter==1)
	 filecounter--;
	else
	 filecounter-=2;

	pf_opendir(&dir,"/");
	pf_readdir(&dir,&fno);

	for(tempcounter=0;tempcounter<filecounter;tempcounter++)
	 {
	 	pf_readdir(&dir,&fno);		
	 }

	 pf_open(fno.fname);
	 
	 if(fno.fname[0])
		 {
		    filecounter=tempcounter+1;
			display_info();
			LCDGotoXY(0,1);
			LCDWriteString("Song No:");
			LCDWriteInt(filecounter,2);
		 }

		if(bStatus!=STOPPED)
	    	TIMSK|=(1<<TOIE0);
}


void change_song()
{
 		TIMSK&=~(1<<TOIE0);
	    song_cnt=0;
	    counter=0;
		readsize=0;
		start_reading=1;
		first_run=0;
		bfck=1;
		bfwh=1;

	    pf_readdir(&dir,&fno);
		
		if(fno.fname[0])
		 {
			pf_open(fno.fname);
		 }
		else
		 {
		  pf_opendir(&dir,"/");
		  pf_open(fno.fname);
		  filecounter=0;
		 }
		
		if(fno.fname[0])
		 {
		    filecounter++;
			display_info();
			LCDGotoXY(0,1);
			LCDWriteString("Song No:");
			LCDWriteInt(filecounter,2);
		 }

		if(bStatus!=STOPPED)
	    	TIMSK|=(1<<TOIE0);
}

int main()
{
 PORTA=0xFF;
 DDRA=0x00;
 PORTB = 0b00001111;  //enable pullups
 DDRB  = 0b00000000;  //let all be input first doesn't matter
 
 /*
  PB0---play/pause
  PB1---stop
  PB2---previous track
  PB3---next track
 */

 DDRD|=(1<<PD5);

  InitLCD(0);
  _delay_ms(50);
  LCDCmd(0b00001100);
  LCDClear();

 	res = disk_initialize();  //init disk
  	_delay_ms(400); //delay sometime for init



 if(res!=FR_OK)
  {
   PRINT("init failed");
   EXIT;
  }
 
 res=pf_mount(&fs);  //mount
 
 if(res!=FR_OK)
  {
   PRINT("mount failed");
   EXIT;
  }
 
 res=pf_opendir(&dir,"/");  //open root

 if(res!=FR_OK)
  {
   PRINT("no root");
   EXIT;
  }	
 
 TCCR0=(1<<CS00);
 TIMSK&=~(1<<TOIE0);

 TCCR1A=(1<<WGM10)|(1<<COM1A1);
 TCCR1B=(1<<WGM12)|(1<<CS10);
 OCR1A=0;

 sei();

 while(1)
  {
   if(start_reading==1)
    {
	 		//cli();

			if(bfwh)
			   {
	   	    	pf_read(buffer1,sizeof(buffer1),&br);
			   }
			else
			   {
			    pf_read(buffer2,sizeof(buffer2),&br);
			   }

			bfwh^=1;

			readsize+=br;

	   		if(readsize>=totalsize)
	    		{
		 			change_song();
				}

	 		start_reading=0;
	 		first_run=1;

			//sei();
	}
   else if(!(PINB & (1<<PB0)))
    {
			cli();
			
			while(!(PINB & (1<<PB0)));
	    	_delay_ms(150);

			if(TIMSK&(1<<TOIE0)) //pause it
			 {
			  OCR1A=0; 			//set duty 0
			  TIMSK&=~(1<<TOIE0);
			  
			  if(bStatus!=STOPPED)
			   bStatus=PAUSED;
			 }
			else // play it
			 {
			  if(bStatus==STOPPED)
			   {
			    display_info();
			   }
			  TIMSK|=(1<<TOIE0);
			  bStatus=PLAYING;
			 }

			sei();
	}
   else if(!(PINB & (1<<PB1)))
    {
			cli();

			while(!(PINB & (1<<PB1)));
	    	_delay_ms(150);
			
			bStatus=STOPPED;
			OCR1A=0; 			//set duty 0
			TIMSK&=~(1<<TOIE0); //stop song
	    	song_cnt=0;
	    	counter=0;
			readsize=0;
			start_reading=1;
			pf_open(fno.fname);

			LCDGotoXY(9,3);
			LCDWriteString("00:00/00:00");

			sei();
	}
   else if(!(PINB & (1<<PB2)))
    {
	  	 	cli();
				
			while(!(PINB&(1<<PB2)));
			_delay_ms(150);

			rewind();

			sei();
	}
   else if(!(PINB & (1<<PB3)))
	{
	  	
			cli();

	    	while(!(PINB & (1<<PB3)));
	    	_delay_ms(150);

	    	change_song();

			sei();
	}
   else if(!(PINA & (1<<PA0))) //down
	{
		cli();

	  	while(!(PINA & (1<<PA0)));
	    	_delay_ms(150);
		
		volume++;
		LCDGotoXY(0,2);
		LCDWriteInt(volume,2);

		sei();
	}
	else if(!(PINA & (1<<PA1))) //up
	{
		cli();

	  	while(!(PINA & (1<<PA1)));
	    	_delay_ms(150);
		
		if(volume>=0)
				volume--;

		LCDGotoXY(0,2);
		LCDWriteInt(volume,2);

		sei();
	}
   else
   	{
	  		prevlength=currlength;
	 		currlength=readsize/16000;
	  		if(prevlength!=currlength)
	   			{
	    			cmin=currlength/60;
					csec=currlength-(cmin*60);
	  				LCDGotoXY(9,3);
	  				LCDWriteInt(cmin,2);
	  				LCDWriteString(":");
					LCDWriteInt(csec,2);
	   			}
	}	

  }

 return 0;
}

ISR(TIMER0_OVF_vect)
{

 counter++;

 if(counter>=2)
  {
   counter=0;

   if(start_reading==0 || first_run)
    {
	  if(bfck)
       OCR1A=(uint8_t)buffer1[song_cnt]>>volume;
	  else
	   OCR1A=(uint8_t)buffer2[song_cnt]>>volume;

       song_cnt++;
	}
   
   if(song_cnt==1)
    {
	 start_reading=1;
	}
   else if(song_cnt>511)
    {
	 bfck^=1;
     song_cnt=0;
	}
  }

}
