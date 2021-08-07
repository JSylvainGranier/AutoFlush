#include <TimeLib.h> 
#include <ClickEncoder.h>
#include <TimerOne.h>
#include"ServoTimer2.h"
#include <LedControl.h>

ClickEncoder *encoder;
volatile int16_t value;
int16_t last;

int DIN = 12;
int CS =  11;
int CLK = 10;

LedControl lc=LedControl(DIN,CLK,CS,0);


ServoTimer2 myservo;


const int trigPin = 5;
const int echoPin = 4;


#define LONG_DISTANCE_COLLECTION_SIZE_INITIAL 128
#define LONG_DISTANCE_COLLECTION_SIZE_ORIENTATION_MODE 2

int LONG_DISTANCE_COLLECTION_SIZE = LONG_DISTANCE_COLLECTION_SIZE_INITIAL; 
const int SHORT_DISTANCE_COLLECTION_SIZE = 10;

//Délais qui passe entre le moment où l'analyse déclare qu'il faut faire un autoflus, et le moment ou on le fait
const int AUTOFLUSH_DELAY_BTW_CONFIRM_AND_FLUSH = 2*60;

long longDistanceCollection[LONG_DISTANCE_COLLECTION_SIZE_INITIAL];
long shortDistanceCollection[SHORT_DISTANCE_COLLECTION_SIZE];
int nextShortDistanceIndex = 0 ;
int nextLongDistanceIndex = 0 ;

//Quand a débuté loop;
long thisLoopTs;
//Quand à débuté le précédant loop.
long lastLoopTs;

//Le TS où il est programmé un flush manuel
long manualFlushOn = 0;

//Le TS où il est programmé un flush automatique
long autoFlushOn = 0;

//Les précédants TS où il y a eu un autoFlush
const int autoFlushHistoryCount = 4;
long autoFlushHistory[autoFlushHistoryCount];

//Le nombre de secondes qu'il faut à un éléments de l'autoFlushHistory pour être périmé
const long AUTO_FLUSH_TIMEOUT = 12l*60l*60l;

//Niveau du prochain flush
int nextFlushLevel = 0;
int displayedValue = 0;

//Cumul du nombre de MS où l'excitation a été à son maximum
long maximumExcitationDuration = 0;
//Cumul du nombre de MS où l'excitation a été à son minimum
long minimumExcitationDuration = 0;

//Niveau actuel de l'excitation du capteur, varie de 0 pas excité, à 4, totalement afolé 
int analyseExcitationLevel = 0;


const int MODE_ANALYSE = 0;
const int MODE_MANUEL= 1;
int mode_actuel = MODE_ANALYSE;

int alternanceDamier = 0;

//Durée d'une alternance du damier en millisecondes
int const DUREE_ALTERNANCE_DAMIER = 1000;

//Délais en secondes avant de lancer une chasse d'eau manuelle
unsigned long const MANUAL_FLUSH_DELAY = 3;

//Délais en millisecondes sans activité après une chasse d'eau.
unsigned int const DELAY_INACTIVITY_AFTER_FLUSH = 10000;

//https://xantorohara.github.io/led-matrix-editor/

const uint64_t AUTO_FLUSH[] = {
  0x0000000810000000,
  0x0000001008000000,
  0x0000142814280000,
  0x0000281428140000,
  0x002a542a542a5400,
  0x00542a542a542a00,
  0x55aa55aa55aa55aa,
  0xaa55aa55aa55aa55
};

const uint64_t MANUAL_FLUSH[] = {
  0x0000000000000000,
  0x0000001818000000,
  0x00003c3c3c3c0000,
  0x007e7e7e7e7e7e00,
  0xffffffffffffffff
};

const uint64_t LEARNING[] = {
  0x1800183860663c00,
  0x2070a42424250e04,
  0x3c3c3c4281ff0000
};

const uint64_t AUTOFLUSH_LIMIT_EXCEED = 0x81c3663c3c66c381;

const int SNAKE_LEDS[12][2] = {
  {2,  2},
  {2,  3},
  {2,  4},
  {2,  5},
  {3,  5},
  {4,  5},
  {5,  5},
  {5,  4},
  {5,  3},
  {5,  2},
  {4,  2},
  {3,  2}
 
};

