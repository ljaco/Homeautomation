///////////////////////////////////////////////////////////////////////////////////
// (c) Lucas Jacomet
// 06.03.2015
// RFM69HW MQTT Serial Node
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
//  RX      D0: FTDI TX for programming and communication to ethernet gateway TX
//  TX      D1: FTDI RX for programming and communication to ethernet gateway RX
//  INT0    D2: DIO0 RFM69HW
//  INT1   ~D3: 
//          D4: 
//         ~D5: 
//         ~D6: 
//          D7: 
//          D8: 
//         ~D9: 
//  SS    ~D10: HV1 - LV1 - NSS RFM69HW
//  MOSI  ~D11: HV2 - LV2 - MOSI RFM69HW
//  MISO   D12: MISO RFM69HW
//  SCK    D13: HV3 - LV3 - SCK RFM69HW

#include <RFM69.h>
#include <SPI.h>

#define NODEID      1
#define NETWORKID   88
#define ENCRYPTKEY  "sampleEncryptKey"
bool highPower = true;
int powerLevel = 8;
bool promiscuousMode = false;
unsigned int retries = 2;
unsigned int retryWaitTime = 50;

typedef struct
{
  byte    fromNodeID;
  byte    function;
  long    value;
  byte    voltage;
  byte    temperature;
}
Payload;

Payload txStruct;
Payload rxStruct;

int lastRSSI = 0;

bool gotRxStruct = false;
bool gotTxStruct = false;

byte toNodeID = 0;

char serial_buff[20];
String serialString;

RFM69 radio;

void setup()
{
  Serial.begin(57600);
  Serial.setTimeout(5);
  delay(10);
  radio.initialize(RF69_433MHZ,NODEID,NETWORKID);
  radio.setHighPower(highPower);
  radio.setPowerLevel(powerLevel);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  txStruct.fromNodeID = NODEID;
  txStruct.voltage = 0;
}

void loop() 
{
  receiveRadioData();
  processRadioData();
  getSerialData();
  sendRadioData();
}

void receiveRadioData(void)
{
  if (radio.receiveDone())
  {
    if ((radio.DATALEN = sizeof(Payload)))
    {
      rxStruct = *(Payload*)radio.DATA;
      lastRSSI = radio.readRSSI();
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

void processRadioData(void)
{
  if(gotRxStruct)
  {
    char buffer [30];
    sprintf(buffer, "s,%d,%d,%ld,%d,%d,%d;",rxStruct.fromNodeID, rxStruct.function, rxStruct.value, rxStruct.voltage, rxStruct.temperature, lastRSSI);
    Serial.println(buffer);
    gotRxStruct = false;
  }
}

void getSerialData(void)
{
  if(Serial.available() > 0)
  {
    delay(4); // time to receive some char
    if(Serial.read() == 's')
    {  // s,toNodeID,function,value;
      toNodeID = Serial.parseInt();
      txStruct.function = Serial.parseInt();
      txStruct.value = Serial.parseInt();
      if(Serial.read() == ';')
        gotTxStruct = true;
    }
  }
}

void sendRadioData(void)
{
  if(gotTxStruct)
  {
    txStruct.temperature = radio.readTemperature(-1);
    radio.send(toNodeID, (const void*)(&txStruct), sizeof(txStruct));
    //  if(radio.sendWithRetry(toNodeID, (const void*)(&txStruct), sizeof(txStruct)))
    //    delay(5);
    //  else
    //    delay(5);
    gotTxStruct = false;
  }
}



