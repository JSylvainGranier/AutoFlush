#include <TimeLib.h> 



#include <ClickEncoder.h>
#include <TimeLib.h>

#include <LedControl.h>
#include"ServoTimer2.h"
#define NODEBUG

#include <TimerOne.h>

const int trigPin = 5;
const int echoPin = 4;

const int LONG_DISTANCE_COLLECTION_SIZE = 128;
const int SHORT_DISTANCE_COLLECTION_SIZE = 10;

long longDistanceCollection[LONG_DISTANCE_COLLECTION_SIZE];
long shortDistanceCollection[SHORT_DISTANCE_COLLECTION_SIZE];
int nextShortDistanceIndex = 0 ;
int nextLongDistanceIndex = 0 ;

long duration;
int distance;


ServoTimer2 myservo;

ClickEncoder *encoder;
int16_t last, value;

int DIN = 12;
int CS =  11;
int CLK = 10;

//Quand a débuté loop;
long thisLoopTs;
//Quand à débuté le précédant loop.
long lastLoopTs;

//Le TS où il est programmé un flush manuel
long manualFlushOn = 0;

//Le TS où il est programmé un flush automatique
long autoFlushOn = 0;

//Niveau du prochain flush
int nextFlushLevel = 0;

//Cumul du nombre de MS où l'excitation a été à son maximum
long maximumExcitationDuration = 0;
//Cumul du nombre de MS où l'excitation a été à son minimum
long minimumExcitationDuration = 0;

//Niveau actuel de l'excitation du capteur, varie de 0 pas excité, à 4, totalement afolé 
int analyseExcitationLevel = 0;

//Caractère reçu par le moniteur console USB
char receivedChar = '0';

LedControl lc=LedControl(DIN,CLK,CS,0);

//Combien de minute on ajoute ou enlève lorsque l'on règle l'heure, et qu'on fait un pas du rotary encoder.
const int PAS_REGLAGE_HEURE = 10;

const uint64_t CLOCK = 0x3c4281b98989423c;

const int MODE_CONFIGURATION = -1;
const int MODE_ANALYSE = 0;
const int MODE_MANUEL= 1;
int mode_actuel = MODE_CONFIGURATION;

void setup() {

  encoder = new ClickEncoder(A1, A0, A2, 4);
  encoder->setAccelerationEnabled(false);

  Timer1.initialize(1000);
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

  #ifdef DEBUG
        mode_actuel = MODE_ANALYSE;
        resetAll();
        Serial.println("Auto config");
      #else
        displayImage(CLOCK);
        delay(3000);
        lc.clearDisplay(0);
      #endif

  
}

void timerIsr() {
  encoder->service();
}



void dessinerHeure(){
  int heuresMontrees = 0;
  for(int row = 0; row < 3; row ++){
    for(int i = 0; i < 8; i++){
      lc.setLed(0, row, i, heuresMontrees < hour() );
      heuresMontrees++;
    }    
  }

  int dizaineMinuteMontrees = 0;

  for(int i = 0; i < 8; i++){
      lc.setLed(0, 7, i, dizaineMinuteMontrees < minute() );
      dizaineMinuteMontrees += 10;
  }  


}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}






int alternanceDamier = 0;

//Durée d'une alternance du damier en millisecondes
int const DUREE_ALTERNANCE_DAMIER = 1000;

//Délais en secondes avant de lancer une chasse d'eau manuelle
unsigned long const MANUAL_FLUSH_DELAY = 3;

//Délais en millisecondes sans activité après une chasse d'eau.
unsigned int const DELAY_INACTIVITY_AFTER_FLUSH = 15000;



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



