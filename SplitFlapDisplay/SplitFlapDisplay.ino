// Split Flap Display
// Morgan Manly
// 16/02/2025

// Enter SplitFlapDisplay.cpp to alter number of modules, and set addresses
// Enjoy :)

#include "Arduino.h" 
#include "SplitFlapDisplay.h"
#include <Preferences.h>

#define COL0_ADDR 0x20

SplitFlapDisplay  display; //Create Display Object
uint8_t           rowAddr;
uint8_t           mode;
char              cmdInput[16];
uint16_t          cmdId;
uint16_t          rowStatus;
bool              cmdReady;

//Tuning offsets (55 steps per character) +ve values move backwards on the character drum, shift flaps upwards
const int         moduleOffset_inits[MAX_MODULES]  = {0, -31, -5, 0 ,0, 0, 0,0}; // Default values for tuning offsets
int               moduleOffsets[MAX_MODULES];    

Preferences prefStore;


void randomTest(void);
void singleInputMode(void);


//  Command Interface Callbacks

//-----------------
void onRequest() {
//-----------------
  uint16_t rowStatus = 0xc000 | cmdId;

  Wire1.write(rowStatus >> 8);
  Wire1.write(rowStatus & 0xff);
  //Serial.printf("onRequest, sent %d\n", cmdId);
}


//-------------------------
void onReceive(int len) {
//-------------------------
  Serial.printf("onReceive[%d]: ", len);
  int p = 0;

  while (Wire1.available()) {
    cmdInput[p] = Wire1.read();
    p = (p > 15) ? 15 : p+1;
  }
  cmdInput[p] = '\0';
  Serial.printf("%s\n", cmdInput);
  cmdReady = true;
  cmdId++;
}


//-----------------------------------------------------------------
void setup() {
//-----------------------------------------------------------------
  uint8_t n, temp[8];
  
  Serial.begin(115200);
  delay(1000);
  Serial.println("Hi.");
  delay(1000);

  // Load offsets stored in flash, or initialize if none.
  prefStore.begin("myPrefs",false);

  if (!prefStore.isKey("offset_2")) {
    Serial.printf("initializing o = %d %d %d\n", moduleOffset_inits[2],moduleOffset_inits[1],moduleOffset_inits[0]);
    prefStore.putInt("offset_7", moduleOffset_inits[7]);
    prefStore.putInt("offset_6", moduleOffset_inits[6]);
    prefStore.putInt("offset_5", moduleOffset_inits[5]);
    prefStore.putInt("offset_4", moduleOffset_inits[4]);
    prefStore.putInt("offset_3", moduleOffset_inits[3]);
    prefStore.putInt("offset_2", moduleOffset_inits[2]);
    prefStore.putInt("offset_1", moduleOffset_inits[1]);
    prefStore.putInt("offset_0", moduleOffset_inits[0]);
  }

  moduleOffsets[7] = prefStore.getInt("offset_7");
  moduleOffsets[6] = prefStore.getInt("offset_6");
  moduleOffsets[5] = prefStore.getInt("offset_5");
  moduleOffsets[4] = prefStore.getInt("offset_4");
  moduleOffsets[3] = prefStore.getInt("offset_3");
  moduleOffsets[2] = prefStore.getInt("offset_2");
  moduleOffsets[1] = prefStore.getInt("offset_1");
  moduleOffsets[0] = prefStore.getInt("offset_0");
  
  Serial.printf("Loading o = %d %d %d\n", moduleOffsets[2],moduleOffsets[1],moduleOffsets[0]);
  prefStore.end();

  // Init flap interface on Wire and read RowId from Col0 pins
  rowAddr = 0x99;
  Wire.begin(8, 9);
  n = Wire.requestFrom(COL0_ADDR, 2);

  if (Wire.available() == 2) {
    uint16_t inputState = 0;
    
    // Read the two bytes and combine them into a 16-bit value, and invert so active sensors are '1'
    inputState = Wire.read();            // Read the lower byte
    inputState |= (Wire.read() << 8);    // Read the upper byte and shift it left

    Serial.printf("Read[%d] %x\n", n, inputState);
    rowAddr = 0x50 + ((inputState >> 8) & 0x07); 
  }
  Serial.printf("Row Id = %x\n", rowAddr);

  // Init Command Interface on Wire1
  Wire1.onReceive(onReceive);
  Wire1.onRequest(onRequest);
  Wire1.begin(rowAddr,10,11,140000);
  cmdInput[0] = '\0';
  cmdId       = 0;
  cmdReady    = false;
  rowStatus   = 0;

  // Init Display Row
  Serial.println("Init Display");
  display.init(); //Initialise Display, and All Modules Within
  display.homeToString("");
  display.writeString("OK");
  delay(500);
  display.writeString(""); 

  mode = 5; 
}


//-----------------------------------------------------------------
void loop() {
//-----------------------------------------------------------------
  rowStatus = cmdId;

  switch (mode) {
    case 0: {                    break; }  // Quiet
    case 1: { singleInputMode(); break; }
    case 4: { cmdTest();         break; } 
    case 5: { randomTest();      break; } //Random Test Mode
    default: break;
  } 

  yield();
}

void cmdTest() {
  //uint8_t  n = Wire.requestFrom(COL0_ADDR, 8);
  delay(500);
  while (Wire.available()) {
    Serial.write(Wire.read());
  }
  Serial.println();


  delay(1000);
}

void singleInputMode(){
  if (cmdReady) {
    //cmdInput[5] = '\0';
    Serial.printf("Single Input %s\n", cmdInput);
    display.writeString(cmdInput, MAX_RPM, false);
    cmdReady = false;
  }
}


void randomTest(){
  display.testRandom();
  delay(4000);
}




//          __
// (QUACK)>(o )___
//          ( ._> /
//           `---'
