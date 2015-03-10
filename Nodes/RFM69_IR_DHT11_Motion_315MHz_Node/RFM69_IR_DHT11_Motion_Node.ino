///////////////////////////////////////////////////////////////////////////////////
// (c) Lucas Jacomet
// 06.03.2015
// RFM69HW IR, DHT11, Motion and 315MHz Node
// 5V Arduino pro Mini with LLC

//      Pinmap:
//          A0:
//          A1:
//          A2: 
//          A3: 
//  SDA     A4: 
//  SCL     A5:
//          A6:
//          A7:
//  RX      D0: FTDI TX for programming only
//  TX      D1: FTDI RX for programming only
//  INT0    D2: DIO0 RFM69HW
//  INT1   ~D3: (D0 315MHz receiver)
//          D4: (D1 315MHz receiver)
//         ~D5: (D2 315MHz receiver)
//         ~D6: Buzzer (D3 315MHz receiver)
//          D7: (PIR motion sensor)
//          D8: DHT11 temperature sensor
//         ~D9: IR receiver diode
//  SS    ~D10: HV1 - LV1 - NSS RFM69HW
//  MOSI  ~D11: HV2 - LV2 - MOSI RFM69HW
//  MISO   D12: MISO RFM69HW
//  SCK    D13: HV3 - LV3 - SCK RFM69HW

#include <RFM69.h>
#include <SPI.h>
#include <IRremote.h>
#include "Dht11.h"

#define NODEID      3
#define NETWORKID   88
#define ENCRYPTKEY  "sampleEncryptKey"
bool highPower = true;
int powerLevel = 8;
bool promiscuousMode = false;
unsigned int retries = 2;
unsigned int retryWaitTime = 50;

typedef struct {
  byte    fromNodeID;
  byte    function;
  long    value;
  byte    voltage;
  byte    temperature;
}
Payload;

Payload txStruct;
Payload rxStruct;

bool gotRxStruct = false;
bool gotTxStruct = false;
bool gotIRData = false;
bool gotBeep = false;

byte toNodeID = 0;
int temperature = 0;
int humidity = 0;

decode_results results;
long int IRCode = 0;

const int buzzerPin = 6;
const int DHT11Pin = 8;
const int IRreceiverPin = 9;

RFM69 radio;
IRrecv irrecv(IRreceiverPin);

void setup()
{
  Serial.begin(57600);
  delay(10);
  radio.initialize(RF69_433MHZ,NODEID,NETWORKID);
  radio.setHighPower(highPower);
  radio.setPowerLevel(powerLevel);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  txStruct.fromNodeID = NODEID;
  irrecv.enableIRIn();
  pinMode(buzzerPin, OUTPUT);
}

void loop() 
{
  receiveData();
  checkIR();
  processIR();
  processData();
  //periodicallySendTempHum();
  sendData();
  if(gotBeep)
    beep(1);
}

void sendStruct(byte toNode, byte function, long value)
{
  toNodeID = toNode;
  txStruct.function = function;
  txStruct.value = value;
  gotTxStruct = true;
}

void sendData()
{
  if(gotTxStruct)
  {
    txStruct.voltage = 0;
    txStruct.temperature = radio.readTemperature(-1);
    radio.send(toNodeID, (const void*)(&txStruct), sizeof(txStruct));
    //  if(radio.sendWithRetry(toNodeID, (const void*)(&txStruct), sizeof(txStruct)))
    //    delay(5);
    //  else
    //    delay(5);
    gotTxStruct = false;
    gotBeep = true;
  }
}

void receiveData(void)
{
  if (radio.receiveDone())
  {
    if ((radio.DATALEN = sizeof(Payload)))
    {
      rxStruct = *(Payload*)radio.DATA;
      gotRxStruct = true;
    }
    else
    {
    }

    if (radio.ACKRequested())
    {
      radio.sendACK();
    }
  }
}

void processData(void)
{
  if(gotRxStruct)
  {
    switch(rxStruct.function)
    {
    case 0:
      sendStruct(rxStruct.fromNodeID, rxStruct.function, rxStruct.value);
      break;

    case 1:
      beep(rxStruct.value);
      break;

    case 2: 
      digitalWrite(buzzerPin, rxStruct.value);
      break;
      
    case 3: 
      getDHT11();
      sendStruct(rxStruct.fromNodeID, 8, temperature);
      break;

    case 4: 
      getDHT11();
      sendStruct(rxStruct.fromNodeID, 9, humidity);
      break;

    default:
      beep(2);
      break;
    }
    gotRxStruct = false;
  }
}

void checkIR(void)
{
  if(irrecv.decode(&results)) 
  {
    IRCode = results.value;
    //Serial.println(IRCode);
    gotIRData = true;
    irrecv.resume();
  }
}

void processIR(void)
{
  if(gotIRData)
  { 
    switch(IRCode) 
    {
    case 2011254980: // Up
      Serial.println("Up");
      sendStruct(1,1,1);
      break;

    case 2011246788: // Down
      Serial.println("Down");
      sendStruct(1,2,1);
      break;

    case 2011271364: // Left
      Serial.println("Left");
      sendStruct(1,3,1);
      break;

    case 2011259076: // Right
      Serial.println("Right");
      sendStruct(1,4,1);
      break;

    case 2011249348: // Enter
      Serial.println("Enter");
      sendStruct(1,5,1);
      break;

    case 2011283652: // Menu
      Serial.println("Menu");
      sendStruct(1,6,1);
      break;

    case 2011298500: // Play
      Serial.println("Play");
      sendStruct(1,7,1);
      //gotBeep = true;
      break;

    default:
      //Serial.println(IRCode);
      break;
    } // end switch (IRCode)
  }
  gotIRData = false;
}

void getDHT11(void)
{
  static Dht11 sensor(DHT11Pin);

  if (sensor.read() == Dht11::OK)
  {
    temperature = sensor.getTemperature();
    humidity = sensor.getHumidity();
  }
  else
  {
    temperature = 0;
    humidity = 0; 
  }
}

void beep(int count)
{
  for(int i=0; i<count; ++i)
  {
    digitalWrite(buzzerPin, HIGH);
    delay(20);
    digitalWrite(buzzerPin, LOW);
    delay(20);
  }
  gotBeep = false;
}















