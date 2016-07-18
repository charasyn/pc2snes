//serap.h
// seraph is a protocol for sending realtime data over serial because
// the teensy raw HID thing was not working on the PC side
//   name is fitting
#ifndef _SERAP_H
#define _SERAP_H

// The protocol works as follows:
// You must send 'S','E','R',0xaf as the first 4 bytes
// seraph will loop until it finds that header
// next are the packet length and data length as two uint8_t
// and then two bytes of padding
// after that, the packet length is read from the input buffer
// only the data length bytes are written to the output buffer

// Required functions:
// int16_t SERAPH_READ(void);
// 0-255 on success, -1 on fail
#ifndef SERAPH_READ
#define SERAPH_READ usb_serial_getchar
#endif
// uint8_t SERAPH_AVAILABLE(void);
// number of buffered bytes
#ifndef SERAPH_AVAILABLE
#define SERAPH_AVAILABLE usb_serial_available
#endif

// Code:
#include <stdint.h>
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

#endif
