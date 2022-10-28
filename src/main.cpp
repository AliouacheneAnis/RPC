/*
  Titre      : Systeme de climatisation 
  Auteur     : Anis Aliouachene
  Date       : 28/10/2022
  Description: 
              - Stockage de la dateheure, température et humidité sur la carte SD a tout les 5 secondes provenant du AHT20
              - Transfert de ces données a tout les 30 secondes en utilisant le format nécessaire pour que Thingsboard utiliser la dateheure de la capture des données
              - Notification de température élevé ou faible avec Rule Chain (a reproduire sur le serveur de production)
              - RPC (LED bleu pour température faible, LED Rouge pour température élevé)  avec Rule Chain
              - Remplacé la LED Bleu et Rouge par un servo moteur
                      Rotation vers la gauche (Chauffage – LED Bleu)
                      Rotation vers la droite (Climatisation – LED Rouge)
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
Servo myservo;       // Declaration Object myservo 

const int CHIP_SELECT = 4, SERVO = 1;  // chipselect pour fonctionner le mem shield 
const unsigned long DELAY_TIME = 5000, DELAY_TIME_ENVOI = 30000, DELAY_RPC = 2000;  // Temps d'attente 

sensors_event_t Tmp, Hum;   // Declaration evenement 
String DataString, DataEnvoi,Data; // String pour les chaine MQTT et RPC 

// Variables
long AnalogValue; 
unsigned long  TempsAvant = 0, TempsAvantEnvoi = 0,TempsRPC = 0; 
int CurrentVal, Position = 90;  

void setup() {
  
  myservo.attach(SERVO); // attach servo au PIN 1

  // Open serial communications and wait for port to open:
  Serial.begin(9600);

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
      aht.getEvent(&Hum, &Tmp); // populate temp and humidity objects with fresh data

      DateTime now = rtc.now(); // Declaration object Now qui recois les donnees actuelle de la part de l'object rtc 
      
      // Creation de la chaine de caractere 
      // j'ai ajouter le nombre 18480 pour ajuster le temps au temps reel ici et trois 0 pour transfere du seconde au milisecondes 
      DataString = "{\"ts\": " + String(now.unixtime()+ 18480 ) + "000, \"values\":{\"tmp\": " + String(Tmp.temperature) + ", \"Hum\": " + String(Hum.relative_humidity) + "}}F";

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
    // Connexion au wifi pour l'envoi 
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
      
      // Fermeture de fichier apres l'envoi vers thingsboard et reception message RPC 
      dataFile.close(); 
      SD.remove("datalog.txt");
      ClientMQTT.loop();
      Serial.println(DataRecu);
      Serial.println("Datarecu");
          if (DataRecu != Data)
          {
            if (DataRecu == "Clim")
            {
                Position = 180;   
                myservo.write(Position); // Servo moteur vers la position 180

            }else if (DataRecu == "Chauffage")
            {
                Position = 0;   
                myservo.write(Position); // Servo moteur vers la position 0

            }else{ 

                  Position = 90;   
                  myservo.write(Position); // Servo moteur vers la position 0
            }
            
            Data = DataRecu;
        }
      
      // deconnexion du wifi et MQTT 
      Serial.println("\n prochain Lot .... "); 
      ClientMQTT.disconnect();
      WiFi.disconnect();
      WiFi.end();
      digitalWrite(LED_BUILTIN, LOW);
      TempsAvantEnvoi = millis();
      }


}
