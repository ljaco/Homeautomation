///////////////////////////////////////////////////////////////////////////////////
// (c) Lucas Jacomet
// 09.01.2015
// RFM69HW Debug Node
// 3.3V Arduino pro Mini

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
//  INT1   ~D3: 
//          D4: 
//         ~D5: 
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

#define NODEID    88
#define NETWORKID 88
#define ENCRYPTKEY  "sampleEncryptKey"
bool highPower = true;
int powerLevel = 8;
bool promiscuousMode = false;
unsigned int retries = 5;
unsigned int retryWaitTime = 500;

typedef struct {
  int     fromNodeID;
  int     function;
  int     value1;
  //int     value2;
  //int     value3;
  byte     temperature;
}
Payload; //7 bytes

Payload txStruct;
Payload rxStruct;

int toNodeID = 0;
bool gotRxData = false;
bool gotRxStruct = false;
bool gotTxStruct = false;


bool isPayload = false;
unsigned int packetCount = 0;
unsigned int senderID = 0;
unsigned int targetID = 0;
int rssi = 0;
unsigned int dataLength = 0;
bool ackReq = false;

RFM69 radio;
//s,3,8,0      s,3,8,2      s,3,8,2      s,3,8,2      s,3,8,0  
void setup()
{
  Serial.begin(57600);
  Serial.setTimeout(2);
  delay(10);
  Serial.println("RFM69HW Debug Node");
  radio.initialize(RF69_433MHZ,NODEID,NETWORKID);
  radio.setHighPower(highPower);
  radio.setPowerLevel(powerLevel);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  txStruct.fromNodeID = NODEID;
  Serial.println("RFM69HW Debug Node");
  Serial.println("Available commands:");
  Serial.println("R = read a register: R,number (in DEC)");
  Serial.println("r = read all register values");
  Serial.println("E = enable encryption, default");
  Serial.println("e = disable encryption");
  Serial.println("M = turn promiscuous mode on");
  Serial.println("m = turn promiscuous mode off, default");
  Serial.println("t = get radio temperature");
  Serial.println("f = get radio frequency, default 433000000 Hz");
  Serial.println("p = set power level: p,powerLevel, default 8");
  Serial.println("H = turn high power on, default");
  Serial.println("h = turn high power off");
  Serial.println("s = send struct to node: s,node,function,value1");
  Serial.println();
}

void loop() 
{
  processSerial();
  sendData();
  receiveData();
  displayData();
}

void processSerial(void)
{
  if (Serial.available() > 0)
  {
    char input = Serial.read();
    switch(input)
    {
    case 'R':
      delay(1);
      if (Serial.available()>0)
      {
        unsigned int reg = Serial.parseInt();
        unsigned int val = radio.readReg(reg);
        Serial.print("Register ");
        Serial.print(reg, HEX);
        Serial.print(" - ");
        Serial.print(val,HEX);
        Serial.print(" - ");
        Serial.println(val,BIN);
      }
      else
        Serial.println("No register entered!");
      break;

    case 'r': // r = read all register values
      radio.readAllRegs();
      break;

    case 'E': // E = enable encryption
      radio.encrypt(ENCRYPTKEY);
      Serial.print("Encryption on with key: ");
      Serial.println(ENCRYPTKEY);
      break;

    case 'e': // e = disable encryption
      radio.encrypt(null);
      Serial.println("Encryption off");
      break;

    case 'M':
      promiscuousMode = true;
      radio.promiscuous(promiscuousMode);
      Serial.println("Promiscuous mode on");
      break;

    case 'm':
      promiscuousMode = false;
      radio.promiscuous(promiscuousMode);
      Serial.println("Promiscuous mode off");
      break;

    case 't':
      Serial.print("Radio Temp is ");
      Serial.print(radio.readTemperature(-1));
      Serial.println("C");
      break;

    case 'f':
      Serial.print("Radio frequency is ");
      Serial.print(radio.getFrequency());
      Serial.println(" Hz");
      break;

    case 'p':
      delay(1);
      if (Serial.available()>0)
        powerLevel = Serial.parseInt();
      else
        Serial.println("No power level entered!");
      if(powerLevel < 0)
        powerLevel = 0;
      else if(powerLevel > 15)
        powerLevel = 15;
      Serial.print("Set power level to ");
      Serial.println(powerLevel);
      radio.setPowerLevel(powerLevel);
      break;

    case 'H':
      highPower = true;
      radio.setHighPower(highPower);
      Serial.println("High power is on");
      break;

    case 'h':
      highPower = false;
      radio.setHighPower(highPower);
      Serial.println("High power is off");
      break;

    case 's':
      delay(1);
      if (Serial.available()>4)
      {
        delay(2);
        toNodeID = Serial.parseInt();
        txStruct.function = Serial.parseInt();
        txStruct.value1 = Serial.parseInt();
        gotTxStruct = true;
      }
      break;

    default:
      Serial.print(input);
      Serial.println(" is not a valid command!");
      break;
    }
  }
}

