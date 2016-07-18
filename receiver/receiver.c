/* receiver.c
 * The AVR part of pc2snes
 * Based on:
 * 
 * Teensy RawHID example
 * http://www.pjrc.com/teensy/rawhid.html
 * Copyright (c) 2009 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above description, website URL and copyright notice and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_serial.h"

#include "serap.h"

#define BUFFER_SIZE 8
#define MAX_BYTES   8

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

volatile uint8_t sel_buf=0;
uint8_t buffer0[BUFFER_SIZE];
uint8_t buffer1[BUFFER_SIZE];

#define data_low  (PORTC &= ~_BV(PC7))
#define data_high (PORTC |=  _BV(PC7))

#define led_toggle (PORTD ^=  _BV(PD6))
#define led_high   (PORTD |=  _BV(PD6))
#define led_low    (PORTD &= ~_BV(PD6))

#define clk   (PIND & _BV(PD1))
#define latch (PIND & _BV(PD0))

int main(void)
{
	int8_t r;

	// set for 16 MHz clock
	// idk if this is necessary but i'm keeping it here anyways
	// this is the only piece of PJRC code still in this file
	CPU_PRESCALE(0);

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.
	usb_init();
	while (!usb_configured()) /* wait */ ;

	// configure pins
	DDRC = _BV(PC7); // only PC7 output
	DDRD = _BV(PD6); // only PD6 output (led)

	led_low;

	for(r=0;r<BUFFER_SIZE;r++){
		buffer0[r]=0;
		buffer1[r]=0;
	}

	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	// I don't think we have to wait for 1 second (maybe on windows?)
	_delay_ms(1000);

	cli();
	// configure interrupt
	EIFR  = 0x4f;      // clear interrupt flags 
	EICRB = 0;         // we don't have to clear this, but we do anyways
	EICRA = 0x02;      // interrupt on falling edge of INT0
	EIMSK = _BV(INT0); // enable only INT0 (D0 on ATmega32u4)

	sei();
	
	while(1){
		r = seraph_receive(sel_buf?buffer1:buffer0, 8);
		if(r>0){
			// update LCD? if I get the LCD lib off my other comp
			
			led_high;
			sel_buf=!sel_buf;
		}
	}
}

// returning 1 means the wait failed
uint8_t wait_clock(void){
	static uint8_t c8;
	c8=0;
	for(;(clk)&&(c8<16);c8++){
		_delay_us(0.5); // we delay by a short amount of time to avoid
		                //   missing the edge of the clock cycle
		                // the time could be shorter but, no need :)
	}
	if(clk)
		return 1;
	c8=0;
	for(;(!clk)&&(c8<16);c8++){
		_delay_us(0.5);
	}
	if(!clk)
		return 1;
	return 0;
}

ISR(INT0_vect)
{
	// do these have to be static? probably not
	static uint8_t bits, bytes, tmp;
	
	// this interrupt only triggers on the falling edge of latch
	//   so we don't have to check the latch pin or anything
	
	bits=0;
	bytes=0;

	led_low;

	// if sel_buf then we were reading into buffer 1
	//   therefore access buffer 0
	if(sel_buf){
		tmp=buffer0[0];
		while(bytes<MAX_BYTES){ // this could be done faster in assembly
			                    //   but it's not necessary
			// push the bits out
			if(tmp&0x80)
				data_low;
			else
				data_high;
			tmp<<=1;
			bits++;
			if(bits==8){
				bits=0;
				bytes++;
				tmp=buffer0[bytes];
			}
			if(wait_clock())
				return;
		}
	}
	else {
		tmp=buffer1[0];
		while(bytes<MAX_BYTES){
			if(tmp&0x80)
				data_low;
			else
				data_high;
			tmp<<=1;
			bits++;
			if(bits==8){
				bits=0;
				bytes++;
				tmp=buffer1[bytes];
			}
			if(wait_clock())
				return;
		}
	}
	data_low;
}
