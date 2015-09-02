/* rotaryencoder.c :
 * Just another library for rotary encoders on Raspberry Pi.
 * Needs its rotaryencoder.h file companion and "wiringPi" LPGLv3 lib.
 */
 
/*=======================================================================\
|      - Copyright (c) - August 2015 - F6HQZ - Francois BERGERET -       |
|                                                                        |
| rotaryencoder.c and rotaryencoder.h files can run only with the        |
| necessary and excellent wiringPi tools suite for Raspberry Pi from the |
| "Gordons Projects" web sites from Gordon Henderson :                   |
| https://projects.drogon.net/raspberry-pi/wiringpi/                     |
| http://wiringpi.com/                                                   |
|                                                                        |
| My library permits an easy use of few rotary encoders with push switch |
| in ther axe and use them as "objects" stored in structures. Like this, |
| they are easy to read or modify values and specs from anywhere in your |
| own software which must use them.                                      |
|                                                                        |
| Thanks to friends who have supported me for this project and all guys  |
| who have shared their own codes with the community.                    |
|                                                                        |
| Always exploring new technologies, curious about any new idea or       |
| solution, while respecting and thanking the work of alumni who have    |
| gone before us.                                                        |
|                                                                        |
| Enjoy !                                                                |
|                                                                        |
| Feedback for bugs or ameliorations to f6hqz-m@hamwlan.net, thank you ! |
\=======================================================================*/
 
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 3 of the 
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; 
 * if not, see <http://www.gnu.org/licenses/>.
 */
 
/*
 * Few of following commented lines are to print some variables values 
 * for debugging, evaluation, curiosity and self training only.        
 * Enable them at your need and personal risk only.                    
 */

#include <wiringPi.h>
#include <stdio.h>

#include "rotaryencoder.h"

// used GPIO for 2 UP/DOWN monitor LEDs
#define	LED_DOWN	25
#define	LED_UP		29

#define DEBOUNCE_DELAY	500	// in microsecondes, 500 microsecondes seems ok

// don't change these init values :
int numberofencoders = 0 ;            // as writed, number of rotary encoders, will be modified by the code later
int numberofbuttons = 0 ;             // as writed, number of axial buttons if any, will be modified by the code later
int step = 0 ;                        // tri-state return variable from the "check_rotation_direction" function, -1, +1 if a step is valid, 0 if false, dubb, bounce
int pre_step = 0 ;                    // previous value of "step", to check if the human operator has changed the rotation direction or not
unsigned char step_was_modified = 0 ; // confirm that encoder has effectively "counted" one more step
unsigned char previous_step = 3 ;     // prepositionning rotary encoder status switches status : the two A and B switches are open if no move
unsigned char sequence ;              // encoder generates a complete 4 steps sequence or one step only (1/4 grey code sequence)
int interrupt ;                       // which wire are we using for A-B rotary encoder and its axial switch if any
unsigned long int now = 0 ;           // used as gap timer (for speed)
unsigned long int lastupdate = 0 ;    // used as gap timer (for speed)
unsigned long int now_2 = 0 ;         // for tests only, to see elementary step duration (1/4 grey code sequence)
unsigned long int lastupdate_2 = 0 ;  // for tests only, to see elementary step duration (1/4 grey code sequence)
unsigned long int now_3 = 0 ;         // used as debouncer timer
unsigned long int lastupdate_3 = 0 ;  // used as debouncer timer
unsigned long int gap = 0 ;           // used as gap timer (for speed)
int speed = 1 ;                       // multiplier for rotation step acceleration if needed
unsigned long int bounces = 0 ;       // cancelled bounces counter, for test/demo

struct encoder *lastEncoder ;
struct encoder *currentEncoder ;

void updateOneEncoder(unsigned char interrupt) ;
int check_rotation_direction(unsigned char previous_step, unsigned char current_step, unsigned char sequence) ;
void updateOneButton(unsigned char interrupt) ;