const int FLUSH_UP_POS = 1250;
const int FLUSH_DOWN_POS = 2000;

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

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  
  lastLoopTs = millis();
 
}

volatile boolean breakLongActions = false;
void timerIsr() {
  encoder->service();
  int newValue = encoder->getValue();
  if(newValue != value){
    value = newValue;
    breakLongActions = true;
  }
  
  
}

void resetAll() {
  //Enlever une chasse d'eau programmée
  manualFlushOn = 0;
  autoFlushOn = 0;
  nextFlushLevel = 0;
  maximumExcitationDuration = 0;
  minimumExcitationDuration = 0;
  analyseExcitationLevel = 0;
  lc.clearDisplay(0);

  for(int i = 0; i<LONG_DISTANCE_COLLECTION_SIZE; i++)
    longDistanceCollection[i] = 0;
   for(int i = 0; i<SHORT_DISTANCE_COLLECTION_SIZE; i++)
    shortDistanceCollection[i] > 0;

}

void displayImage(uint64_t image) {
  for (int i = 0; i < 8; i++) {
    byte row = (image >> i * 8) & 0xFF;
    
    for (int j = 0; j < 8; j++) {
      //Serial.println(bitRead(row, j));
      lc.setLed(0, i, j, bitRead(row, j));

      //delay(10);
      if(breakLongActions)
        return;
      
    }

  }
}

//Ajoute le TS dans la flush history
void appendToFlushHistory(long tsNowIFlush){
  for(int i = 0; i < autoFlushHistoryCount; i++){
    if(autoFlushHistory[i] == 0 || ((autoFlushHistory[i]+AUTO_FLUSH_TIMEOUT) < now()) ){
      autoFlushHistory[i] = tsNowIFlush;
      return;
    }
  }
}

void clearAutoFlushHistory(){
  for(int i = 0; i < autoFlushHistoryCount; i++){
    autoFlushHistory[i] = 0;
  }
}

//A partir des TS qui sont dans l'historique des autoFlush, retourne true s'il est possible de faire un autoFlush à tsTarget.
boolean getAutoflushHistoryClarence(long tsTarget){
  for(int i = 0; i < autoFlushHistoryCount; i++){
    if(autoFlushHistory[i] == 0 || (autoFlushHistory[i]+AUTO_FLUSH_TIMEOUT) < tsTarget){
      return true;
    }
  }
  return false;
}

void flush(int flushLevel){
  
      int startPoint = 4-flushLevel;
      int endPoint = 4+flushLevel;
    
      int pos = FLUSH_DOWN_POS;
      myservo.attach(9);
      myservo.write(FLUSH_DOWN_POS); 
      

      //int angleDelay = nextFlushLevel == 4 ? 20 : nextFlushLevel == 3 ? 40 : nextFlushLevel == 2 ? 60 : 60;
      int angleDelay = 60;
      
      while(pos > FLUSH_UP_POS){
        pos -= 50;
        myservo.write(pos); 
        delay(angleDelay);
      }

      int nbLedAllumees = flushLevel == 4 ? 64 : flushLevel == 3 ? 32 : flushLevel == 2 ? 16 : 4;
      int tempsUp = flushLevel*500;

      int ledDelay = tempsUp / nbLedAllumees;
       
      for(int row = startPoint; row < endPoint; row++)
        for(int col = startPoint; col < endPoint; col++){
          lc.setLed(0,row, col, false);
          delay(ledDelay);
        }

      angleDelay = flushLevel == 4 ? 100 : flushLevel == 3 ? 120 : flushLevel == 2 ? 150 : 200;;

      //delay(flushLevel == 4 ? 3000 : flushLevel == 3 ? 1000 : flushLevel == 2 ? 500 : 150);
          
      while(pos < FLUSH_DOWN_POS){
        pos += 50;
        myservo.write(pos); 
        delay(angleDelay);
      } 

      resetAll();
      
      //Petit delay physique pour me laisser le temps de partir...
      long delayInactivityPassed = 0;
      boolean loopToggle = true;
      
      while(delayInactivityPassed < DELAY_INACTIVITY_AFTER_FLUSH){
        
        for(int i = 0; i < 12 && delayInactivityPassed < DELAY_INACTIVITY_AFTER_FLUSH; i++){
          lc.setLed(0, SNAKE_LEDS[i][0], SNAKE_LEDS[i][1], loopToggle);
          delay(80);
        }
        delayInactivityPassed += 80*12;
        loopToggle = !loopToggle;
        
      }

      myservo.detach();  

      resetAll();
}

