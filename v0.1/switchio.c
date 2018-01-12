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
 *
 *  DESCRIPTION: IO routines for switches
 *
 *
 *  FILENAME: switchio.c
 *	
 *******************************************************************/
#
#define DEBOUNCE_TIME 75        /* time to wait while "de-bouncing" button */

#include <avr/io.h>
#include "datatypes.h"
#include <util/delay.h>
#include "switchio.h"

void switchio_init() 
{
        /* turn on internal pull-up resistors for the switch */
        SWITCH_PORT |= _BV(SWITCH1_BIT);
		SWITCH_PORT |= _BV(SWITCH2_BIT);
}

UINT8 switch_is_pressed(uint8_t *type)
{
        /* the button is pressed when BUTTON_BIT is clear */
        if (bit_is_clear(SWITCH_PIN, SWITCH1_BIT))
        {
                _delay_ms(DEBOUNCE_TIME);
                if (bit_is_clear(SWITCH_PIN, SWITCH1_BIT)) {
				*type = 1; 
				return 1;
				}
        }
         
		/* the button is pressed when BUTTON_BIT is clear */
        if (bit_is_clear(SWITCH_PIN, SWITCH2_BIT))
        {
                _delay_ms(DEBOUNCE_TIME);
                if (bit_is_clear(SWITCH_PIN, SWITCH2_BIT)) {
				*type = 2;
				return 1;
				}
        } 
        return 0;
}

