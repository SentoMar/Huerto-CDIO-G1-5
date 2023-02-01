#include "huerto.h"
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h> //libreria del gps
#include <TinyGPS++.h>

const int sleepTimeS = 25; //(segundos)
TinyGPSPlus gps;
SoftwareSerial ss(13, 12);
huerto huerto(5);

void setup() {
  ss.begin(9600);
  huerto.connectWiFi();
  #ifdef PRINT_DEBUG_MESSAGES
      Serial.print("Server_Host: ");
      Serial.println(Server_Host);
      Serial.print("Port: ");
      Serial.println(String( Server_HttpPort ));
      Serial.print("Server_Rest: ");
      Serial.println(Rest_Host);
  #endif 
 //Serial.println("Inicializando el GPS...");
 //delay(27000);
 //Serial.println("Esperando datos");

}
#define NUM_FIELDS_TO_SEND 7 // Numero de medidas a enviar al servidor REST (Entre 1 y 8)
void loop() {

  String Data[ NUM_FIELDS_TO_SEND + 1]; // Podemos enviar hasta 8 datos

  Data[1] = String(huerto.humiditymap(0));

  Data[2] = String(huerto.salinityValue(0));

  Data[3] = String(huerto.phValue(0));

  Data[4] = String(huerto.tempValue(0));

  Data[5] = String(huerto.luminity(0));
  while(ss.available()>0){
    gps.encode(ss.read());
    if(gps.location.isUpdated()){
      Data[6] = String(gps.location.lat(), 6);
      Data[7] = String(gps.location.lng(), 6);
    }
  }

  huerto.HTTPGet( Data, NUM_FIELDS_TO_SEND );

  double tempp = huerto.tempValue(0);
  double phh = huerto.phValue(0);
  double salinityy = huerto.salinityValue(0);
  double humid = huerto.humiditymap(0);
  double luminityy = huerto.luminity(0);
  
  huerto.regulador(tempp,phh,salinityy,humid,luminityy);

  delay(5000);
  //deepSleep(1000000*sleepTimeS);
}
