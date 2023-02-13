#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Stepper.h>
#include <WiFiUdp.h>
#include <Stepper_28BYJ48.h>

#define INPUT_SIZE 32

WiFiUDP Udp;  // Creation of wifi Udp instance

char packetBuffer[255];

char inputBuf[INPUT_SIZE + 1];

unsigned int localPort = 9999;

IPAddress ipServidor(192, 168, 4, 1);
IPAddress ipCliente(192, 168, 4, 10);   // Different IP than server
IPAddress Subnet(255, 255, 255, 0);

// параметры получамые через serial порт
bool stepperStart = false;
int stepperPosition = 0;
int stepperSpeed = 0;

// Stepper Motor Settings
const int stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution
const int STEP = 1;
const int DIR = 2;
const int ENA = 3;
const int LED_PIN = 15;
Stepper_28BYJ myStepper(stepsPerRevolution, STEP, DIR, ENA, LED_PIN);

// Replace with your network credentials
const char* ssid = "BB9ESERVER"; // type your wifi name
const char* password = "BB9ESERVER";  // type your wifi password

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameters in HTTP POST request
const char* PARAM_INPUT_1 = "direction";
const char* PARAM_INPUT_2 = "steps";

// Variables to save values from HTML form
String direction;
String steps;

// Variable to detect whether a new request occurred
bool newRequest = false;

// HTML to build the web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Stepper Motor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h1>Stepper Motor Control</h1>
    <form action="/" method="POST">
      <input type="radio" name="direction" value="CW" checked>
      <label for="CW">Clockwise</label>
      <input type="radio" name="direction" value="CCW">
      <label for="CW">Counterclockwise</label><br><br><br>
      <label for="steps">Number of steps:</label>
      <input type="number" name="steps">
      <input type="submit" value="GO!">
    </form>
</body>
</html>
)rawliteral";

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println();
  Serial.println(WiFi.localIP());
}


void setup() 
{
  Serial.begin(9600);

  initWiFi();
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA); // ESP-32 as client
  WiFi.config(ipCliente, ipServidor, Subnet);
  Udp.begin(localPort);
  Serial.setTimeout(1000);
  Serial.println("ts12");
  myStepper.setSpeed(5);

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });
  
  // Handle request (form)
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        // HTTP POST input1 value (direction)
        if (p->name() == PARAM_INPUT_1) {
          direction = p->value().c_str();
          Serial.print("Direction set to: ");
          Serial.println(direction);
        }
        // HTTP POST input2 value (steps)
        if (p->name() == PARAM_INPUT_2) {
          steps = p->value().c_str();
          Serial.print("Number of steps set to: ");
          Serial.println(steps);
        }
      }
    }
    request->send(200, "text/html", index_html);
    newRequest = true;
  });

  server.begin();
}

void loop() 
{
   isSerialAvailable();

    if(stepperStart)
    {
        Serial.println("mstart");
        Serial.flush();
        
        myStepper.setSpeed(stepperSpeed);
        myStepper.step(stepperPosition);   
        
        // шлем сообщение о конце движения "move end"
        Serial.println("mend");
        Serial.flush();

        stepperStart = false;
        stepperPosition = 0;
        stepperSpeed = 0;   
    }

  // Check if there was a new request and move the stepper accordingly
  if (newRequest){
    if (direction == "CW"){
      // Spin the stepper clockwise direction
      myStepper.step(steps.toInt());
    }
    else{
      // Spin the stepper counterclockwise direction
      myStepper.step(-steps.toInt());
    }
    newRequest = false;
  }
//unsigned long Tiempo_Envio = millis();

//SENDING
    Udp.beginPacket(ipServidor,9999);   //Initiate transmission of data
    
    Udp.printf("Millis: ");
    
    char buf[20];   // buffer to hold the string to append
    unsigned long testID = millis();   // time since ESP-32 is running millis() 
    sprintf(buf, "%lu", testID);  // appending the millis to create a char
    Udp.printf(buf);  // print the char
    
    Udp.printf("\r\n");   // End segment
    
    Udp.endPacket();  // Close communication
    
    Serial.print("sending: ");   // Serial monitor for user 
    Serial.println(buf);
    
delay(5); // 
 
//RECEPTION
  int packetSize = Udp.parsePacket();   // Size of packet to receive
  if (packetSize) {       // If we received a package
    
    int len = Udp.read(packetBuffer, 255);
    
    if (len > 0) packetBuffer[len-1] = 0;
    Serial.print("Recibido(IP/Size/Data/Dir/Step): ");
    Serial.print(Udp.remoteIP());Serial.print(" / ");
    Serial.print(Udp.remotePort()); Serial.print(" / ");
    Serial.print(packetSize);Serial.print(" / ");
    Serial.print(PARAM_INPUT_1);Serial.print(" CW/ ");
    Serial.print(PARAM_INPUT_2);Serial.print(" sp/ ");
    Serial.println(packetBuffer);
  }
Serial.println("");
delay(5);
}

bool isSerialAvailable()
{ 
    if( !Serial.available() )
    {
        return false;
    }
    
    byte size = Serial.readBytesUntil('.', inputBuf, INPUT_SIZE);
    inputBuf[size] = 0;

    // разбор команды
    // формат строки команды : 'm' + int(Pos) + ',' + int(Speed) + '.'
    // пример "m750,300."

    if( inputBuf[0] != 'm' )
    {
        Serial.println("ts12");
        return false;
    }

    char* separator = strchr(inputBuf+1, ',');
    if (separator != 0)
    {
        *separator = 0;
        stepperPosition = atoi(inputBuf+1);
        ++separator;
        stepperSpeed = atoi(separator);
    }

    if( stepperPosition != 0 && stepperSpeed != 0 )
    {
        stepperStart = true;

        //Serial.println(stepperPosition);
        //Serial.println(stepperSpeed);

        return true;
    }
    else
    {
        stepperStart = false;
        stepperPosition = 0;
        stepperSpeed = 0;

        return false;
    }
}