// Interrupts:
void Interrupt0 (void) { updateOneEncoder(0) ; }
void Interrupt1 (void) { updateOneEncoder(1) ; }
void Interrupt2 (void) { updateOneEncoder(2) ; }
void Interrupt3 (void) { updateOneEncoder(3) ; }
void Interrupt4 (void) { updateOneEncoder(4) ; }
void Interrupt5 (void) { updateOneEncoder(5) ; }
void Interrupt6 (void) { updateOneEncoder(6) ; }
void Interrupt7 (void) { updateOneEncoder(7) ; }
void Inter0 (void) { updateOneButton(0) ; }
void Inter1 (void) { updateOneButton(1) ; }
void Inter2 (void) { updateOneButton(2) ; }
void Inter3 (void) { updateOneButton(3) ; }
void Inter4 (void) { updateOneButton(4) ; }
void Inter5 (void) { updateOneButton(5) ; }
void Inter6 (void) { updateOneButton(6) ; }
void Inter7 (void) { updateOneButton(7) ; }
// these following GPIO exist on Rapsi 2 and B+ only !
void Interrupt21 (void) { updateOneEncoder(21) ; }
void Interrupt22 (void) { updateOneEncoder(22) ; }
void Interrupt23 (void) { updateOneEncoder(23) ; }
void Interrupt24 (void) { updateOneEncoder(24) ; }
void Interrupt25 (void) { updateOneEncoder(25) ; }
void Interrupt26 (void) { updateOneEncoder(26) ; }
void Interrupt27 (void) { updateOneEncoder(27) ; }
void Interrupt28 (void) { updateOneEncoder(28) ; }
void Interrupt29 (void) { updateOneEncoder(29) ; }
void Inter21 (void) { updateOneButton(21) ; }
void Inter22 (void) { updateOneButton(22) ; }
void Inter23 (void) { updateOneButton(23) ; }
void Inter24 (void) { updateOneButton(24) ; }
void Inter25 (void) { updateOneButton(25) ; }
void Inter26 (void) { updateOneButton(26) ; }
void Inter27 (void) { updateOneButton(27) ; }
void Inter28 (void) { updateOneButton(28) ; }
void Inter29 (void) { updateOneButton(29) ; }

