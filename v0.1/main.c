/*
** main.c
** [currently deployed]
** works with book9.xlsm
*/

 
#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <inttypes.h>
#include <stdio.h>
#include "lcd.h"
#include "iso.h"
#include "usart.h"
#include "time.h"
#include "switchio.h"
#include "datatypes.h"



/********************************************************************************
Global Variables
********************************************************************************/

static FILE usart_out = FDEV_SETUP_STREAM(usart_putchar_printf, usart_getchar_printf, _FDEV_SETUP_RW);
static FILE lcd_out = FDEV_SETUP_STREAM(lcd_chr_printf, NULL, _FDEV_SETUP_WRITE);



// PIN DEFINITIONS:
// PC1 -- WHITE LED
// PC2 -- RED LED
// PC3 -- GREEN LED
// PC4 -- PUSH BUTTON
 
// Global menu data
#define maxMenus 10
#define maxItems 6
#define pageSize 5 // available lines minus title line
 
uint8_t curMenu = 0; // menu currently shown
uint8_t curItem = 0; // item currently marked (line 0 for menutitle, so starts on one). NB! Item marked may not correspond to line marked (see cursorCount)
uint8_t updateFlag = 0; // used to register when there has been an update to the menu (moved around) and wait a little before allowing user input
uint8_t cursorCount = 0;  // keeps track of white line of menu the cursor is on. Since there may be multiple pages, this may not be the same as the item marked in the menu
uint8_t menuCount = 0;  // keeps track of what line of menu to print
uint8_t pageScroll = 0; // for far have we scrolled down - pageSize items on each page.
uint8_t menuprintloop = 0; // keep track of numbers of printed items
uint8_t switchtype = 0;
double value;
uint16_t i = 0;
unsigned int eeprom_address;
unsigned int eeprom_found;
double eeprom_data;
double gas_price;
double vol_eff;
uint8_t *car_id;
uint8_t vehicle_type = 0;
uint8_t show_trip = 3;
char *stream;
int elapsed_hours = 0;
int elapsed_mins = 0;
int elapsed_secs = 0;

UBYTE temp;	
UBYTE Lcd_pos;
TIME t;

double MPG_ave;
double MPG_temp;
double speed_ave;
double running_dist;
double running_cost;
double running_time;
double running_time_min;
double running_gallons;
double MPG_ave_trip;
double speed_ave_trip;
double running_dist_trip;
double running_cost_trip;
double running_time_trip;
double running_time_min_trip;
double running_gallons_trip;
double dt_seconds;
double MAF_temp;
double rpm_temp;
double IAT_temp;
double VSS_temp;
int ping_sum;
int ping_length;

UBYTE connect_put[2];
UBYTE connect_get[4];

UBYTE ping_put[15];
UBYTE ping_get[15];

UBYTE PID_put[6];
UBYTE PID_get[10];

UBYTE err_put[6];
UBYTE err_get[10];

UBYTE temp_put[6];
UBYTE temp_get[7];

UBYTE STFT_put[6];
UBYTE STFT_get[7];

UBYTE LTFT_put[6];
UBYTE LTFT_get[7];

UBYTE load_put[6];
UBYTE load_get[7];

UBYTE thrott_put[6];
UBYTE thrott_get[7];

UBYTE timing_put[6];
UBYTE timing_get[7];

UBYTE codes_put[5];
UBYTE codes_get[11];

UBYTE clear_put[5];

UBYTE RPM_put[6];
UBYTE RPM_get[8];

UBYTE IAT_put[6];
UBYTE IAT_get[7];

UBYTE VSS_put[6];
UBYTE VSS_get[7];

UBYTE MAF_put[6];
UBYTE MAF_get[8];

UBYTE MAP_put[6];
UBYTE MAP_get[7];

char menutitle[maxMenus][14];
char menuitem[maxMenus][maxItems][7];
uint16_t menulink[maxMenus][maxItems]; // a link to a submenu OR
uint16_t menuactn[maxMenus][maxItems]; // menu actions - turn on LED or whatever


void initMenu(void) {
    strcpy(menutitle[0], "<<SEL SCROLL>>");
    strcpy(menuitem[0][0], "MPG");
    menulink[0][0] = 1; // link to menutitle[1];
    menuactn[0][0] = 0; // No action - just a sub menu
    strcpy(menuitem[0][1], "DATA");
    menulink[0][1] = 2;
    menuactn[0][1] = 0;
    strcpy(menuitem[0][2], "PIDS");
    menulink[0][2] = 3;
    menuactn[0][2] = 0;
    strcpy(menuitem[0][3], "CEL");
    menulink[0][3] = 4;
    menuactn[0][3] = 0;
    strcpy(menuitem[0][4], "PROG");
    menulink[0][4] = 5;
    menuactn[0][4] = 0;
    strcpy(menuitem[0][5], "\0"); // Need to initialize the end str of array
 
    strcpy(menutitle[1], "MPG");
    strcpy(menuitem[1][0], "RUN");
    menulink[1][0] = 0; // no sub menu, just action
    menuactn[1][0] = 1;
    strcpy(menuitem[1][1], "SAVE");
    menulink[1][1] = 6;
    menuactn[1][1] = 0;
    strcpy(menuitem[1][2], "SHOW");
    menulink[1][2] = 7;
    menuactn[1][2] = 0;
	strcpy(menuitem[1][3], "CLEAR");
    menulink[1][3] = 8;
    menuactn[1][3] = 0;
    strcpy(menuitem[1][4], "//");
    menulink[1][4] = 0;
    menuactn[1][4] = 999;
    strcpy(menuitem[1][5], "\0"); // Need to initialize the end str of array
 
    strcpy(menutitle[2], "DATA");
    strcpy(menuitem[2][0], "SET1");
    menulink[2][0] = 0; // no sub menu, just action
    menuactn[2][0] = 2;
	strcpy(menuitem[2][1], "SET2");
    menulink[2][1] = 0; // no sub menu, just action
    menuactn[2][1] = 3;
    strcpy(menuitem[2][2], "//");
    menulink[2][2] = 0;
    menuactn[2][2] = 999;
    strcpy(menuitem[2][3], "\0"); // Need to initialize the end str of array
 
    strcpy(menutitle[3], "PIDS");
    strcpy(menuitem[3][0], "PIDS");
    menulink[3][0] = 0; // no sub menu, just action
    menuactn[3][0] = 4;
	strcpy(menuitem[3][1], "INIT");
    menulink[3][1] = 0; // no sub menu, just action
    menuactn[3][1] = 5;
	strcpy(menuitem[3][2], "ID");
    menulink[3][2] = 0; // no sub menu, just action
    menuactn[3][2] = 6;
    strcpy(menuitem[3][3], "//");
    menulink[3][3] = 0;
    menuactn[3][3] = 999;
    strcpy(menuitem[3][4], "\0"); // Need to initialize the end str of array


    strcpy(menutitle[4], "CEL CODES");
    strcpy(menuitem[4][0], "#");
    menulink[4][0] = 0; // no sub menu, just action
    menuactn[4][0] = 7;
    strcpy(menuitem[4][1], "GET");
    menulink[4][1] = 0;
    menuactn[4][1] = 8;
	strcpy(menuitem[4][2], "CLEAR");
    menulink[4][2] = 0;
    menuactn[4][2] = 9;
    strcpy(menuitem[4][3], "//");
    menulink[4][3] = 0; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[4][3] = 999;
    strcpy(menuitem[4][4], "\0"); // Need to initialize the end str of array
 
    strcpy(menutitle[5], "PROG");
    strcpy(menuitem[5][0], "GAS $");
    menulink[5][0] = 0; // no sub menu, just action
    menuactn[5][0] = 10;
    strcpy(menuitem[5][1], "VR");
    menulink[5][1] = 0;
    menuactn[5][1] = 11;
    strcpy(menuitem[5][2], "SERIAL");
    menulink[5][2] = 9; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[5][2] = 0;
    strcpy(menuitem[5][3], "//");
    menulink[5][3] = 0; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[5][3] = 999;
    strcpy(menuitem[5][4], "\0"); // Need to initialize the end str of array

    strcpy(menutitle[6], "SAVE");
    strcpy(menuitem[6][0], "ALL");
    menulink[6][0] = 0; // no sub menu, just action
    menuactn[6][0] = 12;
	strcpy(menuitem[6][1], "DAY");
    menulink[6][1] = 0; // no sub menu, just action
    menuactn[6][1] = 13;
	strcpy(menuitem[6][2], "TRIP");
    menulink[6][2] = 0; // no sub menu, just action
    menuactn[6][2] = 14;
    strcpy(menuitem[6][3], "DBASE");
    menulink[6][3] = 0; // no sub menu, just action
    menuactn[6][3] = 15;
	strcpy(menuitem[6][4], "//");
    menulink[6][4] = 0; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[6][4] = 999;
    strcpy(menuitem[6][5], "\0"); // Need to initialize the end str of array    
 

    strcpy(menutitle[7], "SHOW");
    strcpy(menuitem[7][0], "DAY");
    menulink[7][0] = 0; // no sub menu, just action
    menuactn[7][0] = 16;
	strcpy(menuitem[7][1], "TRIP");
    menulink[7][1] = 0; // no sub menu, just action
    menuactn[7][1] = 17;
    strcpy(menuitem[7][2], "//");
    menulink[7][2] = 0; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[7][2] = 999;
    strcpy(menuitem[7][3], "\0"); // Need to initialize the end str of array    

    strcpy(menutitle[8], "CLEAR");
    strcpy(menuitem[8][0], "TRIP");
    menulink[8][0] = 0; // no sub menu, just action
    menuactn[8][0] = 18;
	strcpy(menuitem[8][1], "DBASE");
    menulink[8][1] = 0; // no sub menu, just action
    menuactn[8][1] = 19;
    strcpy(menuitem[8][2], "//");
    menulink[8][2] = 0; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[8][2] = 999;
    strcpy(menuitem[8][3], "\0"); // Need to initialize the end str of array    

    strcpy(menutitle[9], "SERIAL");
    strcpy(menuitem[9][0], "MEM2XL");
    menulink[9][0] = 0; // no sub menu, just action
    menuactn[9][0] = 20;
	strcpy(menuitem[9][1], "OUT2XL");
    menulink[9][1] = 0; // no sub menu, just action
    menuactn[9][1] = 21;
    strcpy(menuitem[9][2], "PING");
    menulink[9][2] = 0;
    menuactn[9][2] = 22;
    strcpy(menuitem[9][3], "COMM");
    menulink[9][3] = 0; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[9][3] = 23;
    strcpy(menuitem[9][4], "//");
    menulink[9][4] = 0; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[9][4] = 999;
    strcpy(menuitem[9][5], "\0"); // Need to initialize the end str of array    
    // END of menu data
}


