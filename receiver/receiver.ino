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

/**************************************\
* PLEASE READ THE FOLLOWING SECTION!!! *
\**************************************/

// Important defines:
//   These defines control what pins are used for I/O.
//   This can be a bit complicated to understand,
//     so the default configuration is as follows:

// Arduino:
//   https://www.arduino.cc/en/Hacking/Atmega168Hardware
// default pin config is:
//   SNES data  to Arduino pin 4
//   SNES clock to Arduino pin 3
//   SNES latch to Arduino pin 2

#ifdef ARDUINO
// status LED on pin 13
#define led_reg B
#define led_bit 5

// SNES data on pin 4
#define data_reg D
#define data_bit 4

// SNES clock on pin 3
#define clk_reg D
#define clk_bit 3

// SNES latch on pin 2
#define latch_reg D
#define latch_bit 2

// These defines have to be adjusted according to the data sheet
//   if you change the latch pin. Must be falling edge.
// http://www.atmel.com/images/Atmel-8271-8-bit-AVR-Microcontroller-ATmega48A-48PA-88A-88PA-168A-168PA-328-328P_datasheet_Complete.pdf
#define latch_EICRA_val 0x02
#define latch_EIMSK_val _BV(INT0)
#define latch_ISR_vect  INT0_vect

// Teensy:
//   https://www.pjrc.com/teensy/pinout.html
// default pin config is:
//   SNES data  to Teensy C7
//   SNES clock to Teensy D1
//   SNES latch to Teensy D0

#elif defined(TEENSY)
// status LED on D6 (built-in LED)
#define led_reg D
#define led_bit 6

// SNES data on C7 
#define data_reg C
#define data_bit 7

// SNES clock on D1
#define clk_reg D
#define clk_bit 1

// SNES latch on D0
#define latch_reg D
#define latch_bit 0

// These defines have to be adjusted according to the data sheet
//   if you change the latch pin. Must be falling edge.
// http://www.atmel.com/Images/Atmel-7766-8-bit-AVR-ATmega16U4-32U4_Datasheet.pdf
#define latch_EICRA_val 0x02
#define latch_EIMSK_val _BV(INT0)
#define latch_ISR_vect  INT0_vect

#endif

#define BUFFER_SIZE 9
#define MAX_BYTES   8

// Useful defines
#ifndef _BV
#define _BV(n) (1<<(n))
#endif

#define EXPAND(...) __VA_ARGS__
#define CAT(a, ...) PRIMITIVE_CAT(a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__

#define PBV(r,b) _BV(CAT(P,CAT(EXPAND(r),EXPAND(b))))
#define PORT(r)  CAT(PORT,EXPAND(r))
#define PIN(r)   CAT(PIN,EXPAND(r))
#define DDR(r)   CAT(DDR,EXPAND(r))

#define data_low   (PORT(data_reg) &= ~PBV(data_reg,data_bit))
#define data_high  (PORT(data_reg) |=  PBV(data_reg,data_bit))

#define led_toggle (PORT(led_reg) ^=  PBV(led_reg,led_bit))
#define led_high   (PORT(led_reg) |=  PBV(led_reg,led_bit))
#define led_low    (PORT(led_reg) &= ~PBV(led_reg,led_bit))

#define clk   (PIN(clk_reg) & PBV(clk_reg,clk_bit))
#define latch (PIN(latch_reg) & PBV(latch_reg,latch_bit))

// Includes
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Arduino / Teensy specific defines
#ifdef ARDUINO
	// Arduino code
	#define SERAPH_READ Serial.read
	#define SERAPH_AVAILABLE Serial.available
	void config_io(void){
		Serial.begin(115200);
	}
#elif defined(TEENSY)
	// Teensy code
	#include "usb_serial.h"
	#define SERAPH_READ usb_serial_getchar
	#define SERAPH_AVAILABLE usb_serial_available
	void config_io(void){
		// set for 16 MHz clock
		// idk if this is necessary but i'm keeping it here anyways
		// this is the only piece of PJRC code still in this file
		// expanded the macro because why not
		CLKPR = 0x80; CLKPR = 0;

		// Initialize the USB, and then wait for the host to set configuration.
		// If the Teensy is powered without a PC connected to the USB port,
		// this will wait forever.
		usb_init();
		while (!usb_configured()) /* wait */ ;
	}
	void setup(void);
	void loop(void);
	int main(void){
		setup();
		loop();
	}
#else
	#error "Unknown microcontroller!"
#endif

uint8_t seraph_receive(uint8_t *buffer, uint8_t length){
	uint8_t fullen,datlen,i,data;
	int16_t read;
	#define safe_do(x) \
		if(!SERAPH_AVAILABLE()) return 0; \
		if((read=SERAPH_READ())<0) return 0; \
		data=(uint8_t)read; \
		x
	#define check(x) safe_do(if(data!=x) continue;)
	while(1){
		check('S')
		check('E')
		check('R')
		check(0xaf);
		break;
	}
	#undef check
	safe_do(fullen=data;)
	safe_do(datlen=data;)
	safe_do()
	safe_do()
	if(datlen>length)
		datlen=length;
	for(i=0;i<fullen;i++){
		safe_do()
		if(i<datlen)
			buffer[i]=data;
	}
	return i;
	#undef safe_do
}

// Main program:
volatile uint8_t sel_buf=0;
uint8_t buffer0[BUFFER_SIZE];
uint8_t buffer1[BUFFER_SIZE];

void setup()
{
	int8_t r;

	config_io();
	
	// configure pins
	// hopefully this will be optimised into a few instructions
	DDR(data_reg)  |=  PBV(data_reg,data_bit);   // data out
	DDR(led_reg)   |=  PBV(led_reg,led_bit);     // led out
	DDR(clk_reg)   &= ~PBV(clk_reg,clk_bit);     // clk in
	DDR(latch_reg) &= ~PBV(latch_reg,latch_bit); // latch in

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
	EIFR  = 0xff;            // clear interrupt flags (for good luck)
	EICRA = latch_EICRA_val; // interrupt on falling edge of INT0
	EIMSK = latch_EIMSK_val; // enable only INT0 (D0 on ATmega32u4)
	sei();
	
	while(1){
		r = seraph_receive(sel_buf?buffer1:buffer0, 8);
		if(r>0){
			led_high;
			sel_buf=!sel_buf;
		}
	}
}

void loop(){

}

// Interrupt + related routines:

// returning 1 means the wait failed
static inline uint8_t wait_clock(void){
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

ISR(latch_ISR_vect)
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