//======================================================================
void updateOneEncoder(unsigned char interrupt)
{
	int pin ;
	unsigned char current_step ;
	
	struct encoder *encoder = encoders ;
	
	step = 0 ;	
	lastupdate_2 = now_2 = micros() ; // start chrono 2 - step duration - 
	now = micros() ; // mark elapsed time for chrono 1 - elaped time between two full sequences (or steps if 1/4 rotary encoder model) for "speed rotation"
	now_3 = micros() ; 
	
	if ( (now_3 - lastupdate_3) < DEBOUNCE_DELAY ) // debouncer time in microsecondes, 500 microsecondes seems ok
	{	// debouncer, suppress a "too short too fast" false step
		++bounces ;
//		printf("timer: %8d - gap:%d < DEBOUNCE_DELAY => STOP PROCESSING - RESET TIMER - prev: %d - current: %d - step:  %d \n", now_3 - lastupdate_3, gap, previous_step, current_step, step) ;
	}
	else
	{	// searching the one encoder which has triggered an interrupt
		for (; encoder < encoders + numberofencoders ; encoder++)
		{ 	// each rotary encoder is checked until found the good one
		
			if ((encoder->pin_a == interrupt) || (encoder->pin_b == interrupt)) 
			{	// found the correct encoder which is interrupting
				int MSB = digitalRead(encoder->pin_a) ;
				int LSB = digitalRead(encoder->pin_b) ;
				current_step = (MSB << 1) | LSB ;
				previous_step = encoder->lastEncoded ; // read last state in memo

				if(current_step != encoder->lastEncoded)
				{ 	// sampled A & B values changed (suppress duplicated after bounce)
//					printf("timer!=0         => goto CHECK-DIRECTION \n") ;
					check_rotation_direction (previous_step, current_step, encoder->sequence) ;
//					printf("step:%2d          <= return from CHECK-DIRECTION \n", step) ;
				}
				break ; // found the correct encoder, exit from the loop, winning machine time
			}
		}
		//--------------------------------------------------------------
		// decreasing at lower speed if switched to another rotary encoder now
		currentEncoder = encoder ;
		if (lastEncoder != encoder) 
			{ speed = 1 ; }
		lastEncoder = encoder ;
		//--------------------------------------------------------------		
		// effective new value, can record "encoder->value" now
		if (step != 0)
		{			
			// mark elapsed time for chrono 1 (gap time now)
			now = micros() ;
			gap = (now - lastupdate) ;
			//----------------------------------------------------------
			// 4 rotary encoder speeds : normal, fast, very fast, 
			// hyper fast, and a short pause to reset to normal.
			// checked when a full sequence has been terminated only,
			// verified by "step_was_modified" status.
			if (step_was_modified)
			{
				if (gap > encoder->pause)
					{ speed = 1 ; } // pause = speed reset
				else if (gap < encoder->speed_Level_Threshold_4)
					{ speed = encoder->speed_Level_Multiplier_4 ; }
				else if (gap < encoder->speed_Level_Threshold_3)
					{ speed = encoder->speed_Level_Multiplier_3 ; }
				else if (gap < encoder->speed_Level_Threshold_2)
					{ speed = encoder->speed_Level_Multiplier_2 ; }					
				step_was_modified = 0 ;
			}
			//----------------------------------------------------------
			if (pre_step != step)
			{ // rotation direction changed, speed is changed to low level now
				step_was_modified = 0 ;
				speed = 1 ;
			}
			// check if limit value raised, in normal and reverse rotation direction then write the correct value in memory
			if (encoder->reverse)
			{
				if ( ( (encoder->value + (speed * - step)) <= encoder->high_Limit ) && ( (encoder->value + (speed * - step)) >= encoder->low_Limit) )
				{
					encoder->value = encoder->value + (speed * - step) ;
				} 
				else
				{
					if (step == -1)
					{
						if (encoder->looping) 
							{ encoder->value = encoder->low_Limit ; }
						else 
							{ encoder->value = encoder->high_Limit ; }
					}
					else
					{
						if (encoder->looping)
							{ encoder->value = encoder->high_Limit ; }
						else
							{ encoder->value = encoder->low_Limit ;	}
					}
				}
			}
			else
			{
				if ( ( (encoder->value + (speed * step)) <= encoder->high_Limit ) && ( (encoder->value + (speed * step)) >= encoder->low_Limit ) )
				{
					encoder->value = encoder->value + (speed * step) ;
				}
				else
				{
					if (step == -1)
					{
						if (encoder->looping)
							{ encoder->value = encoder->high_Limit ; }
						else
							{ encoder->value = encoder->low_Limit ; }
					}
					else
					{
						if (encoder->looping)
							{ encoder->value = encoder->low_Limit ; }
						else
							{ encoder->value = encoder->high_Limit ; }
					}
				}
			}
			// when step is realy modified/validated (-1 or 1) :
			step_was_modified = 1 ;
			pre_step = step ; // -1 or 1, depending rotation direction
			previous_step = current_step ; // 4 possible binary values 00, 10, 00, 01
			lastupdate = now = micros() ; // reset/start speed rotate timer (for gap/encoder rotation speed measurement)
		}
		// when the binary value (current_step: 00, 10, 00, 01) is modified 
		// but not yet the "step" (-1, 1) as just top due to full sequence instead of 1/4 step
		encoder->lastEncoded = current_step ;
	}	
	now_2 = micros() ; // top chrono 2 - step duration
	lastupdate_3 = now_3 = micros() ; // reset/restart bounce timer
}
//======================================================================
int check_rotation_direction(unsigned char previous_step, unsigned char current_step, unsigned char sequence)
{ // rotation direction finder
//	printf("DIR-FINDER-IN - previous-step: %d - current-step: %d - sequence: %d \n", previous_step, current_step, sequence) ;

	switch (current_step) 
	{
		case 0:
			switch (previous_step)
			{
				case 1:
					if (sequence) { step = 0 ; } else { step = 1 ;} ;
					digitalWrite (LED_UP, ON) ;		// On
					digitalWrite (LED_DOWN, OFF) ;	// Off
					break ;
				case 2:
					if (sequence) { step = 0 ; } else { step = -1 ;} ;
					digitalWrite (LED_DOWN, ON) ;	// On
					digitalWrite (LED_UP, OFF) ;	// Off
					break ;
				default:
					break ;
			}
			break ;	
									
		case 1:
			switch (previous_step)
			{
				case 0:
					if (sequence) { step = 0 ; } else { step = -1 ;} ;
					digitalWrite (LED_DOWN, ON) ;	// On
					digitalWrite (LED_UP, OFF) ;	// Off
					break ;
				case 3:
					if (sequence) { step = 0 ; } else { step = 1 ;} ;
					digitalWrite (LED_UP, ON) ;		// On
					digitalWrite (LED_DOWN, OFF) ;	// Off
					break ;
				default:
					break ;
			}
			break ;
									
		case 2:
			switch (previous_step)
			{
				case 0:
					if (sequence) { step = 0 ; } else { step = 1 ;} ;
					digitalWrite (LED_UP, ON) ;		// On
					digitalWrite (LED_DOWN, OFF) ;	// Off
					break ;
				case 3:
					if (sequence) { step = 0 ; } else { step = -1 ;} ;
					digitalWrite (LED_DOWN, ON) ;	// On
					digitalWrite (LED_UP, OFF) ;	// Off
					break ;
				default:
					break ;
			}
			break ;

		case 3:
			switch (previous_step)
			{
				case 1:
					step = -1 ;
					digitalWrite (LED_DOWN, ON) ;	// On
					digitalWrite (LED_UP, OFF) ;	// Off
					break ;
				case 2:
					step = 1 ;
					digitalWrite (LED_UP, ON) ;		// On
					digitalWrite (LED_DOWN, OFF) ;	// Off
					break ;
				default:
					break ;
			}
			break ;	
						
		default:
			break ;
	}
	return step ;
}
//======================================================================
void updateOneButton(unsigned char interrupt)
{
	int pin ;	
	struct button *button = buttons ;
	
	now_3 = micros() ; // top chrono -> wait time between two interrupt states
	
	if ( (now_3 - lastupdate_3) < DEBOUNCE_DELAY ) // debouncer in microsecondes
	{ // debouncer
		++bounces ;
	}
	else
	{
		for (; button < buttons + numberofbuttons ; button++)
		{ 	// each button is checked until found the good one
		
			if (button->pin == interrupt) 
			{	// found the correct encoder which is interrupting
				button->value = digitalRead(button->pin) ;
				break ; // found the correct encoder, exit from the loop
			}
		}
				
		if (button->value)
		{
//			printf("Bouton %d ON !\n") ;
			digitalWrite (LED_DOWN, ON) ;	// On
			digitalWrite (LED_UP, ON) ;		// On
		}
		else
		{
//			printf("Bouton %d OFF !\n") ;
			digitalWrite (LED_DOWN, ON) ;	// On
			digitalWrite (LED_UP, ON) ;		// On
		}
	}	
	
	lastupdate_3 = now_3 = micros() ; // reset/start (gap measurement)
}
//======================================================================
struct encoder *setupencoder(char *label, int pin_a, int pin_b, unsigned char sequence, 
	unsigned char reverse, unsigned char looping, long int low_Limit, long int high_Limit, 
	long int value, unsigned long int pause, 
	int speed_Level_Threshold_2, int speed_Level_Threshold_3, int speed_Level_Threshold_4,
	int speed_Level_Multiplier_2, int speed_Level_Multiplier_3, int speed_Level_Multiplier_4)
{
	int pin ;
	int loop = 0 ;
	
	if (numberofencoders > max_encoders)
	{
		printf("Maximum number of encodered exceded: %i\n", max_encoders) ;
		return NULL ;
	}
	
	// show pins; addresses and label of each rotary encoder
//	printf("pin A:%d - pin B:%d - addresse:%d - label:%s \n\n", pin_a, pin_b, label, label) ;
	
	struct encoder *newencoder = encoders + numberofencoders++ ;
	newencoder->label = label ; // name or label as "Volume" or "Balance" or "Treble", etc...
	newencoder->pin_a = pin_a ; // which GPIO received the A pin from the rotary encoder
	newencoder->pin_b = pin_b ; // which GPIO received the B pin from the rotary encoder
	newencoder->sequence = sequence ; // rotary encoder sends a complete 4 steps sequence (full cycle) or 1/4 cycle only per detent
	newencoder->reverse = reverse ; // encoder much count or rotate in reverse
	newencoder->looping = looping ; // looping from one end to other when value limits are reached, from high_Limit to low_Limit and reverse
	newencoder->low_Limit = low_Limit ; // max lowerst value, could be negative
	newencoder->high_Limit = high_Limit ; // max higherst value, could be negative
	newencoder->value = value ; // used to drive your solution, can be the starting default value or something in memory somewhere
	newencoder->lastEncoded = 3 ; // memo to compare 2 consecutive steps binary values, don't modify
	newencoder->pause = pause ; // pause time between gaps to reset rotary encoder speed level, in microsecondes
	newencoder->speed_Level_Threshold_2 = speed_Level_Threshold_2 ; // second speed shift level threshold value
	newencoder->speed_Level_Threshold_3 = speed_Level_Threshold_3 ; // third speed level threshold value
	newencoder->speed_Level_Threshold_4 = speed_Level_Threshold_4 ; // fourth speed level threshold value
	newencoder->speed_Level_Multiplier_2 = speed_Level_Multiplier_2 ; // second speed level multiplier value
	newencoder->speed_Level_Multiplier_3 = speed_Level_Multiplier_3 ; // third speed level multiplier value
	newencoder->speed_Level_Multiplier_4 = speed_Level_Multiplier_4 ; // fourth speed level multiplier value
  
	// Rotary encoder settings
	pinMode(pin_a, INPUT) ;
	pinMode(pin_b, INPUT) ;
	pullUpDnControl(pin_a, PUD_UP) ;	// pullup internal resistor
	pullUpDnControl(pin_b, PUD_UP) ;	// pullup internal resistor
  
	// interrupts selection
	for (loop = 0 ; loop < 2 ; loop++)
	{
		if (loop == 0)
			{ pin = pin_a ; } 
		else if (loop == 1)
			{ pin = pin_b ; }
	
		switch (pin)
		{
			case 0:
				wiringPiISR (0, INT_EDGE_BOTH, &Interrupt0) ;
				break ;
			case 1:
				wiringPiISR (1, INT_EDGE_BOTH, &Interrupt1) ;
				break ;
			case 2:
				wiringPiISR (2, INT_EDGE_BOTH, &Interrupt2) ;
				break ;
			case 3:
				wiringPiISR (3, INT_EDGE_BOTH, &Interrupt3) ;
				break ;
			case 4:
				wiringPiISR (4, INT_EDGE_BOTH, &Interrupt4) ;
				break ;
			case 5:
				wiringPiISR (5, INT_EDGE_BOTH, &Interrupt5) ;
				break ;
			case 6:
				wiringPiISR (6, INT_EDGE_BOTH, &Interrupt6) ;
				break ;
			case 7:
				wiringPiISR (7, INT_EDGE_BOTH, &Interrupt7) ;
				break ;
			case 21:
				wiringPiISR (21, INT_EDGE_BOTH, &Interrupt21) ;
				break ;
			case 22:
				wiringPiISR (22, INT_EDGE_BOTH, &Interrupt22) ;
				break ;
			case 23:
				wiringPiISR (23, INT_EDGE_BOTH, &Interrupt23) ;
				break ;
			case 24:
				wiringPiISR (24, INT_EDGE_BOTH, &Interrupt24) ;
				break ;
			case 25:
				wiringPiISR (25, INT_EDGE_BOTH, &Interrupt25) ;
				break ;
			case 26:
				wiringPiISR (26, INT_EDGE_BOTH, &Interrupt26) ;
				break ;
			case 27:
				wiringPiISR (27, INT_EDGE_BOTH, &Interrupt27) ;
				break ;
			case 28:
				wiringPiISR (28, INT_EDGE_BOTH, &Interrupt28) ;
				break ;
			case 29:
				wiringPiISR (29, INT_EDGE_BOTH, &Interrupt29) ;
				break ;
			default:
				break ;
		}
	}
	return newencoder ;
}
//======================================================================
struct button *setupbutton(char *label, int pin, long int value)
{
	if (numberofbuttons > max_buttons)
	{
		printf("Max number of buttons exceeded: %i\n", max_buttons) ;
		return NULL ;
	}
	
