//---------------------------------------------------------------------------------------------------
//
//        File: Score_counter
//    Platform: WaveShare ESP32-S3 Zero
// Description: Similart to the split-flap display, but controls 3 digits via a single I/O Expander 
//              module.
//
//

#include "Wire.h"
#include <Score_counter.h>
#include <Preferences.h>

#define I2C_DEV_ADDR 0x27

const uint32_t PAUSE_TIME       = 1000;
const uint8_t  moduleAddress    = 0x20; 
const int      moduleOffset_inits[]  = {0,-31,-5}; // Default values for tuning offsets
const int      stepsPerRotation = 6144;            //number of steps per rotation
const int      magnetPosition   = 650;             //position of character drum when the magnet is detected, this sets position 0 to be the blank flap
char           test_value; 
int            moduleOffsets[N_DIGITS];    //Tuning offsets (55 steps per character) +ve values move backwards on the character drum, shift flaps upwards
uint16_t       row_id;
uint16_t       rowStatus;
MultiModule    module;
long           last_ms;
long           last_us;
uint16_t       demoTimer;
uint8_t        mode;
char           cmdInput[16];
uint16_t       cmdId;
bool           cmdReady;
   
Preferences    prefStore;


//  Command Interface Callbacks

//-----------------
void onRequest() {
//-----------------
  uint16_t rowStatus = 0xc000 | cmdId;

  Wire1.write(rowStatus >> 8);
  Wire1.write(rowStatus & 0xff);
  Serial.printf("onRequest, sent %d\n", cmdId);
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



//--------------------------------------
void setup() {
//--------------------------------------
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Wire.begin(8, 9);
  delay(1000);

  // Load offsets stored in flash, or initialize if none.
  prefStore.begin("myPrefs",false);

  if (!prefStore.isKey("offset_2")) {
    Serial.printf("initializing o = %d %d %d\n", moduleOffset_inits[2],moduleOffset_inits[1],moduleOffset_inits[0]);
    prefStore.putInt("offset_2", moduleOffset_inits[2]);
    prefStore.putInt("offset_1", moduleOffset_inits[1]);
    prefStore.putInt("offset_0", moduleOffset_inits[0]);
    prefStore.putInt("RowId",    0x50);
  }

  moduleOffsets[2] = prefStore.getInt("offset_2");
  moduleOffsets[1] = prefStore.getInt("offset_1");
  moduleOffsets[0] = prefStore.getInt("offset_0");
  row_id           = prefStore.getInt("RowId");
  
  Serial.printf("Loading RowId %d, ofs[] = %d %d %d\n", row_id, moduleOffsets[2],moduleOffsets[1],moduleOffsets[0]);
  prefStore.end();

  // Initialize each SplitFlapModule object with the correct parameters
  module = MultiModule(moduleAddress, stepsPerRotation, moduleOffsets, magnetPosition, N_DIGITS);
  module.init();

  // Init Command Interface on Wire1
  Wire1.onReceive(onReceive);
  Wire1.onRequest(onRequest);
  Wire1.begin(row_id,10,11,140000);
  cmdInput[0] = '\0';
  cmdId       = 0;
  cmdReady    = false;
  rowStatus   = 0;

  Serial.print("Hi.\n");
  Serial.println( sizeof(int));
  mode       = 0;
  test_value = '0';
  demoTimer = 5000;
     
  module.start(2);

}



//--------------------------------------
void loop() {
//--------------------------------------
  uint8_t addr, n, error;
  addr = I2C_DEV_ADDR;
  long current_ms, current_us;

  //  Serial command interpreter (for adjusting offsets)
  if (Serial.available() ) {

  }

  // Check Command interface on Wire1
  rowStatus = cmdId;

  // Run 1 us tasks
  current_us =  micros();
  //if (current_us != last_us) {
    //module.tick();
    module.step(2); 
    module.writeStates();
    delayMicroseconds(650);
    last_us = current_us;
  //}

  //  Run 1 ms tasks
  current_ms = millis();
  if (false) { // current_ms != last_ms) {


    //--------[ Display Demo ]--------
    if (demoTimer) 
       demoTimer --;
    else {

      switch(mode) {
        case 0: {                    break; }  // Quiet
        case 1: { singleInputMode(); break; }
        case 2: { alternate();       break; }
        case 4: { cmdTest();         break; } 
        case 5: { randomTest();      break; }  //Random Test Mode
        default: break;
      }

      demoTimer = 000;
    }
    last_ms = current_ms;
  } 
}


//-------------------------------------------------------------------------
void alternate() {
//-------------------------------------------------------------------------
  //uint8_t  n = Wire.requestFrom(COL0_ADDR, 8);
  Serial.printf("Alternate %d...\n", test_value);
  module.setTarget(2, test_value);
  test_value = (test_value=='0') ? '5' : '0';
}


//-------------------------------------------------------------------------
void cmdTest() {
//-------------------------------------------------------------------------
  //uint8_t  n = Wire.requestFrom(COL0_ADDR, 8);
  while (Wire.available()) {
    Serial.write(Wire.read());
  }
  Serial.println();
  delay(1000);
}


//-------------------------------------------------------------------------
void singleInputMode() {
//-------------------------------------------------------------------------
  if (!cmdReady) return;

  Serial.printf("Single Input %s\n", cmdInput);
  for (int i=N_DIGITS-1; i>=0; i--)  
    module.setTarget(i, cmdInput[i]);
  cmdReady = false;
}


//-------------------------------------------------------------------
void randomTest(void) {
//-------------------------------------------------------------------
  uint16_t n = random(0,999);
  Serial.printf("Demo %d...\n", n);
  display_value(n);
  delay(4000);
}


//---------------------------------------------------------------------------------
void display_value(uint16_t value) {
//---------------------------------------------------------------------------------
  int   v = value;
  char  c[N_DIGITS+1];

  c[N_DIGITS] = '\0';
  for (int i=N_DIGITS-1; i>=0; i--) {
    c[i] = (v % 10) + '0';
    v /= 10;
    module.setTarget(i, c[i]);
  }
  Serial.printf("Display %d [%s]\n", value, c);
}

