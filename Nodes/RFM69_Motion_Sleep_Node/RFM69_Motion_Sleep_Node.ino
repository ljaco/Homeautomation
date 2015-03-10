///////////////////////////////////////////////////////////////////////////////////
// (c) Lucas Jacomet
// 09.03.2015
// RFM69HW Motion Node
// 3.3V Arduino pro Mini with LLC

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
//  INT1   ~D3: 
//          D4: 
//         ~D5: 
//         ~D6: 
//          D7: 
//          D8: 
//         ~D9: PIR motion sensor
//  SS    ~D10: HV1 - LV1 - NSS RFM69HW
//  MOSI  ~D11: HV2 - LV2 - MOSI RFM69HW
//  MISO   D12: MISO RFM69HW
//  SCK    D13: HV3 - LV3 - SCK RFM69HW

#include <RFM69.h>
#include <SPI.h>

#define NODEID      5
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

bool gotTxStruct = false;

byte toNodeID = 0;

bool lastMotionState = false;

const int motionPin = 9;

RFM69 radio;


void wakeUp()
{
  // interrupt handler
}


void setup()
{
  radio.initialize(RF69_433MHZ,NODEID,NETWORKID);
  radio.setHighPower(highPower);
  radio.setPowerLevel(powerLevel);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  txStruct.fromNodeID = NODEID;
  pinMode(motionPin, INPUT);
}

void loop() 
{
  sendData();
  checkForMotion();
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
  }
}


void checkForMotion (void)
{
  if(digitalRead(motionPin) == HIGH && !lastMotionState)
  {
    sendStruct(1,20,1);
    lastMotionState = true;
  }
  else if(digitalRead(motionPin) == LOW && lastMotionState)
  {
    sendStruct(1,20,0);
    lastMotionState = false; 
  }
}














