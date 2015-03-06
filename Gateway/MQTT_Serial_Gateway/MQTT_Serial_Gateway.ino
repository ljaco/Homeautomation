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

typedef struct {
  byte    fromNodeID;
  byte    function;
  int     value1;
  int     value2;
  int     value3;
  byte    voltage;
  byte    temperature;
} 
Payload;

Payload rxStruct;
int lastRSSI = 0;

bool gotSerialRXData = false;

char mqtt_buff[30];

char serial_buff[30];
String serialString;

EthernetClient ethernetClient; 
PubSubClient mqtt(MQTT_SERVER, 1883, callback, ethernetClient);
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, 7, NEO_GRB + NEO_KHZ800);

bool newPush = false;
bool newRelease = true;
int buzzer  = 8;
int button   = 9;
bool gotRed = false;
bool gotGreen = false;
bool gotBlue = false;
byte red = 0;
byte green = 0;
byte blue = 0;


void setup()
{
  Serial.begin(57600);
  delay(500);
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

  if (mqtt.connect("arduinoClient"))
  {     
    mqtt.subscribe("#"); // subscribe to all topics
    //    mqtt.subscribe("buzzer");
    //    mqtt.subscribe("remoteBuzzer");
    //    mqtt.subscribe("temperature1");
    //    mqtt.subscribe("humidity1");
    //    mqtt.subscribe("humidity1");
    //    mqtt.subscribe("node/+/+");

    //Serial.println("Connected to MQTT");
  }
  else
  {
    //Serial.println("Failed to connected to MQTT");
  }

  pinMode(button, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, HIGH);
  pixel.begin();
}

void loop()
{ 
  mqtt.loop();
  handleInputs();
  getSerialRadioData();
  newPublishToMqtt();
  updatePixel();
}

void callback(char* topic, byte* payload, unsigned int length)
{
  String topicStr = String(topic);

  int i = 0;
  for(i=0; i<length; i++)
    mqtt_buff[i] = payload[i];
  mqtt_buff[i] = '\0';
  String messageStr = String(mqtt_buff);

  //Serial.print("Subscribed topic: ");
  //Serial.print(topic);
  //Serial.print(", with message:");
  //Serial.println(messageStr);



  // topicStr == toNode8/function2
  //             01234567890123456 == 17 char
  // messageStr == s,1,2,3 or just 2

  if(topicStr.substring(0,6) == "toNode")
  {
    byte slash = topicStr.indexOf('/');
    byte toNode = topicStr.substring(6,slash).toInt();
    byte function = topicStr.substring(slash + 9, topicStr.length()).toInt();
    
    if(messageStr.substring(0,1) == "s")
    {
      byte secondComma = messageStr.indexOf(',', 2);
      byte thirdComma = messageStr.lastIndexOf(',');
      int value1 = messageStr.substring(2,secondComma).toInt();
      int value2 = messageStr.substring(secondComma + 1, thirdComma).toInt();
      int value3 = messageStr.substring(thirdComma + 1, messageStr.length()).toInt();
      
      sendSerialRadioData(toNode, function, value1, value2, value3);
    }
    else
    {
      int value = messageStr.toInt();
      
      sendSerialRadioData(toNode, function, value);
    }
  }
  else
  {
    if (topicStr == "buzzer")
    {
      if(messageStr == "1")
        digitalWrite(buzzer, LOW);
      else if(messageStr == "0")
        digitalWrite(buzzer, HIGH);
    }


//    else if (strcmp(topic,"remoteBuzzer") == 0)
//    {
//      if(messageStr == "1")
//        sendSerialRadioData(3,3,1);
//      else if(messageStr == "0")
//        sendSerialRadioData(3,3,0);
//    }


    else if (strcmp(topic,"RGB/red") == 0)
    {
      red = messageStr.toInt();
      gotRed = true;
    }


    else if (strcmp(topic,"RGB/green") == 0)
    {
      green = messageStr.toInt();
      gotGreen = true;
    }


    else if (strcmp(topic,"RGB/blue") == 0)
    {
      blue = messageStr.toInt();
      gotBlue = true;
    }


    else if (strcmp(topic,"led1") == 0)
    {

    }


    else if (strcmp(topic,"bla") == 0)
    {

    }


    else if (strcmp(topic,"blo") == 0)
    {

    }


    else
    {
      Serial.println("No topic match!"); 
    }
  }
}

