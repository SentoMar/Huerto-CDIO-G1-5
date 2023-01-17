#include <Adafruit_ADS1X15.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>//libreria del gps
#include <TinyGPS++.h>

const int sleepTimeS = 25; //(segundos)

Adafruit_ADS1115 ads1115;
int channelValue = 0;
int sensorValue = 0;
int humidityValue = 0;
const int dryValue = 330;
const int wetValue = 212;
#define offset -2.41
#define samplingInterval 20
#define printInterval 800
#define ArrayLength 40
int pHArray[ArrayLength];
int pHArrayIndex = 0;
int voltageTemp=4.096;
#define REST_SERVER_THINGSPEAK
//#define WiFi_CONNECTION_UPV
int luminityValue = 0;
TinyGPSPlus gps;
SoftwareSerial ss(13,12);

char dato=' ';

#ifdef WiFi_CONNECTION_UPV //Conexion UPV
  const char WiFiSSID[] = "GTI1";
  const char WiFiPSK[] = "1PV.arduino.Toledo";
#else //Conexion fuera de la UPV
  const char WiFiSSID[] = "JAVIER";
  const char WiFiPSK[] = "palmera24";
#endif

#define power_pin 5
//=====================================================
//=====================================================
//=====================================================

void regulador(double tempValue, double pHValue, double salinityValue, double humiditymap, double luminityValue) {
    Serial.println(" ");
    Serial.println("Se han verificado los valores de la mezcla y ambiente y se ha concluido lo siguiente: ");

    if(humiditymap > 40 || humiditymap < 80) {
      Serial.println("La humedad es ideal, ");
    }
    else {
      Serial.println("La humedad no tiene buenos numeros, ");
    }

    if(salinityValue > 5) {
      Serial.println("la mezcla esta muy salada, ");
    }
    else{
      Serial.println("la mezcla esta bien de sal, ");
    }

    if(pHValue > 6 || pHValue < 8) {
      Serial.println("el nivel de pH esta equilibrado, ");
    }
    else {
      Serial.println("el nivel de pH no es seguro, ");
    }

    if(tempValue > 15 || tempValue < 30) {
      Serial.println("la temperatura es ideal y ");
    }
    else {
      Serial.println("la temperatura no es ideal y ");
    }
    if(luminityValue < 300) {
      Serial.println("la luz es ideal");
    }
    Serial.println("");
}

//=====================================================
//=====================================================
//=====================================================

double tempValue(int channelValue){
  //captura una muestra del ADS1115
  int16_t adc1 = ads1115.readADC_SingleEnded(1);
  //Serial.println(adc1);
  //adc1 va a ser equivalente a lo que conocemos como vd en la formula
  
  //aplicamos la formula de la temperatura en
  //funcion de la lectura digital

  float vo =((adc1*6.128)/32727);

  float temperatura = ((vo - 0.79)/0.033);
  Serial.print("Temperatura: ");
  Serial.print(temperatura, 2);
  Serial.println("°C");
  delay(1000);
  return temperatura;
}

//=====================================================
//=====================================================
//=====================================================

double averageSample(int tam, int *array) {
  int sum=0;
  int i;
  for(i=0;i<=tam-1;i++){
    sum=sum+array[i];
  }
  double media = sum/tam;
  return media;
}

double phValue(int channelValue){
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue, voltage;
  if (millis() - samplingTime > samplingInterval) {
 
    pHArray[pHArrayIndex] = ads1115.readADC_SingleEnded(channelValue);
    
    if (pHArrayIndex == ArrayLength)pHArrayIndex = 0;{ 
      //convertir la lectura a tension
      voltage = (averageSample(ArrayLength, &pHArray[0]))/100;
      pHValue = 3.5 * voltage + offset;
      samplingTime=millis();
    }
   if(millis()-printTime > printInterval){ 
      //Serial.print("Voltage: ");
      //Serial.println(voltage, 2);
      Serial.print("pH value: ");
      Serial.println(pHValue, 2);
      printTime=millis();
      return pHValue;
    }    
  }
}

//=====================================================
//=====================================================
//=====================================================

int inter(double xi) {
  //aplicamos el teorema de lagrange para asi allar los vlores
  //equivalentes de las las lecturas digitales que recibe.
  //Se añaden ceros para evitar porblemas el las divisiones ya que float
  //pese a ser una variable que admite numero decimales no admite numeros tan pequños poniendolos a 0.
  float ec1 = (((76410000000000000) / 11774727272) * (pow(xi, 3)));
  float ec2 = ((91041000000000 / 70932092) * (pow(xi, 2)));
  float ec3 = ((6191832190000000 / 692631016) * xi);
  float ec4 = ((9416520435 / 44265892));
  int ec = ((ec1 / 10000000000000) - (ec2 / 1000000000) + (ec3 / 10000000) - (ec4));
  if (ec < 1) {
    return 0;
  } 
  else {
    return ec;
  }
}

