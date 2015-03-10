///////////////////////////////////////////////////////////////////////////////////
// (c) Lucas Jacomet
// 09.01.2015
// RFM69HW Debug Node with SerLCD
// 5V Arduino pro Mini

//      Pinmap:
//          A0:
//          A1:
//          A2: 
//          A3: 
//  SDA     A4: 
//  SCL     A5:
//          A6:
//          A7:
//  RX      D0: FTDI TX for debugging
//  TX      D1: FTDI RX for debugging
//  INT0    D2: DIO0 RFM69HW
//  INT1   ~D3: LCDSerial RX not connected to SerLCD
//          D4: LCDSerial TX SerLCD RX
//         ~D5: Buzzer
//         ~D6: 
//          D7: 
//          D8: 
//         ~D9: 
//  SS    ~D10: NSS RFM69HW
//  MOSI  ~D11: MOSI RFM69HW
//  MISO   D12: MISO RFM69HW
//  SCK    D13: SCK RFM69HW

#include <RFM69.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <SerLCD.h>

#define NODEID      2
#define NETWORKID   88
#define ENCRYPTKEY  "sampleEncryptKey"
bool highPower = true;
int powerLevel = 31;
bool promiscuousMode = false;


typedef struct {
  int     fromNodeID;
  int     function;
  int     value1;
  //int     value2;
  //int     value3;
  byte     temperature;
}
Payload; //7 bytes

Payload txData;
Payload rxData;

int toNodeID = 0;
bool gotRxData = false;
bool gotTxData = false;
unsigned int packetCount = 0;

bool gotBeep = false;
int buzzerPin = 5;

RFM69 radio;

SoftwareSerial LCDSerial(3, 4); //RX TX
SerLCD LCD(LCDSerial, 16, 2);

void setup()
{
  Serial.begin(57600);
  LCDSerial.begin(9600);
  LCD.begin();
  Serial.setTimeout(2);
  delay(10);
  radio.initialize(RF69_433MHZ,NODEID,NETWORKID);
  radio.setHighPower(highPower);
  radio.setPowerLevel(powerLevel);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  txData.fromNodeID = NODEID;
  pinMode(8, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);


    LCD.setPosition(1,0);
    LCD.print("Debug Node ID: ");
    LCD.print(NODEID);
    LCD.setPosition(2,0);
    LCD.print("Waiting for data");
}

void loop() 
{
  if(digitalRead(8) == LOW && powerLevel)
  {
    powerLevel = false;
    radio.setHighPower(powerLevel);
  }
  else if(digitalRead(8) == HIGH && !powerLevel)
  {
    powerLevel = true;
    radio.setHighPower(powerLevel);
  }
  //sendData();
  receiveData();
  if(gotBeep)
    beep(1,2);
}

void receiveData(void)
{
  if (radio.receiveDone())
  {
    //beep(1,2);
    ++packetCount;
    LCD.clear();
    LCD.setPosition(1,0);
    if(powerLevel)
      LCD.print('H');
    else
      LCD.print('L');
    if (packetCount < 100)
      LCD.print('0');
    if (packetCount < 10)
      LCD.print('0');
    LCD.print(packetCount);
    LCD.print(' ');
    LCD.print(radio.SENDERID);
    if (promiscuousMode)
    {
      LCD.print("-p->");
      LCD.print(radio.TARGETID, DEC);
    }
    else
    {
      LCD.print("--->");
      LCD.print(NODEID, DEC);
    }
    int rssi = radio.readRSSI();
    if(rssi > -10)
      LCD.print("   ");
    else if(rssi > -100)
      LCD.print("  ");
    else
      LCD.print(" ");
    LCD.print(rssi);

    LCD.setPosition(2,0);

    int dataLength = radio.DATALEN;
    rxData = *(Payload*)radio.DATA;
    if (dataLength == sizeof(Payload) && rxData.fromNodeID > 0 && rxData.fromNodeID < 128)
    {
      LCD.print("f=");
      LCD.print(rxData.function);
      LCD.print(" v=");
      LCD.print(rxData.value1);
      //LCD.print(" t=");
      //LCD.print(rxData.temperature);
    }
    else
    {
      LCD.print("D:");
      LCD.print(dataLength);
      LCD.print("b|");
      for (byte i = 0; i < 9; i++)
        LCD.print((char)radio.DATA[i]);
    }

    if (radio.ACKRequested())
    {
      radio.sendACK();
      LCD.setPosition(2,14);
      LCD.print(">A");
    }
  }
}


//void receiveData(void)
//{
//  if (radio.receiveDone())
//  {
//    LCD.setPosition(1,0);
//    LCD.print("RSSI:");
//    int rssi = radio.readRSSI();
//    if(rssi > -10)
//      LCD.print("   ");
//    else if(rssi > -100)
//      LCD.print("  ");
//    else
//      LCD.print(" ");
//    LCD.print(rssi);
//    if (radio.ACKRequested())
//    {
//      radio.sendACK();
//      LCD.setPosition(2,0);
//      LCD.print(">A");
//    }
//  }
//}

void sendData()
{
  if(gotTxData)
  {
    txData.temperature = radio.readTemperature(-1);
    Serial.print(" Sending");
    Serial.print(" [");
    Serial.print(NODEID);
    Serial.print(']');
    Serial.print("--->[");
    Serial.print(toNodeID);
    Serial.print(']'); 
    Serial.print(" Struct (");
    Serial.print(sizeof(txData));
    Serial.print(" bytes) with [function = ");
    Serial.print(txData.function);
    Serial.print(" value1 = ");
    Serial.print(txData.value1);
    Serial.print(" temperature = ");
    Serial.print(txData.temperature);
    Serial.print("] ");
    if (radio.sendWithRetry(toNodeID, (const void*)(&txData), sizeof(txData)))
      Serial.print("- got ACK!");
    else
      Serial.print("- no ACK!");
    Serial.println();
    delay(1);
    gotTxData = false;
  }
}

void beep(int count, int time)
{
  for(int i=0; i<count; ++i)
  {
    digitalWrite(buzzerPin, LOW);
    delay(time);
    digitalWrite(buzzerPin, HIGH);
    delay(time);
  }
  gotBeep = false;
}










