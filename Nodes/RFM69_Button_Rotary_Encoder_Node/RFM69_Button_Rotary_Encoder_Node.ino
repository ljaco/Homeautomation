///////////////////////////////////////////////////////////////////////////////////
// (c) Lucas Jacomet
// 07.01.2015
// RFM69HW Button and Rotary encoder Node
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
//  INT1   ~D3: Red, Silver and Rotary Encoder Button over Diode for wakeup
//          D4: Red Button
//         ~D5: Silver Button
//         ~D6: Rotary Encoder CLK
//          D7: Rotary Encoder DT
//          D8: Rotary Encoder Button
//         ~D9: 
//  SS    ~D10: NSS RFM69HW
//  MOSI  ~D11: MOSI RFM69HW
//  MISO   D12: MISO RFM69HW
//  SCK    D13: SCK RFM69HW

#include <RFM69.h>
#include <SPI.h>
#include <Rotary.h>
#include "LowPower.h"

#define NODEID    4    
#define NETWORKID 88
#define ENCRYPTKEY  "sampleEncryptKey"
bool highPower = true;
int powerLevel = 8;
bool promiscuousMode = false;
unsigned int retries = 2;
unsigned int retryWaitTime = 50;

void sendStruct(int toNode, int function=0, int value1=0, int value2=0, int value3=0);

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

Payload txStruct;
Payload rxStruct;

const int wakeUpPin = 3;
const int redPin = 4;
const int silverPin = 5;
const int rotaryEncoderPin1 = 6;
const int rotaryEncoderPin2 = 7;
const int rotaryEncoderButtonPin = 8;

unsigned char rotaryResult = 0;

long debounceTime = 50;

typedef struct{
  const byte pin;
  bool state;
  bool lastState;
  bool pressed;
  bool released;
  unsigned long nextRead;
} 
Button;

Button red = {redPin,false,false,false,false,0};

Button silver = {silverPin,false,false,false,false,0};

Button rotaryEncoder = {rotaryEncoderButtonPin,false,false,false,false,0};

void getButton(Button& button);

bool firstPressed = false;

bool gotRxStruct = false;
bool gotTxStruct = false;

unsigned long nextSleep = 0;
long timer = 5000;

int toNodeID = 0;

RFM69 radio;
Rotary rotary = Rotary(rotaryEncoderPin1, rotaryEncoderPin2);

void wakeUp()
{
  // interrupt handler
}

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
  pinMode(wakeUpPin, INPUT_PULLUP);
  pinMode(red.pin, INPUT_PULLUP);
  pinMode(silver.pin, INPUT_PULLUP);
  pinMode(rotaryEncoder.pin, INPUT_PULLUP);
}

void loop() 
{
  // Allow wake up pin to trigger interrupt on low.
  radio.sleep();
  digitalWrite(rotaryEncoderPin1, LOW);
  digitalWrite(rotaryEncoderPin2, LOW);
  firstPressed = false;

  delay(5);

  attachInterrupt(1, wakeUp, LOW);

  // Enter power down state with ADC and BOD module disabled.
  // Wake up when wake up pin is low.
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 

  // Disable external pin interrupt on wake up pin.
  detachInterrupt(1); 

  // Do something here
  // Example: Read sensor, data logging, data transmission.

  nextSleep = millis() + timer;

  digitalWrite(rotaryEncoderPin1, HIGH);
  digitalWrite(rotaryEncoderPin2, HIGH);

  Serial.println("Wakeup");
  while(nextSleep > millis())
  {
    getInputs();
    processInputs();
//  receiveData();
    sendData();
//  processStruct();
  }
  Serial.println("Sleep!");
  delay(5);
}

void getButton(Button& button)
{
  if(!digitalRead(button.pin))
    button.state = true;
  else
    button.state = false;
  if (button.state != button.lastState && millis() > button.nextRead) 
  {
    button.nextRead = millis() + debounceTime;
    if (button.state == true)
    {
      if(!firstPressed)
        button.pressed = true;
      else
        firstPressed = false;
    }
    else if(button.state == false)
      button.released = true;
    button.lastState = button.state;
  }
}

void getInputs(void)
{
  getButton(red);
  getButton(silver);
  getButton(rotaryEncoder);
  rotaryResult = rotary.process();
}

void processInputs(void)
{
  if(red.pressed)
  {
    nextSleep = millis() + timer;
    sendStruct(2,1,1,0,0);
    //Serial.println("Red");
    red.pressed = false; 
  }

  if(silver.pressed)
  {
    nextSleep = millis() + timer;
    sendStruct(2,2,1,0,0);
    //Serial.println("Silver");
    silver.pressed = false; 
  }

  if(rotaryEncoder.pressed)
  {
    nextSleep = millis() + timer;
    sendStruct(2,3,1,0,0);
    //Serial.println("Button");
    rotaryEncoder.pressed = false;
  }

  if (rotaryResult) {
    if(rotaryResult == DIR_CW)
    {
      nextSleep = millis() + timer;
      sendStruct(2,4,1,0,0);
      //Serial.println("Right");
    }
    else
    {
      nextSleep = millis() + timer;
      sendStruct(2,4,0,0,0);
      //Serial.println("Left");
    }
  }
}

void sendStruct(int toNode, int function, int value1, int value2, int value3)
{
  toNodeID = toNode;
  txStruct.function = function;
  txStruct.value1 = value1;
  txStruct.value2 = value2;
  txStruct.value3 = value3;
  gotTxStruct = true;
}

void sendData()
{
  if(gotTxStruct)
  {
    txStruct.voltage = 0;
    txStruct.temperature = radio.readTemperature(-1);
    radio.send(toNodeID, (const void*)(&txStruct), sizeof(txStruct));
    //  if(radio.sendWithRetry(toNodeID, (const void*)(&txStruct), sizeof(txStruct)), retries, retryWaitTime)
    //    delay(5);
    //  else
    //    delay(5);
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

void processStruct(void)
{
  if(gotRxStruct)
  {
    switch(rxStruct.function)
    {    
    default: // ping back
      sendStruct(rxStruct.fromNodeID, rxStruct.function, rxStruct.value1, rxStruct.value2, rxStruct.value3);
      break;
    }
    gotRxStruct = false;
  }
}


