double salinityValue(int channelValue) {
  int sol = 0;
  int16_t adc0;

  digitalWrite(power_pin, HIGH);
  delay(100);
  //Realizamos la lectura del sensor.
  adc0 = analogRead(A0);
  digitalWrite(power_pin, LOW);

  sol = inter(adc0);//llamada a la funcion de interpolacion
 
  Serial.print("Salinidad: ");
  Serial.print(sol);
  Serial.println("g");
  return sol;
}

//=====================================================
//=====================================================
//=====================================================

double humiditymap(int channelValue) {
  int16_t adc0 = ads1115.readADC_SingleEnded(2);//leemos el vlor de adc
  humidityValue = map(adc0, 10535, 3500, 0, 100);//se mapea el valor digital
  //Gracias al mapeado optenemos un valor en porcentaje
  //Serial.print(adc0);
  Serial.print("Humedad: ");
  if(humidityValue>=100){
  Serial.println("100%");
  }
  if(humidityValue<0) {
    Serial.println("0%");
  }
  else{
  Serial.print(humidityValue, DEC);
  Serial.println("%");
  }
  delay(1000);
  return humidityValue;
}

//=====================================================
//=====================================================
//=====================================================

double luminity(int channelValue) {
  int16_t adc3 = ads1115.readADC_SingleEnded(3);
  double vd = (4.096/32767)*adc3;
  double vd2 = ((adc3*4.096)*10/32727);

 if(adc3 >= 3200) {
    adc3 == 3200;
 }
  if(adc3 < 60){
    Serial.println("Sombra");
  }
  if(adc3 > 60 && adc3 < 200){
    Serial.println("Luz ambiente");
  }
  if(adc3 > 200 && adc3 < 1000) {
    Serial.println("Luz fuerte");
  }
  if(adc3 > 1000 && adc3 < 3199) {
    Serial.println("Luz muy fuerte");
  }
  else if(adc3 == 3200){
    Serial.println("Sensor Saturado");
  }
  //Serial.print("Valor digital: ");
  //Serial.println(adc3);
  //Serial.print("Tension analogica rara: ");
  //Serial.println(vd, 2);
  //Serial.print("Tension analogica de temp: ");
  //Serial.println(vd2, 2);
  delay(1000);

  return vd;
}
//=====================================================
//=====================================================
//=====================================================


///////////////////////////////////////////////////////
/////////////// SERVER Definitions /////////////////////
//////////////////////////////////////////////////////

#if defined(WiFi_CONNECTION_UPV) //Conexion UPV
  const char Server_Host[] = "proxy.upv.es";
  const int Server_HttpPort = 8080;
#elif defined(REST_SERVER_THINGSPEAK) //Conexion fuera de la UPV
  const char Server_Host[] = "api.thingspeak.com";
  const int Server_HttpPort = 80;
#else
  const char Server_Host[] = "dweet.io";
  const int Server_HttpPort = 80;
#endif

WiFiClient client;

///////////////////////////////////////////////////////
/////////////// HTTP REST Connection ////////////////
//////////////////////////////////////////////////////

#ifdef REST_SERVER_THINGSPEAK 
  const char Rest_Host[] = "api.thingspeak.com";
  String MyWriteAPIKey="KQUNPE8GXE8VVHJB"; // Escribe la clave de tu canal ThingSpeak
#else 
  const char Rest_Host[] = "dweet.io";
  String MyWriteAPIKey="PruebaGTI"; // Escribe la clave de tu canal Dweet
#endif

#define NUM_FIELDS_TO_SEND 7 //Numero de medidas a enviar al servidor REST (Entre 1 y 8)

/////////////////////////////////////////////////////
/////////////// Pin Definitions ////////////////
//////////////////////////////////////////////////////

const int LED_PIN = 5; // Thing's onboard, green LED


/////////////////////////////////////////////////////
/////////////// WiFi Connection ////////////////
//////////////////////////////////////////////////////

void connectWiFi()
{
  byte ledStatus = LOW;

  #ifdef PRINT_DEBUG_MESSAGES
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
  #endif
  
  WiFi.begin(WiFiSSID, WiFiPSK);

  while (WiFi.status() != WL_CONNECTED)
  {
    // Blink the LED
    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;
    #ifdef PRINT_DEBUG_MESSAGES
       Serial.println(".");
    #endif
    delay(500);
  }
  #ifdef PRINT_DEBUG_MESSAGES
     Serial.println( "WiFi Connected" );
     Serial.println(WiFi.localIP()); // Print the IP address
  #endif
}

/////////////////////////////////////////////////////
/////////////// HTTP POST  ThingSpeak////////////////
//////////////////////////////////////////////////////

