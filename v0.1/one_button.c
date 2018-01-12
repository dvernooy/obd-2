// menusys.c
// for NerdKits with ATmega328p
// cz
 
#define F_CPU 14745600
 
#include <stdio.h>
#include <string.h>
 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
 
#include "../libnerdkits/io_328p.h"
#include "../libnerdkits/delay.h"
#include "../libnerdkits/lcd.h"
 
// PIN DEFINITIONS:
// PC1 -- WHITE LED
// PC2 -- RED LED
// PC3 -- GREEN LED
// PC4 -- PUSH BUTTON
 
// Global menu data
#define maxMenus 7
#define maxItems 6
#define pageSize 3 // available lines minus title line
 
int curMenu = 0; // menu currently shown
int curItem = 0; // item currently marked (line 0 for menutitle, so starts on one). NB! Item marked may not correspond to line marked (see cursorCount)
int updateFlag = 0; // used to register when there has been an update to the menu (moved around) and wait a little before allowing user input
int cursorCount = 0;  // keeps track of white line of menu the cursor is on. Since there may be multiple pages, this may not be the same as the item marked in the menu
int menuCount = 0;  // keeps track of what line of menu to print
int pageScroll = 0; // for far have we scrolled down - pageSize items on each page.
int menuprintloop = 0; // keep track of numbers of printed items
 
char menutitle[maxMenus][20];
char menuitem[maxMenus][maxItems][20];
int menulink[maxMenus][maxItems]; // a link to a submenu OR
int menuactn[maxMenus][maxItems]; // menu actions - turn on LED or whatever
 
// LED as output
FILE lcd_stream = FDEV_SETUP_STREAM(lcd_putchar, 0, _FDEV_SETUP_WRITE);
 
void initMenu() {
    strcpy(menutitle[0], "MAIN MENU");
    strcpy(menuitem[0][0], "Green LED");
    menulink[0][0] = 1; // link to menutitle[1];
    menuactn[0][0] = 0; // No action - just a sub menu
    strcpy(menuitem[0][1], "Red LED");
    menulink[0][1] = 2;
    menuactn[0][1] = 0;
    strcpy(menuitem[0][2], "White LED");
    menulink[0][2] = 3;
    menuactn[0][2] = 0;
    strcpy(menuitem[0][3], "All LED");
    menulink[0][3] = 4;
    menuactn[0][3] = 0;
    strcpy(menuitem[0][4], "Menu level 2");
    menulink[0][4] = 5;
    menuactn[0][4] = 0;
 
    strcpy(menuitem[0][5], "\0"); // Need to initialize the end str of array
 
    strcpy(menutitle[1], "Green LED");
    strcpy(menuitem[1][0], "Turn on");
    menulink[1][0] = 0; // no sub menu, just action
    menuactn[1][0] = 1;
    strcpy(menuitem[1][1], "Turn off");
    menulink[1][1] = 0;
    menuactn[1][1] = 2;
    strcpy(menuitem[1][2], "Return to main");
    menulink[1][2] = 999;
    menuactn[1][2] = 0;
    strcpy(menuitem[1][3], "\0"); // Need to initialize the end str of array
 
    strcpy(menutitle[2], "Red LED");
    strcpy(menuitem[2][0], "Turn on");
    menulink[2][0] = 0; // no sub menu, just action
    menuactn[2][0] = 3;
    strcpy(menuitem[2][1], "Turn off");
    menulink[2][1] = 0;
    menuactn[2][1] = 4;
    strcpy(menuitem[2][2], "Return to main");
    menulink[2][2] = 999; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[2][2] = 0;
    strcpy(menuitem[2][3], "\0"); // Need to initialize the end str of array
 
    strcpy(menutitle[3], "White LED");
    strcpy(menuitem[3][0], "Turn on");
    menulink[3][0] = 0; // no sub menu, just action
    menuactn[3][0] = 5;
    strcpy(menuitem[3][1], "Turn off");
    menulink[3][1] = 0;
    menuactn[3][1] = 6;
    strcpy(menuitem[3][2], "Return to main");
    menulink[3][2] = 999; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[3][2] = 0;
    strcpy(menuitem[3][3], "\0"); // Need to initialize the end str of array
 
    strcpy(menutitle[4], "All LEDs");
    strcpy(menuitem[4][0], "Turn on");
    menulink[4][0] = 0; // no sub menu, just action
    menuactn[4][0] = 7;
    strcpy(menuitem[4][1], "Turn off");
    menulink[4][1] = 0;
    menuactn[4][1] = 8;
    strcpy(menuitem[4][2], "Return to main");
    menulink[4][2] = 999; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[4][2] = 0;
    strcpy(menuitem[4][3], "\0"); // Need to initialize the end str of array
 
    strcpy(menutitle[5], "Menu level 2");
    strcpy(menuitem[5][0], "Menu level 3");
    menulink[5][0] = 6; // go to menu 6
    menuactn[5][0] = 0;
    strcpy(menuitem[5][1], "Return to main");
    menulink[5][1] = 999; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[5][1] = 0;
    strcpy(menuitem[5][2], "\0"); // Need to initialize the end str of array
 
    strcpy(menutitle[6], "Menu level 3");
    strcpy(menuitem[6][0], "Return to main");
    menulink[6][0] = 999; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[6][0] = 0;
    strcpy(menuitem[6][1], "\0"); // Need to initialize the end str of array
 
    // END of menu data
}
 