void clean_exit_partial(void){
	lcd_goto_xy(1,6);
		fprintf_P(&lcd_out, PSTR("PRESS KEY"));
			while (1) {
				if (switch_is_pressed(&switchtype)) {
				break;
				}
			}   
}

void clean_exit_full(void){
    lcd_clear();
	lcd_goto_xy(1,1);
	fprintf_P(&lcd_out,PSTR("OK"));
	clean_exit_partial();
}

void clean_start(void){
    lcd_clear();
	lcd_goto_xy(1,1);
	fprintf_P(&lcd_out,PSTR("OK"));
	lcd_goto_xy(1,2);
}



void parse_time(double time_in_min, int *hours, int *mins, int *secs) {
double temp_min;
double temp_secs;

*hours = (int) (time_in_min/60);

temp_min = time_in_min - (double) 60.0 * (*hours);
*mins = (int) temp_min;

temp_secs = 60.0*temp_min -  60.0 *(double) (*mins);
*secs = (int) temp_secs;

}

 
void showMenu(void) {
    lcd_clear();
	lcd_goto_xy(1,1);
    fprintf_P(&lcd_out, PSTR("%s"), menutitle[curMenu]);
    //print menu content
    while ((menuprintloop < (pageSize)) && (menuitem[curMenu][menuCount][0] != '\0')) {
        lcd_goto_xy(1, menuprintloop+2); // +1 to leave first line for menu title
        if ((cursorCount + (pageSize*pageScroll)) == menuCount) {
            // item currently indicated by cursor
            fprintf_P(&lcd_out, PSTR(">%s"), menuitem[curMenu][menuCount]);
        } else {
            fprintf_P(&lcd_out, PSTR(" %s"), menuitem[curMenu][menuCount]);
        }
        menuprintloop++;
        menuCount++;
    }
    menuprintloop = 0;
}
 