void HTTPPost(String fieldData[], int numFields){

// Esta funcion construye el string de datos a enviar a ThingSpeak mediante el metodo HTTP POST
// La funcion envia "numFields" datos, del array fieldData.
// Asegurate de ajustar numFields al número adecuado de datos que necesitas enviar y activa los campos en tu canal web
  
    if (client.connect( Server_Host , Server_HttpPort )){
       
        // Construimos el string de datos. Si tienes multiples campos asegurate de no pasarte de 1440 caracteres
   
        String PostData= "api_key=" + MyWriteAPIKey ;
        for ( int field = 1; field < (numFields + 1); field++ ){
            PostData += "&field" + String( field ) + "=" + fieldData[ field ];
        }     
        
        // POST data via HTTP
        #ifdef PRINT_DEBUG_MESSAGES
            Serial.println( "Connecting to ThingSpeak for update..." );
        #endif
        client.println( "POST http://" + String(Rest_Host) + "/update HTTP/1.1" );
        client.println( "Host: " + String(Rest_Host) );
        client.println( "Connection: close" );
        client.println( "Content-Type: application/x-www-form-urlencoded" );
        client.println( "Content-Length: " + String( PostData.length() ) );
        client.println();
        client.println( PostData );
        #ifdef PRINT_DEBUG_MESSAGES
            Serial.println( PostData );
            Serial.println();
            //Para ver la respuesta del servidor
            #ifdef PRINT_HTTP_RESPONSE
              delay(500);
              Serial.println();
              while(client.available()){String line = client.readStringUntil('\r');Serial.print(line); }
              Serial.println();
              Serial.println();
            #endif
        #endif
    }
}

////////////////////////////////////////////////////
/////////////// HTTP GET  ////////////////
//////////////////////////////////////////////////////

void HTTPGet(String fieldData[], int numFields){
  
// Esta funcion construye el string de datos a enviar a ThingSpeak o Dweet mediante el metodo HTTP GET
// La funcion envia "numFields" datos, del array fieldData.
// Asegurate de ajustar "numFields" al número adecuado de datos que necesitas enviar y activa los campos en tu canal web
  
    if (client.connect( Server_Host , Server_HttpPort )){
           #ifdef REST_SERVER_THINGSPEAK 
              String PostData= "GET https://api.thingspeak.com/update?api_key=";
              PostData= PostData + MyWriteAPIKey ;
           #else 
              String PostData= "GET http://dweet.io/dweet/for/";
              PostData= PostData + MyWriteAPIKey +"?" ;
           #endif
           
           for ( int field = 1; field < (numFields + 1); field++ ){
              PostData += "&field" + String( field ) + "=" + fieldData[ field ];
           }
          
           
           #ifdef PRINT_DEBUG_MESSAGES
              Serial.println( "Connecting to Server for update..." );
           #endif
           client.print(PostData);         
           client.println(" HTTP/1.1");
           client.println("Host: " + String(Rest_Host)); 
           client.println("Connection: close");
           client.println();
           #ifdef PRINT_DEBUG_MESSAGES
              Serial.println( PostData );
              Serial.println();
              //Para ver la respuesta del servidor
              #ifdef PRINT_HTTP_RESPONSE
                delay(500);
                Serial.println();
                while(client.available()){String line = client.readStringUntil('\r');Serial.print(line); }
                Serial.println();
                Serial.println();
              #endif
           #endif  
    }
}

//=====================================================
//=====================================================
//=====================================================

void setup() {
  Serial.begin(115200);//ponemos la velocidad en baudios a la que vamos a trabajar
  ads1115.begin(0x48);
  ads1115.setGain(GAIN_TWOTHIRDS);
  ss.begin(9600);
  //Utilizamos esta ganancia ya que el voltaje usado es de 5V 
  //siendo este el mas cecano pues se relaciona con 6,124V  
  pinMode(power_pin, OUTPUT);
  
  connectWiFi();
  digitalWrite(LED_PIN, HIGH);
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

//=====================================================
//=====================================================
//=====================================================

void loop() {

  String Data[ NUM_FIELDS_TO_SEND + 1]; // Podemos enviar hasta 8 datos

  Data[1] = String(humiditymap(0));

  Data[2] = String(salinityValue(0));

  Data[3] = String(phValue(0));

  Data[4] = String(tempValue(0));

  Data[5] = String(luminity(0));
  while(ss.available()>0){
    gps.encode(ss.read());
    if(gps.location.isUpdated()){
      Data[6] = String(gps.location.lat(), 6);
      Data[7] = String(gps.location.lng(), 6);
    }
  }

  HTTPGet( Data, NUM_FIELDS_TO_SEND );

  double tempp = tempValue(0);
  double phh = phValue(0);
  double salinityy = salinityValue(0);
  double humid = humiditymap(0);
  double luminityy = luminity(0);
  
  regulador(tempp,phh,salinityy,humid,luminityy);

  delay(5000);
  //deepSleep(1000000*sleepTimeS);
}
