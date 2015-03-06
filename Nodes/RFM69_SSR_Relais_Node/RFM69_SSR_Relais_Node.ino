///////////////////////////////////////////////////////////////////////////////////
// (c) Lucas Jacomet
// 09.01.2015
// RFM69HW SSR and Relais Node
// 3.3V Arduino Pro mini

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
//  INT1   ~D3: pin3
//          D4: pin4
//         ~D5: pin5
//         ~D6: pin6
//          D7: pin7
//          D8: pin8 Relais 8 LED
//         ~D9: pin9 Relais 9 Radio
//  SS    ~D10: NSS RFM69HW
//  MOSI  ~D11: MOSI RFM69HW
//  MISO   D12: MISO RFM69HW
//  SCK    D13: SCK RFM69HW

#include <RFM69.h>
#include <SPI.h>

#define NODEID    3   
#define NETWORKID 88
#define ENCRYPTKEY  "sampleEncryptKey"
bool highPower = true;
int powerLevel = 8;
bool promiscuousMode = false;
unsigned int retries = 2;
unsigned int retryWaitTime = 50;

typedef struct {
  int     fromNodeID;	
  int     function; // 0: ping, 1: set pin 8 an9 9 to value1, 8: set pin 9 to value1, 9: set pin 9 to value1
  int     value1; // 0: LOW, 1: HIGH, 2: toggle, 3:get state
  //int     value2;
  //int     value3;
  byte     temperature;
} 
Payload;

Payload txStruct;
Payload rxStruct;

bool gotRxStruct = false;
bool gotTxStruct = false;
int toNodeID = 0;

const int pinCount = 10;
bool pinState[pinCount]; //just index 3 - 9 used

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
  txStruct.fromNodeID = NODEID;

  for (int i=0; i < pinCount; ++i)
    pinState[i] = false;

  for(int i=3; i <= 9; ++i)
  {
    pinMode(i, OUTPUT);
  }
}

void loop() 
{
  receiveData();
  processData();
  sendData();
}

void sendStruct(int toNode, int function, int value1)
{
  toNodeID = toNode;
  txStruct.function = function;
  txStruct.value1 = value1;
  gotTxStruct = true;
}

void sendData()
{
  if(gotTxStruct)
  {
    txStruct.temperature = radio.readTemperature(-1);
    radio.send(toNodeID, (const void*)(&txStruct), sizeof(txStruct));
//    if(radio.sendWithRetry(toNodeID, (const void*)(&txStruct), sizeof(txStruct)), retries, retryWaitTime)
//      delay(10);
//    else
//      delay(10);
    gotTxStruct = false;
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
      sendStruct(rxStruct.fromNodeID, rxStruct.function, rxStruct.value1);
      break;

    case 1: // all on or off
      setPin(8, rxStruct.value1);
      setPin(9, rxStruct.value1);
      break;

    case 8:
      processPinFunction();
      break;

    case 9:
      processPinFunction();
      break;

    default:
      break;
    }
    gotRxStruct = false;
  }
}

void processPinFunction(void)
{
  switch(rxStruct.value1)
  {
  case 0: //set pin to LOW
    pinState[rxStruct.function] = false;
    digitalWrite(rxStruct.function, LOW);
    break;

  case 1: //set pin to HIGH
    pinState[rxStruct.function] = true;
    digitalWrite(rxStruct.function, HIGH);
    break;

  case 2: //toggle pin
    pinState[rxStruct.function] =!pinState[rxStruct.function];
    digitalWrite(rxStruct.function, pinState[rxStruct.function]);
    //sendStruct(rxStruct.fromNodeID, rxStruct.function, pinState[rxStruct.function]);
    break;

  case 3: //send state of the pin back
    sendStruct(rxStruct.fromNodeID, rxStruct.function, pinState[rxStruct.function]);
    break;

  default:
    break;
  }
}

void togglePin(int pin)
{
  pinState[pin] =!pinState[pin];
  digitalWrite(pin, pinState[pin]);
}

void setPin(int pin, bool onOff)
{
  pinState[pin] = onOff;
  digitalWrite(pin, pinState[pin]);
}








