/*
 * Project4Stopwatch.c
 *
 * Created: 3/17/2017 1:59:22 PM
 * Author : Farez Halim (1423780) and Tamara Oldham ()
 *
 * This program utilizes the interrupt service routine to track the time passed after the press of a pushbutton.
 * The time in “HH:MM:SS.hh” format (where hh is the number of hundredths of seconds) is displayed underneath the current state of the stopwatch.
 * Pressing the same Run/Stop pushbutton for an extended time when the stopwatch is paused will reset the recorded time to 00:00:00:00.
 * A PS1420P02CT piezo buzzer is used to generate a 1kHz tone for 100ms at the start of each second that passes when the stopwatch is running;
 */

// header includes
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "lcd.h"

// definitions for delay
#define F_CPU 10000000UL // 10MHz
#include <util/delay.h>

// declaration of global variables used for interrupts
volatile uint8_t millisecond_counter = 0;
volatile uint8_t millisecond = 0;
volatile uint8_t second = 0;
volatile uint8_t minute = 0;
volatile uint8_t hour = 0;
volatile char *state;

// code to enable the use of printf() to the lcd
int lcd_putchar(char c, FILE *unused) {
	(void) unused;
	lcd_putc(c);
	return 0;
}
FILE lcd_stream = FDEV_SETUP_STREAM(lcd_putchar, NULL, _FDEV_SETUP_WRITE);

typedef enum {STATE_STOPWATCH_STOPPED, STATE_STOPWATCH_RUNNING, STATE_STOPWATCH_PAUSED} state_t; // state machine states for stopwatch
typedef enum {STATE_READY_FOR_PRESS, STATE_WAITING_FOR_RELEASE, STATE_DEBOUNCING_PRESS, STATE_DEBOUNCING_RELEASE, STATE_WAITING_FOR_LONG_RELEASE} state_t1; // state machine states for the button

// function that returns true if button PB0 is pressed
uint8_t button_pressed() {
	return !(PINB & 0x01);
}

// function that utilizes the concept of state machines
// to return true if the button is pressed longer than one second
// false if the button press is shorter than one second
bool long_press(void) {
	uint16_t timeout = 0;
	state_t1 button_state = STATE_READY_FOR_PRESS;
	bool long_press = false;

	while (1) {

		switch (button_state) {
			case STATE_READY_FOR_PRESS : {
				if (button_pressed()) {
					button_state = STATE_DEBOUNCING_PRESS;
				}
				break;
			}

			case STATE_DEBOUNCING_PRESS : {
				_delay_ms(50);
				button_state = STATE_WAITING_FOR_RELEASE;
				timeout = 10000000;
				break;
			}

			case STATE_WAITING_FOR_RELEASE : {
				if (!button_pressed()) {
					button_state = STATE_DEBOUNCING_RELEASE;
				} else if (timeout > 0){
					--timeout;
				} else {
					button_state = STATE_WAITING_FOR_LONG_RELEASE;

				}
				break;
			}

			case STATE_DEBOUNCING_RELEASE : {
				_delay_ms(50);
				button_state = STATE_READY_FOR_PRESS;
				return long_press;
				break;
			}

			case STATE_WAITING_FOR_LONG_RELEASE : {
				if (!button_pressed()) {
					long_press = true;
					button_state = STATE_DEBOUNCING_RELEASE;
				}
				break;
			}

		}
	}
}

// interrupt service routine that is called on Timer1 Compare Match A to keep track of the time
// this ISR also plays the buzzer at every second for 100ms
ISR(TIMER1_COMPA_vect,ISR_BLOCK){
	OCR1A += 50000;
	millisecond_counter += 1;

	if ((millisecond_counter == 2)){
		millisecond_counter = 0;
		millisecond += 1;
		if (millisecond == 10) {
			// setting the timer 0 prescaler to 0 stops the timer
			// which stops the buzzer from playing
			TCCR0B = 0;
		}
		if(millisecond == 100){
			second += 1;
			// setting the timer 0 prescaler starts the timer
			// which then plays the buzzer
			TCCR0B |= (1 << CS01) | (1 << CS00);
			millisecond = 0;
			if (second == 60){
				minute += 1;
				second = 0;
				if (minute == 60){
					hour += 1;
					minute = 0;
				}
			}
		}
	}
	printf("%s\n%02d:%02d:%02d:%02d\n",state,hour, minute, second, millisecond);
}


int main(void) {
	// set CKSEL 3:0 to 0111 with SUT 1:0 to 11

	// button initialization
	PORTB |= (1 << PB0); // enable pull up resistor
	DDRB &= ~(1 << PB0); // set button as input

	// intialize state matchine for the stopwatch
	state_t stopwatch_state = STATE_STOPWATCH_STOPPED;
	state = "Stopped";

	TCCR1B = 0x01; // Turn on timer1

	// initalization of the timer that generates a 1kHz square wave for the buzzer
	// desired frequency is 1kHz. therefore, using a prescaler of 64, OCR0A must be set to 155
	// timer 0 is set to CTC mode, and to toggle on compare match
	DDRD |= (1 << PD6); // set OC0A pin as output
	TCCR0A |= (1 << WGM01) | (1 << COM0A0);
	OCR0A = 155;

	stdout = &lcd_stream;
	lcd_init(LCD_DISP_ON); 		/* initialize lcd, display on, cursor off */
	lcd_clrscr();             /* clear screen of lcd */

  while (1) {
   	  switch(stopwatch_state) {
   			case STATE_STOPWATCH_STOPPED : {
   				// when timer stopped, disable global interrupts and output compare interrupt
   				// set all time counting variables to 0 and display the 0's on lcd
   				// wait for button to be pressed
   				cli();
   				TIMSK1 = 0x00;
   				millisecond = 0;
					second = 0;
					minute = 0;
					hour = 0;
					state = "Stopped";
					printf("%s\n%02d:%02d:%02d:%02d\n", state, hour, minute, second, millisecond);

					if (!(long_press())) {
						stopwatch_state = STATE_STOPWATCH_RUNNING;
						break;
					} else if (long_press()) {
						stopwatch_state = STATE_STOPWATCH_RUNNING;
						break;
					}
   			} // end stopped case

   			case STATE_STOPWATCH_RUNNING : {
   				// when timer is running, enable all necessary parameters for the interrupt
   				// wait for button press for state change
   				OCR1A = 50000; //Timer count for 5ms
					TIMSK1 = 0x02; //Turn on OCIEA interrupt enable
					sei(); 				 //Turn on interrupts
					TCNT1 = 0;
					state = "Running";
					if (!(long_press())) {
						stopwatch_state = STATE_STOPWATCH_PAUSED;
						break;
					} else {
						break;
					}
   			} // end runnign case

   			case STATE_STOPWATCH_PAUSED : {
   				// disable the interrupts to stop the stopwatch values from incrementing
   				// if button is short pressed, start the stopwatch again
   				// if button is long pressed, reset the stopwatch and set to stopped state
   				cli();				 // disable interrupts
   				TIMSK1 = 0x00; // disable output compare interrupt
   				state = "Paused";
   				printf("%s   \n%02d:%02d:%02d:%02d\n",state,hour, minute, second, millisecond);
   				if (!(long_press())) {
						stopwatch_state = STATE_STOPWATCH_RUNNING;
						break;
					} else {
						stopwatch_state = STATE_STOPWATCH_STOPPED;
						break;
					}
   			} // end paused case
   		} // end switch
   } // end while (1)
}