void ISO_init_comm(uint8_t show) {
   lcd_clear();
   lcd_goto_xy(1,1);
   fprintf_P(&lcd_out, PSTR("INIT"));
// send 33, resp str should contain 55
	connect_put[0]=0x33;	
//  switch_lcd_wait();
	iso_5baud_putc(connect_put[0]);
	for (i =0; i<3; i++)
	 {temp = iso_getb(&connect_get[i],1, ISO_W1_MAX*2);}
	connect_put[1] = ~connect_get[2];
	temp = iso_putb(&connect_put[1],1, ISO_W4_MIN);
	temp = iso_getb(&connect_get[3],1, ISO_W4_MAX*2);

	if(show ==1) {
		lcd_clear();
		lcd_goto_xy(1,1);
		fprintf_P(&lcd_out,PSTR("INIT"), connect_put[0]);
		lcd_goto_xy(1,2);
		fprintf_P(&lcd_out,PSTR("0x%x"), connect_put[0]);
		lcd_goto_xy(1,3);
		fprintf_P(&lcd_out,PSTR("%x..%x %x"), connect_get[0], connect_get[1], connect_get[2]);
		lcd_goto_xy(1,4);
		fprintf_P(&lcd_out,PSTR("%x..%x"), connect_put[1], connect_get[3]);
		clean_exit_partial();
	}	
}

  void set_data(void){
    PID_put[0] = 0x68; 
	PID_put[1] = 0x6A;
	PID_put[2] = 0xF1;
	PID_put[3] = 0x01; 
	PID_put[4] = 0x00;
	PID_put[5] = 0xC4;

    err_put[0] = 0x68; 
	err_put[1] = 0x6A;
	err_put[2] = 0xF1;
	err_put[3] = 0x01; 
	err_put[4] = 0x01;
	err_put[5] = 0xC5;
	
	codes_put[0] = 0x68; 
	codes_put[1] = 0x6A;
	codes_put[2] = 0xF1;
	codes_put[3] = 0x03; 
	codes_put[4] = 0xC6;

    clear_put[0] = 0x68; 
	clear_put[1] = 0x6A;
	clear_put[2] = 0xF1;
	clear_put[3] = 0x04; 
	clear_put[4] = 0xC7;
	    
    thrott_put[0] = 0x68; 
	thrott_put[1] = 0x6A;
	thrott_put[2] = 0xF1;
	thrott_put[3] = 0x01; 
	thrott_put[4] = 0x11;
	thrott_put[5] = 0xD5;

    load_put[0] = 0x68; 
	load_put[1] = 0x6A;
	load_put[2] = 0xF1;
	load_put[3] = 0x01; 
	load_put[4] = 0x04;
	load_put[5] = 0xC8;

    timing_put[0] = 0x68; 
	timing_put[1] = 0x6A;
	timing_put[2] = 0xF1;
	timing_put[3] = 0x01; 
	timing_put[4] = 0x0E;
	timing_put[5] = 0xD2;

    STFT_put[0] = 0x68; 
	STFT_put[1] = 0x6A;
	STFT_put[2] = 0xF1;
	STFT_put[3] = 0x01; 
	STFT_put[4] = 0x06;
	STFT_put[5] = 0xCA;
	
	LTFT_put[0] = 0x68; 
	LTFT_put[1] = 0x6A;
	LTFT_put[2] = 0xF1;
	LTFT_put[3] = 0x01; 
	LTFT_put[4] = 0x07;
	LTFT_put[5] = 0xCB;
	
    temp_put[0] = 0x68; 
	temp_put[1] = 0x6A;
	temp_put[2] = 0xF1;
	temp_put[3] = 0x01; 
	temp_put[4] = 0x05;
	temp_put[5] = 0xC9;

    IAT_put[0] = 0x68; 
	IAT_put[1] = 0x6A;
	IAT_put[2] = 0xF1;
	IAT_put[3] = 0x01; 
	IAT_put[4] = 0x0F;
	IAT_put[5] = 0xD3;
	
	VSS_put[0] = 0x68; 
	VSS_put[1] = 0x6A;
	VSS_put[2] = 0xF1;
	VSS_put[3] = 0x01; 
	VSS_put[4] = 0x0D;
	VSS_put[5] = 0xD1;
	
	RPM_put[0] = 0x68; 
	RPM_put[1] = 0x6A;
	RPM_put[2] = 0xF1;
	RPM_put[3] = 0x01; 
	RPM_put[4] = 0x0C;
	RPM_put[5] = 0xD0;
    
    MAP_put[0] = 0x68; 
	MAP_put[1] = 0x6A;
	MAP_put[2] = 0xF1;
	MAP_put[3] = 0x01; 
	MAP_put[4] = 0x0B;
	MAP_put[5] = 0xCF;

    MAF_put[0] = 0x68; 
	MAF_put[1] = 0x6A;
	MAF_put[2] = 0xF1;
	MAF_put[3] = 0x01; 
	MAF_put[4] = 0x10;
	MAF_put[5] = 0xD4;
  }

  void get_car_type (uint8_t *car_id) { 
	ISO_init_comm(0);
   	temp = iso_putb(&PID_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	{temp = iso_putb(&PID_put[i],1, ISO_P4_MIN);}

	temp = iso_getb(&PID_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<10; i++)
	{temp = iso_getb(&PID_get[i],1, ISO_W2_MAX*2);}
	
	if (PID_get[6] & 0x01) { //supports MAF ... van
    *car_id = 1;
	}
	else {
	*car_id = 0;
	}
} 

 
void get_supported_PIDs (void) {
    lcd_clear();
	get_car_type(&vehicle_type);
   	temp = iso_putb(&PID_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	{temp = iso_putb(&PID_put[i],1, ISO_P4_MIN);}

	temp = iso_getb(&PID_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<10; i++)
	{temp = iso_getb(&PID_get[i],1, ISO_W2_MAX*2);}
	clean_start();
	fprintf_P(&lcd_out,PSTR("%x %x %x %x"), PID_get[5], PID_get[6],PID_get[7],PID_get[8]);		
	clean_exit_partial();
	
} 
 
void get_num_error_codes (void) {
    lcd_clear();
	get_car_type(&vehicle_type);
	temp = iso_putb(&err_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	{temp = iso_putb(&err_put[i],1, ISO_P4_MIN);}
	
	temp = iso_getb(&err_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<10; i++)
	{temp = iso_getb(&err_get[i],1, ISO_W2_MAX*2);}
	
	clean_start();
	fprintf_P(&lcd_out,PSTR("%x %x %x %x"), err_get[5], err_get[6],err_get[7],err_get[8]);
	lcd_goto_xy(1,3);
	fprintf_P(&lcd_out,PSTR("# CODES SET: %d"),(err_get[5] & 0x7F));
	lcd_goto_xy(1,4);
	if (((err_get[5]>>7) & 0x01) == 0x01) {
	fprintf_P(&lcd_out,PSTR("CEL ON"));}
	else {
	fprintf_P(&lcd_out,PSTR("CEL OFF"));}
    clean_exit_partial();
}

void get_error_codes (void) {
    lcd_clear();
	get_car_type(&vehicle_type);
		
	temp = iso_putb(&codes_put[0],1, ISO_P3_MIN);
	for (i =1; i<5; i++)
	{temp = iso_putb(&codes_put[i],1, ISO_P4_MIN);}
	
	temp = iso_getb(&codes_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<11; i++)
	{temp = iso_getb(&codes_get[i],1, ISO_W2_MAX*2);}
	
	clean_start();
	fprintf_P(&lcd_out,PSTR("%x %x"), codes_get[4], codes_get[5]);
	lcd_goto_xy(1,3);
	fprintf_P(&lcd_out,PSTR("%x %x"), codes_get[6], codes_get[7]);
	lcd_goto_xy(1,4);
	fprintf_P(&lcd_out,PSTR("%x %x"), codes_get[8], codes_get[9]);
    clean_exit_partial();
}

void clear_codes (void) {
    lcd_clear();
    get_car_type(&vehicle_type);
	temp = iso_putb(&clear_put[0],1, ISO_P3_MIN);
	for (i =1; i<5; i++)
	{temp = iso_putb(&clear_put[i],1, ISO_P4_MIN);}
   	clean_exit_full();
}
 
 
 void get_data_set1(void) {

    lcd_clear();
	get_car_type(&vehicle_type);
	lcd_goto_xy(1,1);
    fprintf_P(&lcd_out,PSTR("iSPD       mph"));
	lcd_goto_xy(1,2);
	fprintf_P(&lcd_out,PSTR(" RPM"));
	lcd_goto_xy(1,3);
	fprintf_P(&lcd_out,PSTR("PEDL         %%"));
	lcd_goto_xy(1,4);
	fprintf_P(&lcd_out,PSTR("LOAD         %%"));
	lcd_goto_xy(1,5);
	fprintf_P(&lcd_out,PSTR("Teng         C"));
	lcd_goto_xy(1,6);
	fprintf_P(&lcd_out,PSTR(" TDC       deg"));

 
 while(1) {
    if (switch_is_pressed(&switchtype)) {
		break;
	}
	else {
 	
	temp = iso_putb(&VSS_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	 {temp = iso_putb(&VSS_put[i],1, ISO_P4_MIN);}
    
	temp = iso_getb(&VSS_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<7; i++)
	{temp = iso_getb(&VSS_get[i],1, ISO_W2_MAX*2);}
	
    temp = iso_putb(&RPM_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	 {temp = iso_putb(&RPM_put[i],1, ISO_P4_MIN);}
     
    temp = iso_getb(&RPM_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<8; i++)
	{temp = iso_getb(&RPM_get[i],1, ISO_W2_MAX*2);}
	
	temp = iso_putb(&thrott_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	 {temp = iso_putb(&thrott_put[i],1, ISO_P4_MIN);}
    
	temp = iso_getb(&thrott_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<7; i++)
	{temp = iso_getb(&thrott_get[i],1, ISO_W2_MAX*2);}

	temp = iso_putb(&load_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	 {temp = iso_putb(&load_put[i],1, ISO_P4_MIN);}
    
	temp = iso_getb(&load_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<7; i++)
	{temp = iso_getb(&load_get[i],1, ISO_W2_MAX*2);}

	temp = iso_putb(&temp_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	 {temp = iso_putb(&temp_put[i],1, ISO_P4_MIN);}
    
	temp = iso_getb(&temp_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<7; i++)
	{temp = iso_getb(&temp_get[i],1, ISO_W2_MAX*2);}
	
	temp = iso_putb(&timing_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	 {temp = iso_putb(&timing_put[i],1, ISO_P4_MIN);}
    
	temp = iso_getb(&timing_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<7; i++)
	{temp = iso_getb(&timing_get[i],1, ISO_W2_MAX*2);}
	
	lcd_goto_xy(6,1);
	fprintf_P(&lcd_out,PSTR("%-5.1f"), (double)(0.625*VSS_get[5]));
	lcd_goto_xy(6,2);
	fprintf_P(&lcd_out,PSTR("%-7.0f"), (double)MAKEWORD(RPM_get[5],RPM_get[6])/4.0);
	lcd_goto_xy(6,3);
	fprintf_P(&lcd_out,PSTR("%-7.0f"), (double)(0.3922*thrott_get[5]));
	lcd_goto_xy(6,4);
	fprintf_P(&lcd_out,PSTR("%-7.0f"), (double)(0.3922*load_get[5]));
    lcd_goto_xy(6,5);
	fprintf_P(&lcd_out,PSTR("%-7.0f"), (double)(temp_get[5]-40));
	lcd_goto_xy(6,6);
	fprintf_P(&lcd_out,PSTR("%-5.0f"), (double)(0.5*timing_get[5]-64));
	} //end if
 } //end while
 
 } //end get data1
 
 
 
 void get_data_set2(void) {
	
    lcd_clear();
	get_car_type(&vehicle_type);
	lcd_goto_xy(1,1);
    fprintf_P(&lcd_out,PSTR("STFT         %%"));
	lcd_goto_xy(1,2);
	fprintf_P(&lcd_out,PSTR("LTFT         %%"));
	lcd_goto_xy(1,3);
	fprintf_P(&lcd_out,PSTR("Teng         C"));
	lcd_goto_xy(1,4);
	fprintf_P(&lcd_out,PSTR(" IAT         C"));
	
 
 while(1) {
    if (switch_is_pressed(&switchtype)) {
		break;
	}
	else {
 	
	temp = iso_putb(&STFT_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	 {temp = iso_putb(&STFT_put[i],1, ISO_P4_MIN);}
    
	temp = iso_getb(&STFT_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<7; i++)
	{temp = iso_getb(&STFT_get[i],1, ISO_W2_MAX*2);}
	
	temp = iso_putb(&LTFT_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	 {temp = iso_putb(&LTFT_put[i],1, ISO_P4_MIN);}
    
	temp = iso_getb(&LTFT_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<7; i++)
	{temp = iso_getb(&LTFT_get[i],1, ISO_W2_MAX*2);}
	
	
	temp = iso_putb(&temp_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	 {temp = iso_putb(&temp_put[i],1, ISO_P4_MIN);}
    
	temp = iso_getb(&temp_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<7; i++)
	{temp = iso_getb(&temp_get[i],1, ISO_W2_MAX*2);}
	
	temp = iso_putb(&IAT_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	{temp = iso_putb(&IAT_put[i],1, ISO_P4_MIN);}

	temp = iso_getb(&IAT_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<7; i++)
	{temp = iso_getb(&IAT_get[i],1, ISO_W2_MAX*2);}
		
	lcd_goto_xy(6,1);
	fprintf_P(&lcd_out,PSTR("%-7.0f"), (double)(0.78125*STFT_get[5]-100));
	lcd_goto_xy(6,2);
	fprintf_P(&lcd_out,PSTR("%-7.0f"), (double)(0.78125*LTFT_get[5]-100));
    lcd_goto_xy(6,3);
	fprintf_P(&lcd_out,PSTR("%-7.0f"), (double)(temp_get[5]-40));
	lcd_goto_xy(6,4);
	fprintf_P(&lcd_out,PSTR("%-7.0f"), (double)(IAT_get[5]-40));
	} //end if switch
 } //end while
 
 } //end get data2

 
 
  void Who_am_I (void) {
    _delay_ms(500); 
    lcd_clear();
	get_car_type (&vehicle_type);
	clean_start();
	if (vehicle_type ==1) { //supports MAF ... van
	fprintf_P(&lcd_out,PSTR("SIENNA"));
	}
	else {
    fprintf_P(&lcd_out,PSTR("CAMRY"));
	}
	clean_exit_partial();
} 


 
 void get_MPG (void) {
 
    uint8_t get_MPG_startup =1;

    lcd_clear();
	get_car_type(&vehicle_type);
		    
    /**************
	read stored eeprom data
	******************/
eeprom_address = 460;
eeprom_read_block(&gas_price, (void*)eeprom_address, 4);
eeprom_address = 464;
eeprom_read_block(&vol_eff, (void*)eeprom_address, 4);
	
if (vehicle_type ==1) { //supports MAF ... van
 eeprom_address = 556;
 }
 else{
 eeprom_address = 508;
 }
 eeprom_read_block(&running_time_min_trip, (void*)eeprom_address, 4);
 running_time_trip = 60.0*running_time_min_trip;
 eeprom_address = eeprom_address+4;  
 eeprom_read_block(&speed_ave_trip, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_read_block(&running_dist_trip, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_read_block(&running_gallons_trip, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_read_block(&running_cost_trip, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_read_block(&MPG_ave_trip, (void*)eeprom_address, 4);
      
 
    /**************
	Get some data codes
	******************/
    lcd_clear();	
	GetTime(&t);
	
	while(1) {
	
	if ((switch_is_pressed(&switchtype))|(get_MPG_startup==1)) { 
	   if ((switchtype ==1) & (get_MPG_startup ==0)){
		break;
		}
		else {
        get_MPG_startup = 0;
		show_trip++;
        if(show_trip ==4) show_trip = 0;
        
		 if ((show_trip == 0)|(show_trip == 1)){
		lcd_clear();
		lcd_goto_xy(1,1);
		if (show_trip == 0){
			fprintf_P(&lcd_out,PSTR("2DAY"));}
			else {
			fprintf_P(&lcd_out,PSTR("TRIP"));}
		lcd_goto_xy(1,2);	
		fprintf_P(&lcd_out,PSTR("aSPD       mph"));
		lcd_goto_xy(1,3);
		fprintf_P(&lcd_out,PSTR("DIST        mi"));
		lcd_goto_xy(1,4);
		fprintf_P(&lcd_out,PSTR("FUEL       gal"));	
		lcd_goto_xy(1,5);
		fprintf_P(&lcd_out,PSTR("   $"));
		lcd_goto_xy(1,6);
		fprintf_P(&lcd_out,PSTR("aMPG"));
		}
		else if (show_trip ==2){
		lcd_clear();
		lcd_goto_xy(1,1);
		fprintf_P(&lcd_out,PSTR("iSPD       mph"));
		lcd_goto_xy(1,2);
		fprintf_P(&lcd_out,PSTR("iMPG"));
		} 
		else {
		lcd_clear();
		}//end if show_trip

        //set up views   
         
	    }//end if switchtype ==1
    }	//end case1 switch_is_pressed
	else {
	temp = iso_putb(&VSS_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	 {temp = iso_putb(&VSS_put[i],1, ISO_P4_MIN);}
    
	temp = iso_getb(&VSS_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<7; i++)
	{temp = iso_getb(&VSS_get[i],1, ISO_W2_MAX*2);}
	VSS_temp = (double) VSS_get[5];
	if (VSS_temp < (double) 2.0) {
	VSS_temp = (double) 0.0;}
	
    temp = iso_putb(&RPM_put[0],1, ISO_P3_MIN);
	for (i =1; i<6; i++)
	 {temp = iso_putb(&RPM_put[i],1, ISO_P4_MIN);}
     
    temp = iso_getb(&RPM_get[0],1, ISO_P2_MAX*2);
	for (i =1; i<8; i++)
	{temp = iso_getb(&RPM_get[i],1, ISO_W2_MAX*2);}
	
	rpm_temp = (double)MAKEWORD(RPM_get[5],RPM_get[6])/4.0;
	
	if (vehicle_type == 1) {
    	temp = iso_putb(&MAF_put[0],1, ISO_P3_MIN);
		for (i =1; i<6; i++)
		{temp = iso_putb(&MAF_put[i],1, ISO_P4_MIN);}
    
		temp = iso_getb(&MAF_get[0],1, ISO_P2_MAX*2);
		for (i =1; i<8; i++)
		{temp = iso_getb(&MAF_get[i],1, ISO_W2_MAX*2);}
	    MAF_temp =  (double)MAKEWORD(MAF_get[5],MAF_get[6])/100.0;
	}
    else {

		temp = iso_putb(&MAP_put[0],1, ISO_P3_MIN);
		for (i =1; i<6; i++)
		{temp = iso_putb(&MAP_put[i],1, ISO_P4_MIN);}
    
		temp = iso_getb(&MAP_get[0],1, ISO_P2_MAX*2);
		for (i =1; i<7; i++)
		{temp = iso_getb(&MAP_get[i],1, ISO_W2_MAX*2);}

		temp = iso_putb(&IAT_put[0],1, ISO_P3_MIN);
		for (i =1; i<6; i++)
		{temp = iso_putb(&IAT_put[i],1, ISO_P4_MIN);}
    
		temp = iso_getb(&IAT_get[0],1, ISO_P2_MAX*2);
		for (i =1; i<7; i++)
		{temp = iso_getb(&IAT_get[i],1, ISO_W2_MAX*2);}
	    IAT_temp =  (double)((IAT_get[5]-40)+273.0);
		MAF_temp =  (double)0.000628369*vol_eff*rpm_temp*MAP_get[5]/IAT_temp;
	} //endif vehicle_type
	
	dt_seconds = (double)GetElaspMs(&t)/1000.0;
	GetTime(&t);
	running_time =(double) (running_time +dt_seconds);
	running_time_min =(double) (running_time/60.0);
	running_cost = (double) (running_cost + 2.4307e-05*gas_price*dt_seconds*MAF_temp);
	MPG_temp = (double) (7.101*VSS_temp/MAF_temp);
	running_dist =(double) (running_dist+0.6214*VSS_temp*dt_seconds/3600.0);
    running_gallons =(double) (running_gallons+2.4307e-5*MAF_temp*dt_seconds);
	MPG_ave = (double) (running_dist/running_gallons); 
    speed_ave = (double) (3600.0*running_dist/running_time);

    running_time_trip =(double) (running_time_trip +dt_seconds);
	running_time_min_trip =(double) (running_time_trip/60.0);
	running_cost_trip = (double) (running_cost_trip + 2.4307e-05*gas_price*dt_seconds*MAF_temp);
	running_dist_trip =(double) (running_dist_trip+0.6214*VSS_temp*dt_seconds/3600.0);
    running_gallons_trip =(double) (running_gallons_trip+2.4307e-5*MAF_temp*dt_seconds);
	MPG_ave_trip = (double) (running_dist_trip/running_gallons_trip); 
    speed_ave_trip = (double) (3600.0*running_dist_trip/running_time_trip); 	
	
	if (show_trip == 0) {	
	parse_time(running_time_min, &elapsed_hours, &elapsed_mins, &elapsed_secs);
	lcd_goto_xy(6,1);
	fprintf_P(&lcd_out,PSTR("%02d:%02d:%02d"),elapsed_hours, elapsed_mins, elapsed_secs);
	lcd_goto_xy(6,2);
	fprintf_P(&lcd_out,PSTR("%-5.1f"), (double)speed_ave);
	lcd_goto_xy(6,3);
	fprintf_P(&lcd_out,PSTR("%-6.2f"), (double)running_dist);	
	lcd_goto_xy(6,4);
	fprintf_P(&lcd_out,PSTR("%-5.3f"), (double)running_gallons);
	lcd_goto_xy(6,5);
	fprintf_P(&lcd_out,PSTR("%-6.2f"), (double)running_cost);
	lcd_goto_xy(6,6);
	fprintf_P(&lcd_out,PSTR("%-8.1f"), MPG_ave);
     }
	else if (show_trip==1){
	parse_time(running_time_min_trip, &elapsed_hours, &elapsed_mins, &elapsed_secs);
	lcd_goto_xy(6,1);
	fprintf_P(&lcd_out,PSTR("%02d:%02d:%02d"),elapsed_hours, elapsed_mins, elapsed_secs); 
	lcd_goto_xy(6,2);
	fprintf_P(&lcd_out,PSTR("%-5.1f"), (double)speed_ave_trip);
	lcd_goto_xy(6,3);
	fprintf_P(&lcd_out,PSTR("%-6.2f"), (double)running_dist_trip);
	lcd_goto_xy(6,4);
	fprintf_P(&lcd_out,PSTR("%-5.3f"), (double)running_gallons_trip);
	lcd_goto_xy(6,5);
	fprintf_P(&lcd_out,PSTR("%-5.2f"), (double)running_cost_trip);
	lcd_goto_xy(6,6);
	fprintf_P(&lcd_out,PSTR("%-8.1f"), MPG_ave_trip);
	 
	 }//end if
	 else if (show_trip==2){
	lcd_goto_xy(7,1);
	fprintf_P(&lcd_out,PSTR("%-5.1f"), (double)0.6214*VSS_temp);
	lcd_goto_xy(7,2);
	fprintf_P(&lcd_out,PSTR("%-8.1f"), MPG_temp);    
     } else {
	 parsnum(MPG_ave, 5, 1);
     parscost(running_cost, 8, 5);
	 lcd_goto_xy(11,2);
     fprintf_P(&lcd_out,PSTR("mpg"));

	 }//end if show_trip
	 } //end if switch_is_pressed
	
   } //end while (1)
 
 } //end get MPG
 
 
 void clear_trip (void) {
 _delay_ms(500); 
lcd_clear();	

get_car_type (&vehicle_type);
if (vehicle_type ==1) { //supports MAF ... van
 eeprom_address = 556;
 }
 else {
 eeprom_address = 508;
 }
 
 eeprom_data = 0.0;
for (i=0; i<6; i++) {
 eeprom_update_block(&eeprom_data, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
}
    clean_exit_full();
}


 void clear_vector (void) {
 _delay_ms(500); 
lcd_clear();	


get_car_type (&vehicle_type);

if (vehicle_type ==1) { //supports MAF ... van
	 eeprom_address = 788;
	 }
	 else {
	 eeprom_address = 580;
	 }



for (i=1; i<52; i++) {
	 if (i==1) {
	 eeprom_data = 9999.0;}
	 else {
	 eeprom_data = 0.0;}
	 eeprom_update_block(&eeprom_data, (void*)eeprom_address, 4);
	 eeprom_address = eeprom_address+4;
}
 clean_exit_full();
}


void save_day(uint8_t show_complete){
_delay_ms(200);  
lcd_clear();
get_car_type (&vehicle_type); 
if (show_complete ==0) {
lcd_goto_xy(1,2);
fprintf_P(&lcd_out, PSTR("Saving 2DAY"));
_delay_ms(1500);
}

if (vehicle_type ==1) { //supports MAF ... van
 eeprom_address = 532;
 }
 else{
 eeprom_address = 484;
 }
 
 eeprom_update_block(&running_time_min, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_update_block(&speed_ave, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_update_block(&running_dist, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_update_block(&running_gallons, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_update_block(&running_cost, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_update_block(&MPG_ave, (void*)eeprom_address, 4);
 _delay_ms(200);
if (show_complete ==1) clean_exit_full();
}


void save_vector (uint8_t show_complete) {
uint8_t found = 0; 
_delay_ms(200);  
lcd_clear();
get_car_type (&vehicle_type); 

if (show_complete ==0) {
lcd_goto_xy(1,2);
fprintf_P(&lcd_out, PSTR("Saving DBASE"));
_delay_ms(800);
}
 
if (vehicle_type ==1) { //supports MAF ... van
 eeprom_address = 788;
 }
 else{
 eeprom_address = 580;
 }

//find_start
for (i=1; i<52; i++) {
  eeprom_read_block(&value, (void*)eeprom_address, 4);
  if(((value>9998.9)&(value<9999.1))&(found==0)) {
  found = i;
  eeprom_found = eeprom_address;
  }
  eeprom_address=eeprom_address+12;
 } 

if (i==49) {
 lcd_clear(); 
 lcd_goto_xy(1,1);
 fprintf_P(&lcd_out, PSTR("MEM: FULL"));
} 
else {
  eeprom_address = eeprom_found;
  value = 9999.0;
  eeprom_update_block(&speed_ave, (void*)eeprom_address, 4);
  eeprom_address = eeprom_address+4;
  eeprom_update_block(&running_dist, (void*)eeprom_address, 4);
  eeprom_address = eeprom_address+4;
  eeprom_update_block(&MPG_ave, (void*)eeprom_address, 4);
  eeprom_address = eeprom_address+4;
  eeprom_update_block(&value, (void*)eeprom_address, 4);
  lcd_clear();
  lcd_goto_xy(1,1);
  fprintf_P(&lcd_out, PSTR("Saved"));
  lcd_goto_xy(1,2);
  fprintf_P(&lcd_out, PSTR("MEM: %d/16"), (found+2)/3);
 }
   clean_exit_partial();
}

void save_trip (uint8_t show_complete) {
_delay_ms(200);  
lcd_clear();
get_car_type (&vehicle_type); 
if (show_complete ==0) {
lcd_goto_xy(1,2);
fprintf_P(&lcd_out, PSTR("Saving TRIP"));
_delay_ms(800);
}
if (vehicle_type ==1) { //supports MAF ... van
 eeprom_address = 556;
 }
 else{
 eeprom_address = 508;
 }
 
 eeprom_update_block(&running_time_min_trip, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_update_block(&speed_ave_trip, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_update_block(&running_dist_trip, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_update_block(&running_gallons_trip, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_update_block(&running_cost_trip, (void*)eeprom_address, 4);
 eeprom_address = eeprom_address+4;
 eeprom_update_block(&MPG_ave_trip, (void*)eeprom_address, 4);
 _delay_ms(200);
if (show_complete ==1) clean_exit_full();
}

void save_all(void){
save_day(0);
save_trip(0);
save_vector(0);
}


void show_data(uint8_t type){
double eeprom_show_data = 0.0;
int eeprom_show_hours = 0;
int eeprom_show_mins =  0;
int eeprom_show_secs = 0;
_delay_ms(500);  
lcd_clear();
get_car_type (&vehicle_type); 

if (type ==0) {//day

	if (vehicle_type ==1) { //supports MAF ... van
	 eeprom_address = 532;
	 }
	 else{
	 eeprom_address = 484;
	 }
}
else{//trip
	if (vehicle_type ==1) { //supports MAF ... van
	 eeprom_address = 556;
	 }
	 else{
	 eeprom_address = 508;
	 }
}

		lcd_clear();
		lcd_goto_xy(1,1);
		fprintf_P(&lcd_out,PSTR("TIME"));
		lcd_goto_xy(1,2);
		fprintf_P(&lcd_out,PSTR("aSPD       mph"));
		lcd_goto_xy(1,3);
		fprintf_P(&lcd_out,PSTR("DIST        mi"));
		lcd_goto_xy(1,4);
		fprintf_P(&lcd_out,PSTR("FUEL       gal"));
		lcd_goto_xy(1,5);	
		fprintf_P(&lcd_out,PSTR("   $"));
		lcd_goto_xy(1,6);
		fprintf_P(&lcd_out,PSTR("aMPG"));

	lcd_goto_xy(6,1);
	eeprom_read_block(&eeprom_show_data, (void*)eeprom_address, 4);
    parse_time(eeprom_show_data, &eeprom_show_hours, &eeprom_show_mins, &eeprom_show_secs);
	fprintf_P(&lcd_out,PSTR("%02d:%02d:%02d"),eeprom_show_hours, eeprom_show_mins, eeprom_show_secs);
	lcd_goto_xy(6,2);
	eeprom_read_block(&eeprom_show_data, (void*)eeprom_address+4, 4);
	fprintf_P(&lcd_out,PSTR("%-5.1f"), (double)eeprom_show_data);
	lcd_goto_xy(6,3);
	eeprom_read_block(&eeprom_show_data, (void*)eeprom_address+8, 4);
	fprintf_P(&lcd_out,PSTR("%-6.2f"), (double)eeprom_show_data);
	lcd_goto_xy(6,4);
	eeprom_read_block(&eeprom_show_data, (void*)eeprom_address+12, 4); 
	fprintf_P(&lcd_out,PSTR("%-5.3f"), (double)eeprom_show_data);
	lcd_goto_xy(6,5);
	eeprom_read_block(&eeprom_show_data, (void*)eeprom_address+16, 4); 
	fprintf_P(&lcd_out,PSTR("%-5.2f"), (double)eeprom_show_data);
	lcd_goto_xy(6,6);
	eeprom_read_block(&eeprom_show_data, (void*)eeprom_address+20, 4); 
	fprintf_P(&lcd_out,PSTR("%-8.1f"), (double)eeprom_show_data);
	_delay_ms(500);
	while (1) {
	if (switch_is_pressed(&switchtype)) {
	break;
	}
}   
}


void set_gas_price (void){
_delay_ms(500);
lcd_clear();
eeprom_address = 460;
eeprom_read_block(&gas_price, (void*)eeprom_address, 4);
lcd_goto_xy(1,1);
fprintf_P(&lcd_out, PSTR("Current:"));
lcd_goto_xy(1,2);
fprintf_P(&lcd_out, PSTR("$%5.2f"), gas_price);

uint8_t d1=3;
uint8_t d2=5;
uint8_t d3=0;
uint8_t cur_item=3;
uint8_t done = 0;
while(1) { 
if (done ==1) break;
lcd_goto_xy(1,3);
fprintf_P(&lcd_out, PSTR(" %d"), d1);
lcd_goto_xy(1,4);
fprintf_P(&lcd_out, PSTR(" %d"), d2);
lcd_goto_xy(1,5);
fprintf_P(&lcd_out, PSTR(" %d"), d3);
lcd_goto_xy(1,6);
fprintf_P(&lcd_out, PSTR(" OK"));

lcd_goto_xy(1,cur_item);
fprintf_P(&lcd_out, PSTR(">"));
_delay_ms(500);
while(1) {		
if (switch_is_pressed(&switchtype)) {
 if (switchtype == 1) { //we are scrolling
 _delay_ms(500);
 cur_item++;
 if (cur_item == 7) cur_item = 3;
 break;
}  //switchtype1
 if (switchtype == 2) { //we incrementing
   _delay_ms(500);
  if (cur_item == 3) {
  d1++;
  if (d1==10) d1=0;
  }
  if (cur_item == 4) {
  d2++;
  if (d2 ==10) d2=0;
  }
  
  if (cur_item == 5) {
  d3++;
  if (d3 ==10) d3=0;
  }
  if (cur_item == 6) done =1;
  break;
  }//switchtype2
}//switchpressed
}//INNER WHILE

} //OUTER while
_delay_ms(500);
gas_price = (double)(100.0*d1+10.0*d2+d3)/100;
eeprom_address = 460;
eeprom_update_block(&gas_price, (void*)eeprom_address, 4);
lcd_clear();
clean_start();
eeprom_read_block(&gas_price, (void*)eeprom_address, 4);
fprintf_P(&lcd_out, PSTR("$%5.2f"), gas_price);
clean_exit_partial();
}

void serial_settings (void){
lcd_clear();
_delay_ms(500);
lcd_goto_xy(1,1);
fprintf_P(&lcd_out, PSTR("19200 baud"));
lcd_goto_xy(1,2);
fprintf_P(&lcd_out, PSTR("8 data"));
lcd_goto_xy(1,3);
fprintf_P(&lcd_out, PSTR("No parity"));
lcd_goto_xy(1,4);
fprintf_P(&lcd_out, PSTR("1 stop"));
lcd_goto_xy(1,5);
fprintf_P(&lcd_out, PSTR("No flow"));
lcd_goto_xy(1,6);
fprintf_P(&lcd_out, PSTR("PRESS KEY"));
_delay_ms(500);
while (1) {
	if (switch_is_pressed(&switchtype)) {
	break;
	}
}   
}

uint16_t hexatoi(uint8_t *addr){ 
   uint8_t   *p; 
   uint8_t n; 
   uint16_t addr_val = 0; 

   if (addr[0] != '\0') { 
      p = addr; 
      addr_val = 0; 
      while (*p != '\0') { 
              addr_val <<= 4; 
              n = *p; 
              if ((n >= '0') && (n <= '9')) { 
                      n = n - '0'; 
              } 
              else if ((n >= 'a') && (n <= 'f')) { 
                      n = n - 'a' + 10; 
              } 
              else if ((n >= 'A') && (n <= 'F')) { 
                      n = n - 'A' + 10; 
              } 
              else { 
                      n = 0; 
              } 
              addr_val += n; 
              p++; 
      } 
   } 
   return addr_val; 
} 


void ping (void){

int j;
INT8 incoming;
//uint8_t wait;
lcd_clear();
    ping_put[0] = 0x68; 
	ping_put[1] = 0x6A;
	ping_put[2] = 0xF1;
	ping_sum = ping_put[0]+ping_put[1]+ping_put[2]; 
    lcd_clear();
    lcd_goto_xy(1,1);
    fprintf_P(&lcd_out,PSTR("READY"));
		 /*
		 wait = 1;	
         while (wait){
                if (switch_is_pressed(&switchtype)){
                        _delay_ms(500);
						lcd_clear();
						lcd_goto_xy(1,1);
						fprintf_P(&usart_out,PSTR("Enter num bytes\n\r"));
						wait = 0;
                }
        } 
        */
		incoming = usart_getchar();
		while (incoming !=115){    //char "s" from serial starts it
		incoming = usart_getchar();
		}
		lcd_clear();
		//lcd_goto_xy(1,1);
		//fprintf_P(&lcd_out,PSTR("CONNECTED")); 

/* ... working code ...*/
/*
while(1) {
		 fscanf_P(&usart,PSTR("%s"),&stream);
	     i = (int) hexatoi((uint8_t*)&stream);
		 lcd_clear();
		 lcd_goto_xy(1,2);
		 fprintf_P(&lcd_out,PSTR("%x"),i);
		 
		 for (j = 1;j<=i;j++) {
		 fscanf_P(&usart,PSTR("%s"),&stream);	 
		 lcd_goto_xy(1,j+2);
		 fprintf_P(&lcd_out,PSTR("%x"),(UBYTE) hexatoi((uint8_t*)&stream));
		 }			 
		 //send codes to car, then show return codes ... snippet below is just a test of this.
     _delay_ms(2000);
     testback = 0xAB;
     fprintf_P(&usart,PSTR("%06d\n\r"),testback); //print in decimal, convert to hex on other side
}
*/
         //first read ping length i
		 fscanf_P(&usart_out,PSTR("%s"),&stream);
	     //i = (int) hexatoi((uint8_t*)&stream);
		//lcd_goto_xy(1,1);
		//fprintf_P(&lcd_out,PSTR("%x"),i);
	     ping_length = 4 + (int) hexatoi((uint8_t*)&stream);
		// fprintf_P(&lcd_out,PSTR("MSG %d bytes"),ping_length-4);
		//fill up ping array & output to LCD for confirmation
				
		for (j = 1;j<=(ping_length-4);j++) {
		 fscanf_P(&usart_out,PSTR("%s"),&stream);	 
		 lcd_goto_xy(1,j);
		 ping_put[2+j] = (UBYTE) hexatoi((uint8_t*)&stream);
		 fprintf_P(&lcd_out,PSTR("BYTE %d 0x%x"),j,(UBYTE)ping_put[2+j]);
		 ping_sum += ping_put[2+j];
		 }	
		 //last byte is parity check byte
         ping_put[ping_length-1] = (UBYTE) ping_sum; 
		 lcd_goto_xy(1,5);
         fprintf_P(&lcd_out,PSTR("CHKSM 0x%x"),ping_put[ping_length-1]);
		 lcd_goto_xy(1,6);
         fprintf_P(&lcd_out,PSTR("PRESS KEY"));
		 while (1) {
			if (switch_is_pressed(&switchtype)) {
			break;
			}
		}   
		 
	
    ISO_init_comm(0); 
    temp = iso_putb(&ping_put[0],1, ISO_P3_MIN);
	for (i =1; i<ping_length; i++)
	{temp = iso_putb(&ping_put[i],1, ISO_P4_MIN);}
	

	ping_length = 1;
	temp = iso_getb(&ping_get[0],1, ISO_P2_MAX*2);
	ping_sum = ping_get[0];
	while(1){
	temp = iso_getb(&ping_get[ping_length],1, ISO_W2_MAX*2);
	if ((ping_length>3) && (ping_get[ping_length] == (UBYTE) ping_sum)) {
	 break; //checksum ... last bit
	} //end if >3
	ping_sum += ping_get[ping_length];
	ping_length++;
	}//end while
    lcd_goto_xy(1,2);
	fprintf_P(&lcd_out,PSTR("OK"));
		
	//output ping_get
    for (i=0; i<(ping_length+1); i++)	{
	//output & delay
	fprintf_P(&usart_out,PSTR("%06d\r\n"),ping_get[i]);
	_delay_ms(200);
	}
	fprintf_P(&usart_out,PSTR("%06d\r\n"),999);
    clean_exit_full();
}




void EEPROM2XL (void){ //working w/ delays as set &BOOK3.xls
UINT8 incoming;
unsigned int addr;
float temp_read = 0.0;
lcd_clear();

/* just used to update arbitrary eeprom address
eeprom_address = 980;
eeprom_data = 9999.0;
eeprom_update_block(&eeprom_data, (void*)eeprom_address, 4);
*/	 


lcd_goto_xy(1,1);
fprintf_P(&lcd_out,PSTR("Wait4XL")); 

incoming = usart_getchar();
while (incoming !=115){    //char "s" from serial starts it
incoming = usart_getchar();
}

lcd_goto_xy(1,2);
fprintf_P(&lcd_out,PSTR("OK")); 

while (usart_char_is_waiting()){
incoming = usart_getchar();
}

addr=460;
//for (i=1; i<200; i++) _delay_ms(25); 	  

while(1) {
		if ((switch_is_pressed(&switchtype)) | (addr > 988)) {
		fprintf_P(&usart_out,PSTR("%06d\r\n"),999); //print in decimal, convert to hex on other side
		break;
		}
      eeprom_read_block(&temp_read, (void*)addr, 4);
	  lcd_goto_xy(1,2);
	  fprintf_P(&lcd_out,PSTR("%d"), addr); 
      fprintf_P(&usart_out,PSTR("%08.2f\r\n"), temp_read);	
	  //fprintf_P(&usart_out,PSTR("%6.2f"), temp_read);
	  addr = addr+4;
	  //for (i=1; i<20; i++) _delay_ms(6);
	  _delay_ms(60);
		}  
    clean_exit_full();
}



void STREAM2XL (void){

UINT8 incoming, i;
uint8_t cur_item=1;
double serial_data = 0.0;
uint8_t done = 0;
uint8_t update = 0;
int started = 0;
int stopped = 0;
int need_to_exit = 0;

while(1) { //while(1) #1

    need_to_exit = 0;
	started = 0;
	if (done ==1) break;
	lcd_clear();
	lcd_goto_xy(1,1);
	fprintf_P(&lcd_out, PSTR(" SPEED"));
	lcd_goto_xy(1,2);
	fprintf_P(&lcd_out, PSTR(" RPM"));
	lcd_goto_xy(1,3);
	fprintf_P(&lcd_out, PSTR(" THROTT"));
	lcd_goto_xy(1,4);
	fprintf_P(&lcd_out, PSTR(" iMPG"));
	lcd_goto_xy(1,5);
	fprintf_P(&lcd_out, PSTR(" DIST"));
	lcd_goto_xy(1,6);
	fprintf_P(&lcd_out, PSTR(" DONE"));
	lcd_goto_xy(1,cur_item);
	fprintf_P(&lcd_out, PSTR(">"));
	_delay_ms(500);
	update = 0;
		while(1) {	//while(1) #2
			if (switch_is_pressed(&switchtype)) {
				 if (switchtype == 1) { //we are scrolling
					cur_item++;
					if (cur_item == 7) cur_item = 1;
				}  //switchtype1
				if (switchtype == 2) { //we have selected
					update = 1;
					if(cur_item == 6) done = 1;
				}//switchtype2
				break;
			}//switchpressed
		}//while(1) #2
 
		if ((update ==1) & (done ==0)){
			_delay_ms(500);
			lcd_clear();
			lcd_goto_xy(1,1);
			fprintf_P(&lcd_out,PSTR("Wait4XL")); 
			

				incoming = usart_getchar();
				while (incoming !=115){    //char "s" from serial starts it
				incoming = usart_getchar();
				}

				lcd_goto_xy(1,2);
				fprintf_P(&lcd_out,PSTR("OK")); 

				while (usart_char_is_waiting()){
				incoming = usart_getchar();
				}

				//for (i = 1; i<=100; i++){
				//fprintf_P(&usart_out,PSTR("%6d\n\r"), i);
				//_delay_ms(150);
				//}//next i
                get_car_type(&vehicle_type);
				
				while(1) { //while(1) #3
				        stopped = 0;
						if (need_to_exit == 1) {
							lcd_clear();
							lcd_goto_xy(1,1);
							fprintf_P(&lcd_out,PSTR("DONE"));
						break;
						}
				        /*
						if (switch_is_pressed(&switchtype)) {
						done = 1;
						break;
						}	
						*/
                     if (started ==1) {			
						incoming = usart_getchar();
						while (incoming !=115){
							if ((incoming == 117)|(switch_is_pressed(&switchtype)))
								{
									need_to_exit = 1;
									break;
								} 
						incoming = usart_getchar();
						}
						if (need_to_exit == 1) {
							lcd_clear();
							lcd_goto_xy(1,1);
							fprintf_P(&lcd_out,PSTR("DONE"));
							break;
						}
						lcd_clear();
						lcd_goto_xy(1,1);
						fprintf_P(&lcd_out,PSTR("RUN"));
						stopped = 0;
						while (usart_char_is_waiting()){
						incoming = usart_getchar();
						}
					 } //if (started ==1)

				while(1) { //while(1) #4
							while (usart_char_is_waiting()) {
							incoming=usart_getchar();
							  while ((incoming ==116)|(incoming ==117)|(switch_is_pressed(&switchtype))){
							        lcd_clear();
									lcd_goto_xy(1,1);
									fprintf_P(&lcd_out,PSTR("PAUSE"));
									started = 1; //we are already going
									stopped = 1; // we are currently stopped
									if (incoming ==116){	//pause ... 116 ... "t"
										break;}
									else	{      //otherwise want to exit ... "u" or switch
										need_to_exit = 1; //want to exit the loop
										break;
									}
								} //end while (incoming)

							}//while (Usart_char_is_waiting())
							if (stopped ==1) break; //break out of while(1) 4
							
							// do all the reading & output here  
							
		            temp = iso_putb(&thrott_put[0],1, ISO_P3_MIN);
					for (i =1; i<6; i++)
					 {temp = iso_putb(&thrott_put[i],1, ISO_P4_MIN);}
					
					temp = iso_getb(&thrott_get[0],1, ISO_P2_MAX*2);
					for (i =1; i<7; i++)
					{temp = iso_getb(&thrott_get[i],1, ISO_W2_MAX*2);}

					temp = iso_putb(&load_put[0],1, ISO_P3_MIN);
					for (i =1; i<6; i++)
					 {temp = iso_putb(&load_put[i],1, ISO_P4_MIN);}
					
					temp = iso_getb(&load_get[0],1, ISO_P2_MAX*2);
					for (i =1; i<7; i++)
					{temp = iso_getb(&load_get[i],1, ISO_W2_MAX*2);}


						temp = iso_putb(&VSS_put[0],1, ISO_P3_MIN);
					for (i =1; i<6; i++)
					 {temp = iso_putb(&VSS_put[i],1, ISO_P4_MIN);}
					
					temp = iso_getb(&VSS_get[0],1, ISO_P2_MAX*2);
					for (i =1; i<7; i++)
					{temp = iso_getb(&VSS_get[i],1, ISO_W2_MAX*2);}
					VSS_temp = (double) VSS_get[5];
					if (VSS_temp < (double) 2.0) {
					VSS_temp = (double) 0.0;}
					
					temp = iso_putb(&RPM_put[0],1, ISO_P3_MIN);
					for (i =1; i<6; i++)
					 {temp = iso_putb(&RPM_put[i],1, ISO_P4_MIN);}
					 
					temp = iso_getb(&RPM_get[0],1, ISO_P2_MAX*2);
					for (i =1; i<8; i++)
					{temp = iso_getb(&RPM_get[i],1, ISO_W2_MAX*2);}
					
					rpm_temp = (double)MAKEWORD(RPM_get[5],RPM_get[6])/4.0;
					
					if (vehicle_type == 1) {
						temp = iso_putb(&MAF_put[0],1, ISO_P3_MIN);
						for (i =1; i<6; i++)
						{temp = iso_putb(&MAF_put[i],1, ISO_P4_MIN);}
					
						temp = iso_getb(&MAF_get[0],1, ISO_P2_MAX*2);
						for (i =1; i<8; i++)
						{temp = iso_getb(&MAF_get[i],1, ISO_W2_MAX*2);}
						MAF_temp =  (double)MAKEWORD(MAF_get[5],MAF_get[6])/100.0;
					}
					else {

						temp = iso_putb(&MAP_put[0],1, ISO_P3_MIN);
						for (i =1; i<6; i++)
						{temp = iso_putb(&MAP_put[i],1, ISO_P4_MIN);}
					
						temp = iso_getb(&MAP_get[0],1, ISO_P2_MAX*2);
						for (i =1; i<7; i++)
						{temp = iso_getb(&MAP_get[i],1, ISO_W2_MAX*2);}

						temp = iso_putb(&IAT_put[0],1, ISO_P3_MIN);
						for (i =1; i<6; i++)
						{temp = iso_putb(&IAT_put[i],1, ISO_P4_MIN);}
					
						temp = iso_getb(&IAT_get[0],1, ISO_P2_MAX*2);
						for (i =1; i<7; i++)
						{temp = iso_getb(&IAT_get[i],1, ISO_W2_MAX*2);}
						IAT_temp =  (double)((IAT_get[5]-40)+273.0);
						MAF_temp =  (double)0.000628369*vol_eff*rpm_temp*MAP_get[5]/IAT_temp;
					} //endif vehicle_type
					
					dt_seconds = (double)GetElaspMs(&t)/1000.0;
					GetTime(&t);
					running_time =(double) (running_time +dt_seconds);
					running_time_min =(double) (running_time/60.0);
					running_cost = (double) (running_cost + 2.4307e-05*gas_price*dt_seconds*MAF_temp);
					MPG_temp = (double) (7.101*VSS_temp/MAF_temp);
					running_dist =(double) (running_dist+0.6214*VSS_temp*dt_seconds/3600.0);
					running_gallons =(double) (running_gallons+2.4307e-5*MAF_temp*dt_seconds);
					
					switch (cur_item) {
											case 1: 
												serial_data = 0.6214*VSS_temp;
												break;
											case 2: 
												serial_data = rpm_temp;
												break;
											case 3: 
												serial_data = 0.3922*thrott_get[5];
												break;
											case 4: 
												serial_data = MPG_temp;
												break;
											case 5: 
												serial_data = running_dist;
												break;
											case 6: 
												serial_data = 0.3922*load_get[5];
												break;                        
												}
						fprintf_P(&usart_out,PSTR("%08.2f\r\n"), serial_data);
						lcd_goto_xy(1,2);
						fprintf_P(&lcd_out,PSTR("%-8.2f"), serial_data);

						_delay_ms(70);				
							} //while(1) #4
					} //while(1) #3
												
		  }//if done = 0 & update = 1

}//while(1) #1
    clean_exit_full();    
}



void set_volumetric_eff (void){
_delay_ms(500);
lcd_clear();
eeprom_address = 464;
eeprom_read_block(&vol_eff, (void*)eeprom_address, 4);
lcd_goto_xy(1,1);
fprintf_P(&lcd_out, PSTR("Current"));
lcd_goto_xy(1,2);
fprintf_P(&lcd_out, PSTR("%5.1f%%"), vol_eff);

uint8_t d1=8;
uint8_t d2=0;
uint8_t cur_item=3;
uint8_t done = 0;
while(1) { 
if (done ==1) break;
lcd_goto_xy(1,3);
fprintf_P(&lcd_out, PSTR(" %d"), d1);
lcd_goto_xy(1,4);
fprintf_P(&lcd_out, PSTR(" %d"), d2);
lcd_goto_xy(1,5);
fprintf_P(&lcd_out, PSTR(" OK"));

lcd_goto_xy(1,cur_item);
fprintf_P(&lcd_out, PSTR(">"));
_delay_ms(500);
while(1) {		
if (switch_is_pressed(&switchtype)) {
 if (switchtype == 1) { //we are scrolling
 _delay_ms(500);
 cur_item++;
 if (cur_item == 6) cur_item = 3;
 break;
}  //switchtype1
 if (switchtype == 2) { //we incrementing
  _delay_ms(500);
  if (cur_item == 3) {
  d1++;
  if (d1==10) d1=0;
  }
  if (cur_item == 4) {
  d2++;
  if (d2 ==10) d2=0;
  }
  
  if (cur_item == 5) done =1;
  break;
  }//switchtype2
}//switchpressed
}//INNER WHILE

} //OUTER while
_delay_ms(500);
vol_eff = (double)(10.0*d1+d2);
eeprom_address = 464;
eeprom_update_block(&vol_eff, (void*)eeprom_address, 4);
lcd_clear();
clean_start();
eeprom_read_block(&vol_eff, (void*)eeprom_address, 4);
 _delay_ms(5);
fprintf_P(&lcd_out, PSTR("%5.1f%%"), vol_eff);
clean_exit_partial();
}

 
/*******************************************************************************
                                Main
********************************************************************************/
int main(void)
{	
    //initialize variables
    MPG_ave = (double) 0.0;
	running_dist = (double) 0.0;
	running_time = (double) 0.0;
	running_cost = (double) 0.0;
    running_gallons = (double) 0.0;
	speed_ave = (double) 0.0;
	running_time_min = (double) 0.0;
	MPG_ave_trip= (double) 0.0;
	running_dist_trip = (double) 0.0;
	running_time_trip = (double) 0.0;
	running_cost_trip = (double) 0.0;
    running_gallons_trip = (double) 0.0;
	speed_ave_trip = (double) 0.0;
	running_time_min_trip = (double) 0.0;
    //
	
	TimeInit();	//start timer routine
	sei(); 	// enable interrupts
	connect_timer(1);
	switchio_init();	// initialize switches
	iso_init();
	usart_init ( MYUBRR );	// fire up the usart
//	fprintf_P(&usart_out,PSTR("alive"));
	     
	// Setup LCD
	lcd_init();
	lcd_contrast(0x40);
 
    initMenu(); // initialize menu by adding menu data to menu globals
	set_data();
     
    while(1) {
    showMenu();
	_delay_ms(500);
 
        while(1) { 
		
              if (switch_is_pressed(&switchtype)) {
                if (switchtype == 1) { //we are scrolling
				curItem++; // add one to curr item
                cursorCount++;
                   if (menuitem[curMenu][curItem][0] == '\0') { 
				   curItem = 0; 
				   pageScroll = 0; 
				   cursorCount = 0;
				   }
                   if (cursorCount >= pageSize) {
                    // we have scrolled past this page, go to next
                    // remember, we check if we have scrolled off the MENU under clicks.  This is off the PAGE.
                    pageScroll++;  // next "page"
                    cursorCount=0; // reset cursor location
                    }
                    menuCount = pageScroll*pageSize;
					break;
                } 
				
				if (switchtype == 2) { //we are selecting 
					// handle user input
					if (menuactn[curMenu][curItem]) {
						// has an action
						switch (menuactn[curMenu][curItem]) {
							case 1: //run MPG - DONE
								get_MPG();
								break; 
							case 2: //screen 1 of data - DONE
								get_data_set1();
								break;
							case 3: //screen 2 of data - DONE
								get_data_set2();
								break;
							case 4: //PIDS supported - DONE
								get_supported_PIDs();
								break;
							case 5: //ISO DEBUG - DONE
								ISO_init_comm(1);
								break;
							case 6: //WHO AM I - DONE
								Who_am_I();
								break;	
							case 7: //NUM CODES - DONE
								get_num_error_codes();
								break;
							case 8: //GET CODES - DONE
								get_error_codes();
								break;
							case 9: //CLEAR CODES - DONE
								clear_codes();
								break;
							case 10: //GAS PRICE - DONE
								set_gas_price();
								break;
							case 11: //VOLUMETRIC RATIO - DONE
								set_volumetric_eff();
								break;
							case 12: //SAVE ALL - DONE
								save_all();
								break;
							case 13: //SAVE DAY - DONE
								save_day(1);
								break;            
							case 14: //SAVE TRIP - DONE
								save_trip(1);
								break;
							case 15: //SAVE VECTOR - DONE
								save_vector(1);
								break;       
							case 16: //SHOW DAY - DONE
								show_data(0);
								break;            
							case 17: //SHOW TRIP - DONE
								show_data(1);
								break;
							case 18: //CLEAR TRIP - DONE
								clear_trip();
								break;            
							case 19: //CLEAR VECTOR - DONE
								clear_vector();
								break;	
							case 20: //EEPROM 2 XL - DONE - need to test
								EEPROM2XL();
								break;
							case 21: //STREAM 2 XL - DONE - need to test
								STREAM2XL();
								break;	
                            case 22: //PING - DONE
								ping();
								break;		
                            case 23: //SETTINGS - DONE
								serial_settings();
								break;		 		
							case 999: //RETURN TO MAIN
								curMenu = 0; // return to main menu
								curItem = 0; // reset menu item to which cursor point
								pageScroll = 0; // reset menu page scroll
								cursorCount = 0; // reset menu location of page
								//menuCount = pageScroll*pageSize; // reprint from first line of this page
							    break;
							}//switch	
						menuCount = 0;
						break;
					    } 
						else //GO TO A SUB-MENU
						{
						curMenu = menulink[curMenu][curItem];  // set to menu selected by cursor
						curItem = 0; // reset menu item to which cursor point
						pageScroll = 0; // reset menu page scroll
						cursorCount = 0; // reset menu location of page
						menuCount = pageScroll*pageSize; // reprint from first line of this page
						break;
						}//end if action
						updateFlag = 1; // we have updated the menu.  Flag is used to delay user input
					} // end we are selecting
				} // end switch is pressed
			} // end while (1)	
	    } //end while (1)
  
  return 0;

} //end main




 
	
	