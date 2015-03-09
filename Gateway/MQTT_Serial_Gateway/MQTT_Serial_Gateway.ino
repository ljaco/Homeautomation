///////////////////////////////////////////////////////////////////////////////////
// (c) Lucas Jacomet
// 06.03.2015
// MQTT Serial Gateway
// 5V Arduino Duemillanove

//      Pinmap:
//          A0:
//          A1:
//          A2: 
//          A3: 
//  SDA     A4: 
//  SCL     A5:
//          A6:
//          A7:
//  RX      D0: FTDI TX for programming and communication to RFM69HW gateway TX
//  TX      D1: FTDI RX for programming and communication to RFM69HW gateway RX
//  INT0    D2: 
//  INT1   ~D3: 
//          D4: 
//         ~D5: 
//         ~D6: 
//          D7: 
//          D8: 
//         ~D9: 
//  SS    ~D10: Ethernet shield
//  MOSI  ~D11: Ethernet shield
//  MISO   D12: Ethernet shield
//  SCK    D13: Ethernet shield

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

#define MQTT_SERVER "192.168.0.6"

byte mac[]= { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,0, 20);
IPAddress gateway(192,168,0, 1);
IPAddress subnet(255, 255, 255, 0);

void sendSerialRadioData(byte destination, byte function = 0, long value = 0);

typedef struct
{
  byte    fromNodeID;
  byte    function;
  long    value;
  byte    voltage;
  byte    temperature;
  int     RSSI;
}
Payload;

Payload rxStruct;

bool gotSerialRXData = false;

EthernetClient ethernetClient; 
PubSubClient mqtt(MQTT_SERVER, 1883, callback, ethernetClient);
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, 7, NEO_GRB + NEO_KHZ800);

bool newPush = false;
bool newRelease = true;
const int buzzerPin  = 8;
const int buttonPin   = 9;

long pixelColor = 0;
bool gotColor = false;


void setup()
{
  Serial.begin(57600);
  delay(500);
  pixel.begin();
  if (Ethernet.begin(mac) == 0)
    //  {
    //    Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip);
  //  }
  //  Serial.print("IP address: ");
  //  for (byte thisByte = 0; thisByte < 4; thisByte++)
  //  {
  //    Serial.print(Ethernet.localIP()[thisByte], DEC);
  //    Serial.print(".");
  //  }
  //  Serial.println();

  if (mqtt.connect("eth1Client", "eth/1/in/status", 0, 1, "offline"))
  {     
    mqtt.subscribe("node/+/out/+"); // subscribe to all outgoing node topics
    mqtt.subscribe("eth/1/out/+"); // subscribe to all outgoing eth topics
    mqtt.publish("eth/1/in/status", (uint8_t*)"online", 6, true);
    //Serial.println("Connected to MQTT");
  }
  else
  {
    //Serial.println("Failed to connected to MQTT");
  }

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);
  pixel.setPixelColor(0, 0, 0, 0);
  pixel.show();
}

void loop()
{ 
  mqtt.loop();
  checkInputs();
  getSerialRadioData();
  newPublishToMqtt();
  updatePixel();
}

void callback(char* topic, byte* payload, unsigned int length)
{
  String topicStr = String(topic);

  int i = 0;
  char mqtt_buff[30];
  for(i=0; i<length; i++)
    mqtt_buff[i] = payload[i];
  mqtt_buff[i] = '\0';
  String messageStr = String(mqtt_buff);

  //Serial.print("Subscribed topic: ");
  //Serial.print(topic);
  //Serial.print(", with message:");
  //Serial.println(messageStr);



  // topicStr == node/8/out/2
  //             01234567890123456 == 12 char
  // messageStr == s,1,2,3 or just 2
  byte firstSlash = topicStr.indexOf('/');
  byte secondSlash = topicStr.indexOf('/', firstSlash + 1);
  byte thirdSlash = topicStr.indexOf('/', secondSlash + 1);
  byte function = topicStr.substring(thirdSlash + 1, topicStr.length()).toInt();
  long value = messageStr.toInt();
  
  if(topicStr.substring(0,firstSlash) == "node")
  {
    byte toNode = topicStr.substring(firstSlash + 1,secondSlash).toInt();
    sendSerialRadioData(toNode, function, value);  
  }
  else if(topicStr.substring(0,firstSlash) == "eth")
  {
  switch(function)
    {
    case 0:
    break;
    
    case 1:
    if(messageStr == "1")
      digitalWrite(buzzerPin, LOW);
    else if(messageStr == "0")
      digitalWrite(buzzerPin, HIGH);
    break;
    
    case 2:
    pixelColor = value;
    gotColor = true;
    break;
    
    case 3:
    break;
    
    default:
    break;
    
    }
  }
  else
  {
    // No topic mach!
  }
}