void procedeAnalyseDistance(){



  boolean hasHistoryClarence = getAutoflushHistoryClarence(now());
  
  if(!hasHistoryClarence){
    analyseExcitationLevel = 0;
    return;
  }
    
  //============== Apprendre l'environnement, ou si on le connait, simplement mesurer la distance avec l'obstacle 


  long duration;
  int distance;
  
  boolean learning = false;
  double moyenneCourte = 0;
  double moyenneLongue = 0;
  
  int nbValsInLongDistColl = 0;
  int nbValsInShortDistColl = 0;
  
  //Ecouter ce l'écho de la cuvette
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance= duration*0.034/2;

  //Serial.println(distance);

  if(breakLongActions)
    return;
  
  //Entretenir des moyennes longues et courtes...  
  shortDistanceCollection[nextShortDistanceIndex++] = distance;
  
  
    
  for(int i = 0; i<LONG_DISTANCE_COLLECTION_SIZE; i++){
    if(longDistanceCollection[i] > 0){
      moyenneLongue += longDistanceCollection[i];
      nbValsInLongDistColl++;
    } else {
      learning = true;
    }
  }
  if(nbValsInLongDistColl>0)
    moyenneLongue = moyenneLongue/nbValsInLongDistColl;
   
  for(int i = 0; i<SHORT_DISTANCE_COLLECTION_SIZE; i++){
    if(shortDistanceCollection[i] > 0){
      moyenneCourte += shortDistanceCollection[i];
      nbValsInShortDistColl++;
    }
      
  }
  moyenneCourte = moyenneCourte / nbValsInShortDistColl;
  
  //Pour déterminer un écart représentant le chat qui passe.
  long delta = abs(moyenneCourte - moyenneLongue);
  Serial.println(delta);
  boolean willUpdateMoyenneLongue = delta < 10 || learning || LONG_DISTANCE_COLLECTION_SIZE == LONG_DISTANCE_COLLECTION_SIZE_ORIENTATION_MODE;
  
  if(nextShortDistanceIndex > SHORT_DISTANCE_COLLECTION_SIZE-1){
    nextShortDistanceIndex = 0;
  
    if(willUpdateMoyenneLongue){
      longDistanceCollection[nextLongDistanceIndex++] = moyenneCourte;
      if(nextLongDistanceIndex > LONG_DISTANCE_COLLECTION_SIZE-1){
        nextLongDistanceIndex = 0;
      }
    }
    
    
  }
  
  if(breakLongActions)
    return;
  
  //Et instruire la valeur analyseExcitationLevel
  double moyLongueDouble = moyenneLongue;
  double deltaDouble = delta;
  double diviseur = 100;
  double deltaRepresentePrctMoyenneLongue = (deltaDouble/diviseur)*moyLongueDouble;
  if(deltaRepresentePrctMoyenneLongue > 4){
    analyseExcitationLevel = 3; 
  } else if(deltaRepresentePrctMoyenneLongue > 3){
    analyseExcitationLevel = 2; 
  } else if(deltaRepresentePrctMoyenneLongue > 2){
    analyseExcitationLevel = 1; 
  } else {
    analyseExcitationLevel = 0; 
  } 

  //===========  analyseExcitationLevel nous donne une idée simplifiée de l'activité devant le capteur. Déterminons si et quand faire quelque chose.

  if(breakLongActions)
    return;
  
  if(learning && (nbValsInLongDistColl * 3) < LONG_DISTANCE_COLLECTION_SIZE){
    analyseExcitationLevel = 0;
  }
    
  analyseExcitationLevel = constrain(analyseExcitationLevel, 0, 3);

  
  //Si la mesure est actuellement excitée, alors on repart de zéro pour attendre que l'obstacle parte. 
  if(analyseExcitationLevel > 0){
     minimumExcitationDuration = 0;
     autoFlushOn = 0;
  }

  if (autoFlushOn == 0){
    //Pas encore de chasse d'eau auto programmée.

    
    if(analyseExcitationLevel == 3){
      maximumExcitationDuration += thisLoopTs - lastLoopTs;
    } else if(analyseExcitationLevel == 0){
      minimumExcitationDuration += thisLoopTs - lastLoopTs; 
    }

    if( maximumExcitationDuration  > 8000 
      && minimumExcitationDuration > 15000  
      && getAutoflushHistoryClarence(now()+AUTOFLUSH_DELAY_BTW_CONFIRM_AND_FLUSH) 
      && LONG_DISTANCE_COLLECTION_SIZE == LONG_DISTANCE_COLLECTION_SIZE_INITIAL
     ){
     
      autoFlushOn = now()+AUTOFLUSH_DELAY_BTW_CONFIRM_AND_FLUSH;
      nextFlushLevel = 2;
                  
    }

    
    if(!hasHistoryClarence){
      displayImage(AUTOFLUSH_LIMIT_EXCEED);
    } else if(learning && (nbValsInLongDistColl * 3) < LONG_DISTANCE_COLLECTION_SIZE){
      int indexDamier = alternanceDamier > DUREE_ALTERNANCE_DAMIER ? 1 : 0;          
      displayImage(LEARNING[indexDamier]);
    } else {
      int indexDamier = (analyseExcitationLevel*2) + (alternanceDamier > DUREE_ALTERNANCE_DAMIER ? 1 : 0);
      displayImage(AUTO_FLUSH[indexDamier]);
    }
    
    
  } else {
    //Une chasse auto est fixée, on arrête de faire bouger le damier.
    int indexDamier = ((nextFlushLevel-1)*2);
    displayImage(AUTO_FLUSH[indexDamier]);
  }
}

