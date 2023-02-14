#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AccelStepper.h>

bool stepperStart = false;
int stepperPosition = 0;
int stepperSpeed = 0;
int  packetSize = 0;

#define INPUT_SIZE 32

char inputBuf[INPUT_SIZE + 1];
char packetBuffer[INPUT_SIZE + 1];

const char *ssid = "BB9ESERVER";  
const char *password = "BB9ESERVER";

WiFiUDP udp;
WiFiUDP udp2;

void setup()
{
 
  Serial.begin(115200); // set up Serial library at 9600 bps

  WiFi.softAP(ssid, password);  // ESP-32 as access point
  
  udp2.begin(9091);  

  Serial.setTimeout(10);
  Serial.println("tl");

}


#pragma pack(push, 1)
typedef struct {
  int32_t magic_number;
  int32_t stepper_position;
  int32_t stepper_speed;
} control_packet_udp_sock_t;
#pragma pack(pop)

control_packet_udp_sock_t ctrl_pack_0;

const char * udpAddress = "192.168.4.100";
const int udpPort = 9090;

bool isSerialAvailable();

void loop()
{

  isSerialAvailable();
  
  if(stepperStart) {

    ctrl_pack_0.magic_number = 666666;
    ctrl_pack_0.stepper_position = stepperPosition;
    ctrl_pack_0.stepper_speed = stepperSpeed;
    
    udp.beginPacket(udpAddress,udpPort);
    udp.write((uint8_t *) &ctrl_pack_0, sizeof(control_packet_udp_sock_t));
    udp.endPacket();

  /*
    while (1) {
      
      packetSize = udp2.parsePacket();
        
      if( packetSize <= 0 ) 
        continue;

      udp2.read(packetBuffer, 255);

      if (strcmp (packetBuffer, "mstart" ) == 0) {

        Serial.println("mstart");
        Serial.flush();
        break;

      }
    }

    while (1) {
      
      sleep(100);

      packetSize = udp2.parsePacket();
        
      if( packetSize <= 0 ) 
        continue;

      udp2.read(packetBuffer, 255);

      if (strcmp (packetBuffer, "mend" ) == 0) {

        Serial.println("mend");
        Serial.flush();
        break;

      }
    }

  */



    stepperStart = false;
    stepperPosition = 0;
    stepperSpeed = 0;   
  }

  packetSize = udp2.parsePacket();
    
  if( packetSize > 0 ) {
    
    udp2.read(packetBuffer, 255);
    packetBuffer[packetSize]='\0';

    if (strcmp (packetBuffer, "mstart" ) == 0) {

      Serial.println("mstart");
      Serial.flush();

    } else if (strcmp (packetBuffer, "mend" ) == 0) {
      
      Serial.println("mend");
      Serial.flush();

    } else {
      Serial.println(packetBuffer);
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

    // Serial.println(stepperPosition);
    // Serial.println(stepperSpeed);

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
  
