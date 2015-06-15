// pins 8-13
#define S_DATA  8
#define S_LATCH 9
#define S_CLOCK 10

#define LED
#define MAXFRAME 1024 // max num of bits in one clock

#define checkLatch (PINB&(1<<(S_LATCH-8)))
#define checkClock (PINB&(1<<(S_CLOCK-8)))
inline void writeData(byte x){ if(x) PORTB|=(1<<(S_DATA-8)); else PORTB&=~(1<<(S_DATA-8)); }

inline byte reverse(byte b) { // thanks "sth" from StackOverflow
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

byte bits[MAXFRAME];
unsigned short i,count;
#ifdef LED
byte led;
#endif

void setup() {
  Serial.begin(112500);
  
  pinMode(S_DATA,OUTPUT);
  pinMode(S_LATCH,INPUT_PULLUP);
  pinMode(S_CLOCK,INPUT_PULLUP);
  digitalWrite(S_DATA,LOW);
  
  #ifdef LED
  led=0;
  pinMode(13,OUTPUT);
  #endif
  
  for(i=0;i<MAXFRAME;i++){
    bits[i]=1;
  }
  count=0;
}

void loop() {
  while(count==0){
    Serial.print("N");
    delayMicroseconds(800);
    if(Serial.read()=='r'){
      count=Serial.read();
      count=count|(Serial.read()<<8);
      if(count>MAXFRAME){
        Serial.print("H");
        count=0;
      }
      byte tmp;
      for(i=0;i<count;i++){
        byte im8=i%8;
        if(im8==0)
          tmp=Serial.read();
        bits[i]=(tmp&(1<<im8)!=0)?0:1;
      }
    }
  }
  
  #ifdef LED
  if (led) {
    led=0;PORTB&=~(1<<(13-8));
  } else {
    led=1;PORTB|=1<<(13-8);
  }
  #endif
  
  noInterrupts();
  while(!checkLatch);
  writeData(1);
  while(checkLatch);
  for(i=0;i<count;i++){
    writeData(bits[i]);
    while(checkClock);  //wait for falling
    while(!checkClock); //wait for rising
  }
  writeData(1);
  interrupts();
  
  count=0;
}