void loop() {

  thisLoopTs = millis();
  breakLongActions = false;
  
  if(value == last){
    //Il n'e c'est rien passé. 

  
  } else if (value > last){
    //Vers la droite, programmation d'une chasse forcée.
    manualFlushOn = now()+MANUAL_FLUSH_DELAY;
    autoFlushOn = 0;
    
    nextFlushLevel++;
    if(nextFlushLevel > 4){
      nextFlushLevel = 4;
    }
  
  } else if (value < last) {
    //Vers la gauche, changement de mode, ou baisse de la chasse manuelle.
    
    if(manualFlushOn == 0){
      mode_actuel = mode_actuel == MODE_ANALYSE ? MODE_MANUEL : MODE_ANALYSE;
      resetAll();
      clearAutoFlushHistory();
    } else if(manualFlushOn > 0 && nextFlushLevel < 0) {
      resetAll();
    } else {
      nextFlushLevel--;
      autoFlushOn = 0;
      manualFlushOn = now()+MANUAL_FLUSH_DELAY;
      if(nextFlushLevel < 0){
        nextFlushLevel = 0;
      }
    }
  
  }

  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {

    if(LONG_DISTANCE_COLLECTION_SIZE == LONG_DISTANCE_COLLECTION_SIZE_INITIAL){
      LONG_DISTANCE_COLLECTION_SIZE = LONG_DISTANCE_COLLECTION_SIZE_ORIENTATION_MODE;
    } else {
      LONG_DISTANCE_COLLECTION_SIZE = LONG_DISTANCE_COLLECTION_SIZE_INITIAL;
    }
    
    mode_actuel = MODE_ANALYSE;
    resetAll();
    clearAutoFlushHistory();
    
  }  

  if(manualFlushOn > 0) {
       displayedValue = nextFlushLevel;
       displayImage(MANUAL_FLUSH[nextFlushLevel]);
  } else if(!breakLongActions && mode_actuel == MODE_ANALYSE){
    procedeAnalyseDistance();
  } 

  
  if (!breakLongActions && value != last) {
    last = value;
  }



  alternanceDamier += thisLoopTs - lastLoopTs;
  if(alternanceDamier > (2* DUREE_ALTERNANCE_DAMIER)){
    alternanceDamier = 0;
  }

  //C'est l'heure de faire une chasse d'eau !!!
  if((manualFlushOn != 0 && manualFlushOn<now()) || (autoFlushOn != 0 && autoFlushOn < now())){

    if(autoFlushOn != 0){
      appendToFlushHistory(now());
    }

    flush(nextFlushLevel);
    
  }

  lastLoopTs = thisLoopTs;
}
