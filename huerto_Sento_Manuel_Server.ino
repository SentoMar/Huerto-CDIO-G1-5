#include <Adafruit_ADS1X15.h>
#include <ESP8266WiFi.h>

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
#define WiFi_CONNECTION_UPV

#ifdef WiFi_CONNECTION_UPV //Conexion UPV
  const char WiFiSSID[] = "GTI1";
  const char WiFiPSK[] = "1PV.arduino.Toledo";
#else //Conexion fuera de la UPV
  const char WiFiSSID[] = "...";
  const char WiFiPSK[] = "...";
#endif

#define power_pin 5

//=====================================================
//=====================================================
//=====================================================

double tempValue(int channelValue){
  //captura una muestra del ADS1115
  int16_t adc0 = ads1115.readADC_SingleEnded(2);
  //adc0 va a ser equivalente a lo que conocemos como vd en la formula
  
  //aplicamos la formula de la temperatura en
  //funcion de la lectura digital

  float vo =((adc0*4.096)/32727);

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
      Serial.print("Voltage: ");
      Serial.println(voltage, 2);
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
  int16_t adc0 = ads1115.readADC_SingleEnded(channelValue);//leemos el vlor de adc
  humidityValue = map(adc0, 20150, 12000, 0, 100);//se mapea el valor digital
  //Gracias al mapeado optenemos un valor en porcentaje
  Serial.print("Humedad: ");
  if(humidityValue>=100){
  Serial.println("100%");
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

#define NUM_FIELDS_TO_SEND 4 //Numero de medidas a enviar al servidor REST (Entre 1 y 8)

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

  HTTPGet( Data, NUM_FIELDS_TO_SEND );

  //se llama a cada funcion por serparado
}
