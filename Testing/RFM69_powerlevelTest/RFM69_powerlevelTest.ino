///////////////////////////////////////////////////////////////////////////////////
// (c) Lucas Jacomet
// 07.01.2015
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

#define NODEID      9
#define NETWORKID   88
#define ENCRYPTKEY  "sampleEncryptKey"
bool highPower = true;
unsigned int powerLevel = 15;
bool promiscuousMode = false;

typedef struct {
  int     fromNodeID;
  int     function;
  int     value1;
  //int     value2;
  //int     value3;
  byte     temperature;
} 
Payload;

Payload txData;
Payload rxData;

bool gotRxData = false;
bool gotTxData = false;


int toNodeID = 0;

RFM69 radio;

void setup()
{
  Serial.begin(57600);
  delay(10);
  radio.initialize(RF69_433MHZ,NODEID,NETWORKID);
  radio.setHighPower(highPower);
  radio.setPowerLevel(powerLevel);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  txData.fromNodeID = NODEID;
}

void loop() 
{
  radio.setPowerLevel(powerLevel);
  delay(1);
  sendStruct(2,0,powerLevel);
  Serial.println(powerLevel);
  --powerLevel;
  if(powerLevel < 0 || powerLevel > 15)
    powerLevel = 15;
  delay(800);
  //  receiveData();
  //  if(gotRxData)
  //    processData();
  if(gotTxData)
    sendData();
}

void sendStruct(int toNode, int function, int value1)
{
  toNodeID = toNode;
  txData.function = function;
  txData.value1 = value1;
  gotTxData = true;
}

void sendData()
{
  txData.temperature = radio.readTemperature(-1);
  radio.send(toNodeID, (const void*)(&txData), sizeof(txData));
  //  if(radio.sendWithRetry(toNodeID, (const void*)(&txData), sizeof(txData)))
  //    delay(5);
  //  else
  //    delay(5);
  gotTxData = false;
}

void receiveData(void)
{
  if (radio.receiveDone())
  {
    if ((radio.DATALEN = sizeof(Payload)))
    {
      rxData = *(Payload*)radio.DATA;
      gotRxData = true;
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
  switch(rxData.function)
  {

  default:
    break;
  }
  gotRxData = false;
}