void showMenu() {
    lcd_clear_and_home();
    fprintf_P(&lcd_stream, PSTR("%s"), menutitle[curMenu]);
    //print menu content
    while ((menuprintloop < (pageSize)) && (menuitem[curMenu][menuCount][0] != '\0')) {
        lcd_goto_position(menuprintloop+1, 0); // +1 to leave first line for menu title
        if ((cursorCount + (pageSize*pageScroll)) == menuCount) {
            // item currently indicated by cursor
            fprintf_P(&lcd_stream, PSTR("->%s"), menuitem[curMenu][menuCount]);
        } else {
            fprintf_P(&lcd_stream, PSTR("  %s"), menuitem[curMenu][menuCount]);
        }
        menuprintloop++;
        menuCount++;
    }
    menuprintloop = 0;
}
 
int clicklen() {
    int timeCount = 0;
    while (timeCount < 500) { // half a second click = long
        if (!(PINC & (1<<PC4))) {
            // click stopped
            return 0;
        }
        delay_ms(1);
        timeCount++;
    }
    return 1; // click lasted for more than 500ms
}
 
int clicks() {
    delay_ms(65);  // just to give button time to settle
    //Started by btn click, no check for how long
    if (clicklen() == 1) {
        // long click
        // return selection
        return curItem;
    } else {
        // short click
        curItem++; // add one to curr item
        cursorCount++;
        if (menuitem[curMenu][curItem][0] == '\0') { curItem = 0; pageScroll = 0; cursorCount = 0; }
    }
    return 999; // we can't use 0 since that is actually a menu item. So 999 indicates short click;
 
}
 
//-- MAIN--
int main() {
    int selection; // return action from click function
    lcd_init();
 
    DDRC &= ~(1<<PC4); // Set PC4 for input
    DDRC |= (1<<PC3);   // Set PC3 for output
    DDRC |= (1<<PC2);   // Set PC2 for output
    DDRC |= (1<<PC1);   // Set PC1 for output
 
    initMenu(); // initialize menu by adding menu data to menu globals
 
    // print menu
    while(1) {
        showMenu();
 
        if (updateFlag>0) {
            // here we want to wait a while before we proceed back to the menu.  This is basically in order for the user to see the result of
            // his actions and then give him time enough to release the push button.  If we don't do this it will just race through the
            // next menu until he lets go.  We only do this is there has been an update to the menu
            delay_ms(300);
            updateFlag = 0;
        }
 
        while(1) { 
            if (PINC & (1<<PC4)) {
                selection = clicks();
                if (selection == 999) {
                    // no selection, just flipping though menu
                    //menuCount = 0; // if we are still on same page, we resent what menu lines we will print                  
                    if (cursorCount >= pageSize) {
                        // we have scrolled past this page, go to next
                        // remember, we check if we have scrolled off the MENU under clicks.  This is off the PAGE.
                        pageScroll++;  // next "page"
                        cursorCount=0; // reset cursor location
                    }
                    menuCount = pageScroll*pageSize;
                    showMenu(); // updates menu
                } else {
                    // a selection was made
                    break; // proceed
                }
            } 
        }
 
        // handle user input
        if (menuactn[curMenu][selection]) {
            // has an action
            switch (menuactn[curMenu][selection]) {
                case 1:
                    // action 1
                    PORTC |= (1<<PC3);  // Turn on green LED
                    break;
                case 2:
                    // action 2
                    PORTC &= ~(1<<PC3); // Turn off green LED
                    break;
                case 3:
                    PORTC |= (1<<PC2);  // Turn on red LED
                    break;             
                case 4:
                    PORTC &= ~(1<<PC2); // Turn off red LED
                    break; 
                case 5:
                    PORTC |= (1<<PC1);  // Turn on white LED
                    break;
                case 6:
                    PORTC &= ~(1<<PC1); // Turn off white LED
                    break;
                case 7:
                    PORTC |= (1<<PC1);
                    PORTC |= (1<<PC2);
                    PORTC |= (1<<PC3);  // turn on all LEDs
                    break;
                case 8:
                    PORTC &= ~(1<<PC1);
                    PORTC &= ~(1<<PC2);
                    PORTC &= ~(1<<PC3); // turn off all LEDs
                    break;
            }
            // wait a sec to give user time to release btn
            delay_ms(150);
            menuCount = 0;
        } else {
            // no action so must have sub menu
            switch (menulink[curMenu][selection]) {
                case 999:
                    curMenu = 0; // return to main menu
                    curItem = 0; // reset menu item to which cursor point
                    pageScroll = 0; // reset menu page scroll
                    cursorCount = 0; // reset menu location of page
                    menuCount = pageScroll*pageSize; // reprint from first line of this page
                    break;
                default:
                    curMenu = menulink[curMenu][selection];  // set to menu selected by cursor
                    curItem = 0; // reset menu item to which cursor point
                    pageScroll = 0; // reset menu page scroll
                    cursorCount = 0; // reset menu location of page
                    menuCount = pageScroll*pageSize; // reprint from first line of this page
                    break;
            }
            updateFlag = 1; // we have updated the menu.  Flag is used to delay user input
        }
        //cursorCount = 0;
        //menuCount = 0;
    }
    return 0;
}