void handleInputs(void)
{
  if(digitalRead(button) == 0 && !newPush){
    mqtt.publish("buzzer", (uint8_t*)"1", 1, true);
    //Serial.println("PUSHED");
    newPush = true;
    newRelease = false;
  }
  else if (digitalRead(button) == 1 && !newRelease){
    mqtt.publish("buzzer", (uint8_t*)"0", 1, true);
    //Serial.println("RELEASED");
    newRelease = true;
    newPush = false;
  }
}

void getSerialRadioData(void)
{ // s,3,9,5,0,0,0,24; enter button
  if(Serial.available() > 0)
  { 
    delay(4); // time to receive some char
    if(Serial.read() == 's')
    {  // s,fromNode,func,val1,val2,val3,volt,temp;
      rxStruct.fromNodeID = Serial.parseInt();
      rxStruct.function = Serial.parseInt();
      rxStruct.value1 = Serial.parseInt();
      rxStruct.value2 = Serial.parseInt();
      rxStruct.value3 = Serial.parseInt();
      rxStruct.voltage = Serial.parseInt();
      rxStruct.temperature = Serial.parseInt();
      lastRSSI = Serial.parseInt();
      if(Serial.read() == ';')
        gotSerialRXData = true;
    }
  }
}

void sendSerialRadioData(byte destination, byte function, int value1, int value2, int value3)
{
  char buffer [25];
  sprintf(buffer, "s,%d,%d,%d,%d,%d;",destination, function, value1, value2, value3);
  Serial.print(buffer);

  //  Serial.print('s');
  //  Serial.print(',');
  //  Serial.print(destination);
  //  Serial.print(',');
  //  Serial.print(function);
  //  Serial.print(',');
  //  Serial.print(value1);
  //  Serial.print(',');
  //  Serial.print(value2);
  //  Serial.print(',');
  //  Serial.print(value3);
  //  Serial.print(';');
}

void newPublishToMqtt (void)
{
  if(gotSerialRXData)
  {

    char topic1 [35];
    sprintf(topic1, "fromNode%d/function%d/value1",rxStruct.fromNodeID, rxStruct.function);
    char message1 [10];
    sprintf(message1, "%d",rxStruct.value1);
    mqtt.publish(topic1, (uint8_t*)message1, strlen(message1), true);


    char topic2 [35];
    sprintf(topic2, "fromNode%d/function%d/value2",rxStruct.fromNodeID, rxStruct.function);
    char message2 [10];
    sprintf(message2, "%d",rxStruct.value2);
    mqtt.publish(topic2, (uint8_t*)message2, strlen(message2), true);


    char topic3 [35];
    sprintf(topic3, "fromNode%d/function%d/value3",rxStruct.fromNodeID, rxStruct.function);
    char message3 [10];
    sprintf(message3, "%d",rxStruct.value3);
    mqtt.publish(topic3, (uint8_t*)message3, strlen(message3), true);


    char nodeVoltage [35];
    sprintf(nodeVoltage, "fromNode%d/voltage",rxStruct.fromNodeID);
    char voltage [10];
    sprintf(voltage, "%d",rxStruct.voltage);
    mqtt.publish(nodeVoltage, (uint8_t*)voltage, strlen(voltage), true);


    char nodeTemperature [35];
    sprintf(nodeTemperature, "fromNode%d/temperature",rxStruct.fromNodeID);
    char temperature [10];
    sprintf(temperature, "%d",rxStruct.temperature);
    mqtt.publish(nodeTemperature, (uint8_t*)temperature, strlen(temperature), true);


    char nodeRSSI [35];
    sprintf(nodeRSSI, "fromNode%d/rssi",rxStruct.fromNodeID);
    char rssi [10];
    sprintf(rssi, "%d", lastRSSI);
    mqtt.publish(nodeRSSI, (uint8_t*)rssi, strlen(rssi), true);


    //mqtt.publish("nodes/node99/function0/value0", "88");
    gotSerialRXData = false;
  }

}

void updatePixel (void)
{
  if(gotRed && gotGreen && gotBlue)
  {
    red = map(red, 0, 100, 0, 255);
    green = map(green, 0, 100, 0, 255);
    blue = map(blue, 0, 100, 0, 255);
    pixel.setPixelColor(0, red, green, blue);
    pixel.show();
    gotRed = false;
    gotGreen = false;
    gotBlue = false;
  }

}

//void serialDebug(void)
//{
//  if(serialRadio.available() > 0)
//  {
//    Serial.println(serialRadio.available());
//    delay(4);
//    int i = 0;
//    char buffer [20]; 
//    while(serialRadio.available() > 0)
//    {
//      Serial.print(serialRadio.read(), BIN);
//      Serial.println();
//    }
//  }
//
//}






































