
 /*******************************************************************
 *
 *  DESCRIPTION:
 *
 *
 *  FILENAME: switchio.h
 *	
 *******************************************************************/
#ifndef __SWITCHIO_H
#define __SWITCHIO_H

#define SWITCH_PORT PORTB       /* PORTx - register for button output */
#define SWITCH_PIN PINB         /* PINx - register for button input */
#define SWITCH1_BIT PB3          /* bit for button input/output */
#define SWITCH2_BIT PB2          /* bit for button input/output */

void switchio_init(void);
UINT8 switch_is_pressed(uint8_t *type);

#endif 

