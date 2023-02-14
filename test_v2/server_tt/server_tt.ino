#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AccelStepper.h>

// WIFI STA PARAMETERS

const char* wifi_sta_ssid = "BB9ESERVER"; // type your wifi name
const char* wifi_sta_password = "BB9ESERVER";  // type your wifi password

IPAddress self_ip(192, 168, 4, 100);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

const char * udpAddress = "192.168.4.10";


void init_wifi_sta(void);
void init_udp_socket(void);

int udp_sock_port = 9090;
WiFiUDP udp_sock;

// STEPPER MOTOR PARAMETERS

bool stepperStart = false;
int stepperPosition = 0;
int stepperSpeed = 0;

const int DIR = 2;
const int STEP = 1;
const int ENA = 3;
const int LED_PIN = 15;
#define motorInterfaceType 1
AccelStepper stepper_0(motorInterfaceType, STEP, DIR);

// 

void setup() {
  
  init_wifi_sta();
  init_udp_socket();

  Serial.begin(115200);
  
  stepper_0.setMaxSpeed(10000);
  stepper_0.setAcceleration(5000);
  stepper_0.setSpeed(10000); 

}

void loop() {
  // put your main code here, to run repeatedly:
  proc_udp_socket();

  if(stepperStart) {
    Serial.println("mstart");
    Serial.flush();

    //сообщаем, что пакет отправлен 
    udp_sock.beginPacket(udp_sock.remoteIP(),9091);
    udp_sock.print("mstart");
    udp_sock.endPacket();

    stepper_0.setSpeed(stepperSpeed);
    stepper_0.setMaxSpeed(stepperSpeed);
    stepper_0.setAcceleration(stepperSpeed);
    stepper_0.move(stepperPosition);
    while(stepper_0.distanceToGo() != 0) {
      stepper_0.run();
    }
    
    Serial.println("mend");
    Serial.flush();
    udp_sock.beginPacket(udp_sock.remoteIP(),9091);
    udp_sock.print("mend");
    udp_sock.endPacket();
    stepperStart = false;
    stepperPosition = 0;
    stepperSpeed = 0;   
  }
    
}

// PD -> T -> PT1 -> D
// T -> PT2 -> D

// FUNCTIONS

void init_wifi_sta(void) {
  
  Serial.print("Connecting to ");
  Serial.println(wifi_sta_ssid);
  
  if (!WiFi.config(self_ip, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  
  WiFi.begin(wifi_sta_ssid, wifi_sta_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void init_udp_socket(void) {
  udp_sock.begin(udp_sock_port);  
}

#pragma pack(push, 1)
typedef struct {
  int32_t magic_number;
  int32_t stepper_position;
  int32_t stepper_speed;
} control_packet_udp_sock_t;
#pragma pack(pop)

void proc_udp_socket() {
  
  char packetBuffer[255];
  int packetSize = 0; 
  control_packet_udp_sock_t *p_control_packet_0;
  int len = 0;
  p_control_packet_0 = (control_packet_udp_sock_t *) packetBuffer;

  // check that something is received 
  packetSize = udp_sock.parsePacket();
  if( packetSize <= 0 ) 
    return;

  // get something  
  len = udp_sock.read(packetBuffer, 255);

  // parse
  if (len == sizeof(control_packet_udp_sock_t)) {

    if ( p_control_packet_0->magic_number == 666666 ) {
      if ( ( p_control_packet_0->stepper_position != 0 ) && ( p_control_packet_0->stepper_speed != 0 ) ) {
        stepperStart = true;
        stepperPosition = p_control_packet_0->stepper_position;
        stepperSpeed = p_control_packet_0->stepper_speed;  
        
        Serial.print("received.stepperPosition = ");
        Serial.println(stepperPosition);
        Serial.flush(); 
        
        Serial.print("received.stepperSpeed = ");
        Serial.println(stepperSpeed);
        Serial.flush();
    
        
      } else {
        stepperStart = false;
        stepperPosition = 0;
        stepperSpeed = 0;
      }        
    }  
  }
  
}