//void receiveData(void)
//{
//  if (radio.receiveDone())
//  {
//    ++packetCount;
//    Serial.print("#[");
//    if(packetCount < 10000)
//      Serial.print('0');
//    if (packetCount < 1000)
//      Serial.print('0');
//    if (packetCount < 100)
//      Serial.print('0');
//    if (packetCount < 10)
//      Serial.print('0');
//    Serial.print(packetCount);
//    Serial.print(']');
//    Serial.print(" [");
//    Serial.print(radio.SENDERID, DEC);
//    Serial.print(']');
//    if (promiscuousMode)
//    {
//      Serial.print("-p->[");
//      Serial.print(radio.TARGETID, DEC);
//      Serial.print(']');
//    }
//    else
//    {
//      Serial.print("--->[");
//      Serial.print(NODEID, DEC);
//      Serial.print(']'); 
//    }
//    int rssi = radio.readRSSI();
//    if(rssi > -100)
//      Serial.print(" [RX_RSSI: ");
//    else
//      Serial.print(" [RX_RSSI:");
//    Serial.print(rssi);
//    Serial.print(']');
//    int dataLength = radio.DATALEN;
//    rxStruct = *(Payload*)radio.DATA;
//    if (dataLength == sizeof(Payload) && rxStruct.fromNodeID > 0 && rxStruct.fromNodeID < 128)
//    {
//      Serial.print(" Struct (");
//      Serial.print(dataLength);
//      Serial.print(" bytes)  [");
//      Serial.print("fromNodeID = ");
//      Serial.print(rxStruct.fromNodeID);
//      Serial.print(" function = ");
//      Serial.print(rxStruct.function);
//      Serial.print(" value1 = ");
//      Serial.print(rxStruct.value1);
//      Serial.print(" temperature = ");
//      Serial.print(rxStruct.temperature);
//      Serial.print(']');
//    }
//    else
//    {
//      Serial.print(" No Struct (");
//      Serial.print(dataLength);
//      Serial.print(" bytes)  [");
//      for (byte i = 0; i < dataLength; i++)
//        Serial.print((char)radio.DATA[i]);
//      Serial.print(']');
//    }
//
//    if (radio.ACKRequested())
//    {
//      radio.sendACK();
//      Serial.print(" - ACK sent.");
//    }
//    Serial.println();
//  }
//}

void receiveData(void)
{
  if (radio.receiveDone())
  {
    senderID = radio.SENDERID;
    if (promiscuousMode)
      targetID = radio.TARGETID;
    rssi = radio.readRSSI();
    dataLength = radio.DATALEN;
    if (dataLength == sizeof(Payload))
    {
      rxStruct = *(Payload*)radio.DATA;
      if(rxStruct.fromNodeID > 0 && rxStruct.fromNodeID < 128)
        gotRxStruct = true;
      else
        gotRxStruct = false;
    }
    else
      gotRxStruct = false;
    if (radio.ACKRequested())
    {
      radio.sendACK();
      ackReq = true;
    }
    else
      ackReq = false;
    gotRxData = true;
  }
}

void displayData(void)
{
  if(gotRxData)
  {
    ++packetCount;
    Serial.print("#[");
    if(packetCount < 10000)
      Serial.print('0');
    if (packetCount < 1000)
      Serial.print('0');
    if (packetCount < 100)
      Serial.print('0');
    if (packetCount < 10)
      Serial.print('0');
    Serial.print(packetCount);
    Serial.print(']');
    Serial.print(" [");
    Serial.print(senderID);
    Serial.print(']');
    if (promiscuousMode)
    {
      Serial.print("-p->[");
      Serial.print(targetID);
      Serial.print(']');
    }
    else
    {
      Serial.print("--->[");
      Serial.print(NODEID, DEC);
      Serial.print(']'); 
    }
    if(rssi > -100)
      Serial.print(" [RX_RSSI: ");
    else
      Serial.print(" [RX_RSSI:");
    Serial.print(rssi);
    Serial.print(']');
    if (gotRxStruct)
    {
      Serial.print(" Struct (");
      Serial.print(dataLength);
      Serial.print(" bytes)  [");
      Serial.print("fromNodeID = ");
      Serial.print(rxStruct.fromNodeID);
      Serial.print(" function = ");
      Serial.print(rxStruct.function);
      Serial.print(" value1 = ");
      Serial.print(rxStruct.value1);
      Serial.print(" temperature = ");
      Serial.print(rxStruct.temperature);
      Serial.print(']');
    }
    else
    {
      Serial.print(" No Struct (");
      Serial.print(dataLength);
      Serial.print(" bytes)  [");
      for (byte i = 0; i < dataLength; i++)
        Serial.print(radio.DATA[i]);
      Serial.print(']');
    }
    if(ackReq)
      Serial.print(" - ACK sent.");
    else
      Serial.print(" - No ACK req.");
    Serial.println();
    gotRxData = false;
  }
}

void sendData()
{
  if(gotTxStruct)
  {
    txStruct.temperature = radio.readTemperature(-1);
    Serial.print(" Sending");
    Serial.print(" [");
    Serial.print(NODEID);
    Serial.print(']');
    Serial.print("--->[");
    Serial.print(toNodeID);
    Serial.print(']'); 
    Serial.print(" Struct (");
    Serial.print(sizeof(txStruct));
    Serial.print(" bytes) with [function = ");
    Serial.print(txStruct.function);
    Serial.print(" value1 = ");
    Serial.print(txStruct.value1);
    Serial.print(" temperature = ");
    Serial.print(txStruct.temperature);
    Serial.print("] ");
    radio.send(toNodeID, (const void*)(&txStruct), sizeof(txStruct));
    //    if (radio.sendWithRetry(toNodeID, (const void*)(&txStruct), sizeof(txStruct)), retries, retryWaitTime)
    //      Serial.print("- got ACK!");
    //    else
    //      Serial.print("- no ACK!");
    Serial.println();
    delay(1);
    gotTxStruct = false;
  }
}