	struct button *newbutton = buttons + numberofbuttons++ ;
	newbutton->label = label ; // name or label as "Effect" or "Mute" or "+10dB", etc...
	newbutton->pin = pin ;
	newbutton->value = value ; // memorized for use later in your software
	
	//Button settings
	pinMode(pin,INPUT) ;
	pullUpDnControl(pin, PUD_UP) ;  // pullup internal resistor
	
	// interrupts
	switch (pin)
	{
		case 0:
			wiringPiISR (0, INT_EDGE_BOTH, &Inter0) ;
			break ;
		case 1:
			wiringPiISR (1, INT_EDGE_BOTH, &Inter1) ;
			break ;
		case 2:
			wiringPiISR (2, INT_EDGE_BOTH, &Inter2) ;
			break ;
		case 3:
			wiringPiISR (3, INT_EDGE_BOTH, &Inter3) ;
			break ;
		case 4:
			wiringPiISR (4, INT_EDGE_BOTH, &Inter4) ;
			break ;
		case 5:
			wiringPiISR (5, INT_EDGE_BOTH, &Inter5) ;
			break ;
		case 6:
			wiringPiISR (6, INT_EDGE_BOTH, &Inter6) ;
			break ;
		case 7:
			wiringPiISR (7, INT_EDGE_BOTH, &Inter7) ;
			break ;
		case 21:
			wiringPiISR (21, INT_EDGE_BOTH, &Inter21) ;
			break ;
		case 22:
			wiringPiISR (22, INT_EDGE_BOTH, &Inter22) ;
			break ;
		case 23:
			wiringPiISR (23, INT_EDGE_BOTH, &Inter23) ;
			break ;
		case 24:
			wiringPiISR (24, INT_EDGE_BOTH, &Inter24) ;
			break ;
		case 25:
			wiringPiISR (25, INT_EDGE_BOTH, &Inter25) ;
			break ;
		case 26:
			wiringPiISR (26, INT_EDGE_BOTH, &Inter26) ;
			break ;
		case 27:
			wiringPiISR (27, INT_EDGE_BOTH, &Inter27) ;
			break ;
		case 28:
			wiringPiISR (28, INT_EDGE_BOTH, &Inter28) ;
			break ;
		case 29:
			wiringPiISR (29, INT_EDGE_BOTH, &Inter29) ;
			break ;
		default:
			break ;
	}
	return newbutton ;
}