void loop() {


    thisLoopTs = millis();

    value += encoder->getValue();
  
  
  
    ClickEncoder::Button b = encoder->getButton();
    if (b != ClickEncoder::Open) {
    Serial.print("Button: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (b) {
      case ClickEncoder::Pressed:
          Serial.println("ClickEncoder::Pressed");
          //Ne marche jamais
      break;
      case ClickEncoder::Held:
          Serial.println("ClickEncoder::Held");
      break;
      case ClickEncoder::Released:
          Serial.println("ClickEncoder::Released");
      break;
      case ClickEncoder::Clicked:
          Serial.println("ClickEncoder::Clicked");
          lc.clearDisplay(0);
          if(mode_actuel == MODE_CONFIGURATION){
            mode_actuel = MODE_ANALYSE;
            resetAll();
          } else {
            dessinerHeure();
            mode_actuel = MODE_CONFIGURATION;
          }
      break;
      case ClickEncoder::DoubleClicked:
          Serial.println("ClickEncoder::DoubleClicked");
          
        break;
    }
  } 

 
  
  if(mode_actuel == MODE_CONFIGURATION ){
    if(value != last){
      dessinerHeure();
      lireConfigHeure();
    }
    
  } else {

    boolean learning = false;
    double moyenneCourte = 0;
    double moyenneLongue = 0;
    
    int nbValsInLongDistColl = 0;
    int nbValsInShortDistColl = 0;

    if(mode_actuel == MODE_ANALYSE ){

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
      boolean willUpdateMoyenneLongue = delta < 10 || learning;
    
      if(nextShortDistanceIndex > SHORT_DISTANCE_COLLECTION_SIZE-1){
        nextShortDistanceIndex = 0;
    
        if(willUpdateMoyenneLongue){
          longDistanceCollection[nextLongDistanceIndex++] = moyenneCourte;
          if(nextLongDistanceIndex > LONG_DISTANCE_COLLECTION_SIZE-1){
            nextLongDistanceIndex = 0;
          }
        }
        
        
      }
    
     
  
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

   
      if(learning && (nbValsInLongDistColl * 3) < LONG_DISTANCE_COLLECTION_SIZE){
        analyseExcitationLevel = 0;
      }
       #ifdef DEBUG
        Serial.print("Moyenne courte : ");
        Serial.print(moyenneCourte);
        Serial.print(" Moyenne longue : ");
        Serial.print(moyenneLongue);
        Serial.print(" (val actuelle : ");
        Serial.print(distance);
        Serial.print(" , learning : ");
        Serial.print(learning);
        Serial.print("@");
        Serial.print(nextLongDistanceIndex);
        Serial.print(") Delta : ");
        Serial.print(delta);

        Serial.print(" Excitation : (");
        Serial.print(deltaDouble);
        Serial.print(" / ");
        Serial.print(diviseur);
        Serial.print(" )* ");
        Serial.print(moyLongueDouble);
        Serial.print(" = ");
        Serial.print(deltaRepresentePrctMoyenneLongue);
        Serial.print(", soit ");
        Serial.print(analyseExcitationLevel);
        Serial.println();
      #endif

    }

    if(value == last){
      //Il n'e c'est rien passé. 
      
      if(mode_actuel == MODE_ANALYSE && manualFlushOn == 0) {
        
        analyseExcitationLevel = constrain(analyseExcitationLevel, 0, 3);

        

        if(analyseExcitationLevel > 0){
           minimumExcitationDuration = 0;
           autoFlushOn = 0;
        }

        if (autoFlushOn == 0){
          if(analyseExcitationLevel == 3){
            maximumExcitationDuration += thisLoopTs - lastLoopTs;
          } else if(analyseExcitationLevel == 0){
            minimumExcitationDuration += thisLoopTs - lastLoopTs; 
          }
  
          if( maximumExcitationDuration  > 8000 && minimumExcitationDuration > 15000 ){
            nextFlushLevel = 3;
            
            time_t initialTimeToRecover = now();

            #ifdef DEBUG
              Serial.println("Il est actuellement ");
              digitalClockDisplay();
            #endif
  
            do {
              setTime(now()+(1*60));
            } while(hour()>22 || hour() < 6);

            #ifdef DEBUG
              Serial.println("Et je vais faire un flush a : ");
              digitalClockDisplay();
            #endif
            autoFlushOn = now();
            nextFlushLevel = 3;

            setTime(initialTimeToRecover);
            #ifdef DEBUG
              Serial.println("Retour à l'heure de départ : ");
              digitalClockDisplay();
              
            #endif
            
            
          }

          
          if(learning && (nbValsInLongDistColl * 3) < LONG_DISTANCE_COLLECTION_SIZE){
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
        
        
      } else {
        displayImage(MANUAL_FLUSH[nextFlushLevel]);
      }
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
    
    alternanceDamier += thisLoopTs - lastLoopTs;
    if(alternanceDamier > (2* DUREE_ALTERNANCE_DAMIER)){
      alternanceDamier = 0;
    }

    #ifdef DEBUGPLUS
        Serial.print(hour());
        Serial.print(":");
        Serial.print(minute());
        Serial.print(":");
        Serial.println(second());
       #endif

   
    if((manualFlushOn != 0 && manualFlushOn<now()) || (autoFlushOn != 0 && autoFlushOn < now())){
      int startPoint = 4-nextFlushLevel;
      int endPoint = 4+nextFlushLevel;
    
      int pos = FLUSH_DOWN_POS;
      myservo.attach(9);
      myservo.write(FLUSH_DOWN_POS); 
      while(pos > FLUSH_UP_POS){
        pos -= 50;
        myservo.write(pos); 
        delay(20);
      }
       
      for(int row = startPoint; row < endPoint; row++)
        for(int col = startPoint; col < endPoint; col++){
          lc.setLed(0,row, col, false);
          delay(80);
        }
      
    
      while(pos < FLUSH_DOWN_POS){
        pos += 50;
        myservo.write(pos); 
        delay(35);
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
    } else if(manualFlushOn != 0 && nextFlushLevel == 0){
      resetAll();
      
    }
    
  }
    
 
  

  if (value != last) {
  
    last = value;

    
  }
  
  
  lastLoopTs = thisLoopTs;
}



void displayImage(uint64_t image) {
  for (int i = 0; i < 8; i++) {
    byte row = (image >> i * 8) & 0xFF;
    
    for (int j = 0; j < 8; j++) {
      //Serial.println(bitRead(row, j));
      lc.setLed(0, i, j, bitRead(row, j));
    }
  }
}

void lireConfigHeure(){
    time_t t = now();
    if(value > last){
      t += PAS_REGLAGE_HEURE*60;
    } else if(value < last) {
      t -= PAS_REGLAGE_HEURE*60;
    }
    //digitalClockDisplay();
    setTime(t);
    dessinerHeure();
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
