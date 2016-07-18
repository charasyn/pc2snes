#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <termios.h>

struct JoypadState { // conf. for PS4
	uint8_t button[13];
	int16_t axis[8]; // we only care up to the DPAD
} jss;

struct JSEvent {
	unsigned int time;    /* event timestamp in milliseconds */
	short value;          /* value */
	unsigned char type;   /* event type */
	unsigned char number; /* axis/button number */
} jse;

// serap.h transmit end
#define SERAPH_HEADER_LEN 8
#define BUFLEN 32
#define PNDLEN BUFLEN-SERAPH_HEADER_LEN
#define DATLEN 8
uint8_t buffer[BUFLEN];

void poll(int fd){
	int i;
	while(1){
		i=read(fd,&jse,sizeof(struct JSEvent));
		if(i!=sizeof(struct JSEvent))
			break;
		switch(jse.type){
			case 1:
				jss.button[jse.number]=(uint8_t)jse.value;
				break;
			case 2:
				jss.axis[jse.number]=jse.value;
				break;
		}
	}
	
	#define tb(x)  ((x)?1:0)
	#define pos(x) (x>10000)
	#define neg(x) (x<-10000)
	uint8_t tmp;
	// BYetUDLR
	tmp=       tb(jss.button[1]);
	tmp=tmp<<1|tb(jss.button[0]);
	tmp=tmp<<1|tb(jss.button[8]);
	tmp=tmp<<1|tb(jss.button[9]);
	tmp=tmp<<1|neg(jss.axis[1])|neg(jss.axis[7]);
	tmp=tmp<<1|pos(jss.axis[1])|pos(jss.axis[7]);
	tmp=tmp<<1|neg(jss.axis[0])|neg(jss.axis[6]);
	tmp=tmp<<1|pos(jss.axis[0])|pos(jss.axis[6]);
	buffer[SERAPH_HEADER_LEN]=tmp;
	
	// AXLR0000
	tmp=       tb(jss.button[2]);
	tmp=tmp<<1|tb(jss.button[3]);
	tmp=tmp<<1|tb(jss.button[4])|tb(jss.button[6]);
	tmp=tmp<<1|tb(jss.button[5])|tb(jss.button[7]);
	tmp<<=4;
	buffer[SERAPH_HEADER_LEN+1]=tmp;
	#undef tb
	#undef pos
	#undef neg
}

int main(int argc, char** argv)
{
	int r;
	int jfd,sfd;
	
	jfd = open("/dev/input/js0",O_RDONLY|O_NONBLOCK);
	if(jfd<0){
		printf("Couldn't open joystick\n");
		return 1;
	}
	printf("js0 opened\n");
		
	struct termios oldtio,newtio;
	
	sfd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY); 
	if (sfd <0){
		printf("Unable to open serial IO\n");
		close(jfd);
		return 1;
	}
	tcgetattr(sfd,&oldtio); /* save current port settings */

	memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag = B38400 | CRTSCTS | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
        
	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;
	
	// this shouldn't matter because we never read
	newtio.c_cc[VTIME]    = 0; // inter-character timer unused
	newtio.c_cc[VMIN]     = 5; // blocking read until 5 chars received
	tcflush(sfd, TCIFLUSH);
	tcsetattr(sfd,TCSANOW,&newtio);
	
	printf("Serial configured\n");
	
	memset(buffer,0,BUFLEN);
	memset(&jss,0,sizeof(struct JoypadState));
	
	buffer[0]='S';
	buffer[1]='E';
	buffer[2]='R';
	buffer[3]=0xaf;
	buffer[4]=(uint8_t)PNDLEN;
	buffer[5]=(uint8_t)DATLEN;
	
	printf("Starting communication\nPress the PS button (12) to exit...\n");
	while (1) {
		poll(jfd);
		write(sfd,buffer,BUFLEN);
		//if(buffer[8]) printf("%x\t",buffer[8]);
		if(jss.button[12])
			break;
		usleep(990);
	}
	tcsetattr(sfd,TCSANOW,&oldtio);
	close(sfd);
	close(jfd);
}
