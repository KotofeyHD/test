#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <StepperDFRobot.h>

WiFiUDP Udp; // Creation of wifi Udp instance

StepperDFRobot motor = StepperDFRobot(400, 4, 5, 6, 7);

bool stepperStart = false;
int stepperPosition = 0;
int stepperSpeed = 0;
int motorPin1 = 4;
int motorPin2 = 5;
int motorPin3 = 6;
int motorPin4 = 7;

#define INPUT_SIZE 32

char inputBuf[INPUT_SIZE + 1];

char packetBuffer[255];

unsigned int localPort = 9999;

const char *ssid = "BB9ESERVER";  
const char *password = "BB9ESERVER";

String serverNameSTEP = "http://192.168.4.1/step";
//String serverNameDIR = "http://192.168.4.1/dir";
//String serverNameENA = "http://192.168.4.1/enable";
//String serverNameLED_PIN = "http://192.168.4.1/led";

void setup() {
  Serial.begin(9600);
  WiFi.softAP(ssid, password);  // ESP-32 as access point
  Udp.begin(localPort);
  Serial.setTimeout(10);
  Serial.println("tl");
  }

void loop() 
{
   isSerialAvailable();

  if (stepperStart)
  {
    Serial.println("mstart");
    Serial.flush();
    motor.setSpeed(stepperSpeed);
    motor.step(stepperPosition);

    // шлем сообщение о конце движения "move end"
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
    digitalWrite(motorPin3, LOW);
    digitalWrite(motorPin4, LOW);
    Serial.println("mend");
    Serial.flush();

    stepperStart = false;
    stepperPosition = 0;
    stepperSpeed = 0;


    stepperStart = false;
    stepperPosition = 0;
    stepperSpeed = 0;
  }


  //-----------------------------------------
  // HTTP REQUEST 
  if(WiFi.status()== WL_CONNECTED ){ 
      HTTPClient http;
      http.begin(serverNameSTEP.c_str());
     // http.begin(serverNameDIR.c_str());
      //http.begin(serverNameENA.c_str());
      //http.begin(serverNameLED_PIN.c_str());
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      Serial.println();
    }
  //-----------------------------------------

  
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len-1] = 0;
    Serial.print("Recibido(IP/Size/Data/Dir/Step): ");
    Serial.print(Udp.remoteIP());Serial.print(" / ");
    Serial.print(packetSize);Serial.print(" / ");
    Serial.println(packetBuffer);

    Udp.beginPacket(Udp.remoteIP(),Udp.remotePort());
    Udp.printf("received: ");
    Udp.printf(packetBuffer);
    Udp.printf("\r\n");
    Udp.endPacket();
     }
  }
}

bool isSerialAvailable()
{
  if ( !Serial.available() )
  {
    return false;
  }

  byte size = Serial.readBytesUntil('.', inputBuf, INPUT_SIZE);
  inputBuf[size] = 0;

  // разбор команды
  // формат строки команды : 'm' + int(Pos) + ',' + int(Speed) + '.'
  // пример "m750,300."

  if ( inputBuf[0] != 'm' )
  {
    Serial.println("tl");
    return false;
  }


  char* separator = strchr(inputBuf + 1, ',');
  if (separator != 0)
  {
    *separator = 0;
    stepperPosition = atoi(inputBuf + 1);
    ++separator;
    stepperSpeed = atoi(separator);
  }

  if ( stepperPosition != 0 && stepperSpeed != 0 )
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
