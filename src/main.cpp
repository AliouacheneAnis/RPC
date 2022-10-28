/*
  Titre      : Capture des données sur une carte SD
  Auteur     : Anis Aliouachene
  Date       : 27/09/2022
  Description: 
              - Capture du TimeStamp avec le RTC (Date,Heure, Minutes, Seconde), ainsi que la Température et humidité avec le AHT20
              - Ces données doivent être capturé a toutes les 5 secondes et conservé sur la carte SD
              - Transmission de ces données a toutes les minutes sur Thingsboard
              - Une fois les données transmise, effacé les données de la carte SD
              - Vérifié le résultat dans les données de télémétries
  Version    : 0.0.2
*/


// Include des libraires
#include<Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h" 
#include <Adafruit_AHTX0.h>
#include<WIFIConnector_MKR1000.h>
#include<MQTTConnector.h>
#include<Servo.h>


RTC_DS3231 rtc;      // Declration object rtc
Adafruit_AHTX0 aht;  // Declaratiob object aht
Servo myservo;

const int CHIP_SELECT = 4, SERVO = 1;  // chipselect pour fonctionner le mem shield 
const unsigned long DELAY_TIME = 5000, DELAY_TIME_ENVOI = 30000, DELAY_RPC = 2000;  // Temps d'attente 
const int LED_PIN_ROUGE = 2, LED_PIN_BLUE = 3, IntensiteAllumer = 255, IntensiteEteint = 0;  

sensors_event_t Tmp, Hum;   // Declaration evenement 
String Seconde, Minute, Heure, Jour, Mois, Annee, DataString, DataEnvoi,Data;

long AnalogValue; 
unsigned long  TempsAvant = 0, TempsAvantEnvoi = 0,TempsRPC = 0; 
int CurrentVal, Position = 90;  

void setup() {
  
  myservo.attach(SERVO);
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  pinMode(LED_PIN_BLUE,OUTPUT);
  pinMode(LED_PIN_ROUGE,OUTPUT); 
  pinMode(LED_BUILTIN,OUTPUT); 

  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(CHIP_SELECT)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");

  // communication avec rtc 
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  
  // communication avec aht 
  if (aht.begin()) {
    Serial.println("Found AHT20");
    } else {
    Serial.println("Didn't find AHT20");
  }   
}

void loop() {
  
  digitalWrite(LED_BUILTIN, LOW);

  if (millis() - TempsAvant > DELAY_TIME )
  
  {   
      aht.getEvent(&Hum, &Tmp);// populate temp and humidity objects with fresh data

      DateTime now = rtc.now(); // Declaration object Now qui recois les donnees actuelle de la part de l'object rtc 
      
      // Creation de la chaine de caractere 
      DataString = "{\"ts\": " + String(now.unixtime()+ 14400 ) + "000, \"values\":{\"tmp\": " + String(Tmp.temperature) + ", \"Hum\": " + String(Hum.relative_humidity) + "}}F";

      // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      File dataFile = SD.open("datalog.txt", FILE_WRITE);

      // if the file is available, write to it:
      if (dataFile) {
        dataFile.println(DataString); 
        Serial.println(DataString);
        dataFile.close();
      // print to the serial port too:
      }
        // if the file isn't open, pop up an error:
        else {
          Serial.println("error opening datalog.txt");
        }

      TempsAvant = millis();
  }

  if ( millis() - TempsAvantEnvoi > DELAY_TIME_ENVOI )
  { 
    status = WL_IDLE_STATUS; 
    digitalWrite(LED_BUILTIN, HIGH);
    wifiConnect(); 
    MQTTConnect();

    unsigned int i = 0; 
    File dataFile = SD.open("datalog.txt", FILE_READ);
        // if the file is available, write to it:
    if (dataFile) {
        
        while (i < dataFile.size()-1) // Boucle pour lire le fichier jusqu'a la fin  selon i 
        {
                
            dataFile.seek(i);  // aller vers la position i 
            Payload = dataFile.readStringUntil('F'); // lire le fichier jusqu'a qu'on trouve le caractere F qui indique la fin de la chaine MQTT et affecter la chaine a Payload 
            i = dataFile.position() + 1; // sauter la ligne pour lire la prochaine chaine MQTT 
            sendPayload(); // Envoi de la chaine MQTT 
            delay(500); 
        }

      }
    // if the file isn't open, pop up an error:
       else {
        Serial.println("error opening datalog.txt");
      } 
      
      dataFile.close(); 
      SD.remove("datalog.txt");
      ClientMQTT.loop();
      Serial.println(DataRecu);
      Serial.println("Datarecu");
          if (DataRecu != Data)
          {
            if (DataRecu == "Clim")
            {
                analogWrite(LED_PIN_ROUGE, IntensiteAllumer);
                Serial.println("LED Rouge Allumer"); 
                Position = 180;   
                myservo.write(Position); // Servo moteur vers la position 180

            }else if (DataRecu == "Chauffage")
            {
                analogWrite(LED_PIN_BLUE, IntensiteAllumer);
                Serial.println("LED Blue Allmer"); 
                Position = 0;   
                myservo.write(Position); // Servo moteur vers la position 0

            }else{ 
                  analogWrite(LED_PIN_ROUGE, IntensiteEteint);
                  analogWrite(LED_PIN_BLUE, IntensiteEteint);
                  Serial.println("pasDatarecu");
                  Position = 90;   
                  myservo.write(Position); // Servo moteur vers la position 0
            }
            
            Data = DataRecu;
        }
      
      Serial.println("\n prochain Lot .... "); 
      ClientMQTT.disconnect();
      WiFi.disconnect();
      WiFi.end();
      digitalWrite(LED_BUILTIN, LOW);
      TempsAvantEnvoi = millis();
      }


}
