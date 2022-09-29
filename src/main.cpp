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


RTC_DS3231 rtc;      // Declration object rtc
Adafruit_AHTX0 aht;  // Declaratiob object aht

const int CHIP_SELECT = 4;  // chipselect pour fonctionner le mem shield 
const unsigned long DELAY_TIME = 5000, DELAY_TIME_ENVOI = 60000;  // Temps d'attente 

sensors_event_t Tmp, Hum;   // Declaration evenement 
String Seconde, Minute, Heure, Jour, Mois, Annee, DataString, DataEnvoi;  
unsigned long  TempsAvant = 0, TempsAvantEnvoi = 0; 

void setup() {

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
    wifiConnect(); 
    MQTTConnect(); 

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

  if (millis() - TempsAvant > DELAY_TIME )
  
  {   
      aht.getEvent(&Hum, &Tmp);// populate temp and humidity objects with fresh data

      DateTime now = rtc.now(); // Declaration object Now qui recois les donnees actuelle de la part de l'object rtc 
      // recupuration du temps actuel et Changement type de donnees en string

      Heure = String(now.hour(), DEC); 
      Minute = String(now.minute(),DEC); 
      Seconde = String(now.second(), DEC); 
      Annee  = String(now.year(), DEC); 
      Mois = String(now.month(), DEC);
      Jour = String(now.day(),DEC); 

      // Creation de la chaine de caractere 
      DataString = "{\"DateTime\": "  + Heure + "h" + Minute + "m" + Seconde + "s" + Annee + '-' + Mois + '-' + Jour + ",\"Tmp\": " + Tmp.temperature + ",\"Hum\": " + Hum.relative_humidity + "}F";
      

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
        }

      }
    // if the file isn't open, pop up an error:
       else {
        Serial.println("error opening datalog.txt");
      }

      dataFile.close(); 
      SD.remove("datalog.txt"); 
      Serial.println("\n prochain Lot .... "); 

    TempsAvantEnvoi = millis();
  }

}