void checkInputs(void)
{
  if(digitalRead(buttonPin) == 0 && !newPush){
    mqtt.publish("eth/1/in/1", (uint8_t*)"1", 1, true);
    //Serial.println("PUSHED");
    newPush = true;
    newRelease = false;
  }
  else if (digitalRead(buttonPin) == 1 && !newRelease){
    mqtt.publish("eth/1/in/1", (uint8_t*)"0", 1, true);
    //Serial.println("RELEASED");
    newRelease = true;
    newPush = false;
  }
}

void getSerialRadioData(void)
{ // s,9,2,1,0,23,-45;
  if(Serial.available() > 0)
  { 
    delay(4); // time to receive some char
    if(Serial.read() == 's')
    {  // s,fromNode,function,value,volt,temp,RSSI;
      rxStruct.fromNodeID = Serial.parseInt();
      rxStruct.function = Serial.parseInt();
      rxStruct.value = Serial.parseInt();
      rxStruct.voltage = Serial.parseInt();
      rxStruct.temperature = Serial.parseInt();
      rxStruct.RSSI = Serial.parseInt();
      if(Serial.read() == ';')
        gotSerialRXData = true;
    }
  }
}

void sendSerialRadioData(byte destination, byte function, long value)
{
  char buffer [25];
  sprintf(buffer, "s,%d,%d,%ld;",destination, function, value);
  Serial.print(buffer);
}

void newPublishToMqtt (void)
{
  if(gotSerialRXData)
  {
    char topicValue [25];
    sprintf(topicValue, "node/%d/in/%d",rxStruct.fromNodeID, rxStruct.function);
    char value [12];
    sprintf(value, "%ld",rxStruct.value);
    mqtt.publish(topicValue, (uint8_t*)value, strlen(value), true);

    char topicVoltage [25];
    sprintf(topicVoltage, "node/%d/in/voltage",rxStruct.fromNodeID);
    char voltage [12];
    sprintf(voltage, "%d",rxStruct.voltage);
    mqtt.publish(topicVoltage, (uint8_t*)voltage, strlen(voltage), true);

    char topicTemperature [30];
    sprintf(topicTemperature, "node/%d/in/temperature",rxStruct.fromNodeID);
    char temperature [12];
    sprintf(temperature, "%d",rxStruct.temperature);
    mqtt.publish(topicTemperature, (uint8_t*)temperature, strlen(temperature), true);

    char topicRSSI [25];
    sprintf(topicRSSI, "node/%d/in/rssi",rxStruct.fromNodeID);
    char rssi [12];
    sprintf(rssi, "%d", rxStruct.RSSI);
    mqtt.publish(topicRSSI, (uint8_t*)rssi, strlen(rssi), true);
    
    gotSerialRXData = false;
  }
}

void updatePixel (void)
{
  if(gotColor)
  {
    byte red = map((pixelColor >> 20), 0, 100, 0, 255);
    byte green = map(((pixelColor >> 10) & 0x3ff), 0, 100, 0, 255);
    byte blue = map((pixelColor & 0x3ff), 0, 100, 0, 255);
    pixel.setPixelColor(0, red, green, blue);
    pixel.show();
  }
}


































