#include <TimeLib.h> 
#include <ClickEncoder.h>
#include <TimerOne.h>

#include <LedControl.h>
#define NODEBUG
#define NODEBUGHISTORY

volatile int nextFlushLevel = 4;
int displayedValue = -1;

ClickEncoder *encoder;
volatile int16_t value;
int16_t last;

int DIN = 12;
int CS =  11;
int CLK = 10;


LedControl lc=LedControl(DIN,CLK,CS,0);

void setup() {

  encoder = new ClickEncoder(A1, A0, A2, 4);
  encoder->setAccelerationEnabled(true);

  Timer1.initialize(10000);
  Timer1.attachInterrupt(timerIsr); 
  
  last = -1;
 
  last = encoder->getValue();
  lc.shutdown(0,false);       //The MAX72XX is in power-saving mode on startup
  lc.setIntensity(0,0);      // Set the brightness to maximum value
  lc.clearDisplay(0);         // and clear the display



  Serial.begin(9600);
  Serial.println("Bonjour!");
 
}

volatile boolean breakeDisplay = false;
void timerIsr() {
  encoder->service();
  int newValue = encoder->getValue();
  if(newValue != value){
    value = newValue;
    breakeDisplay = true;
  }
  
  
}

const uint64_t MANUAL_FLUSH[] = {
  0x0000000000000000,
  0x0000001818000000,
  0x00003c3c3c3c0000,
  0x007e7e7e7e7e7e00,
  0xffffffffffffffff
};

void displayImage(uint64_t image) {
  for (int i = 0; i < 8; i++) {
    byte row = (image >> i * 8) & 0xFF;
    
    for (int j = 0; j < 8; j++) {
      //Serial.println(bitRead(row, j));
      lc.setLed(0, i, j, bitRead(row, j));

      //delay(10);
      if(breakeDisplay)
        return;
      
    }

  }
}

void loop() {
  
  if(false){
    
    
  } else {

     if(value == last){
      //Il n'e c'est rien passé. 

      
    } else if (value > last){
      //Vers la droite, programmation d'une chasse forcée.

      nextFlushLevel++;
      if(nextFlushLevel > 4){
        nextFlushLevel = 4;
      }
     
    } else if (value < last) {
      //Vers la gauche, changement de mode, ou baisse de la chasse manuelle.
      
        nextFlushLevel--;
       
        if(nextFlushLevel < 0){
          nextFlushLevel = 0;
        }
     
      
    }

  //Serial.println(nextFlushLevel);
    
         breakeDisplay = false;
         displayedValue = nextFlushLevel;
         displayImage(MANUAL_FLUSH[nextFlushLevel]);
        
      
    
     
    
  }
    
 
  

  if (!breakeDisplay && value != last) {
  
    last = value;

    
  }
  
}
