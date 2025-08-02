#include "Wire.h"
#include <multiModule.h>
#include <Preferences.h>

#define I2C_DEV_ADDR 0x27
const uint32_t PAUSE_TIME       = 1000;
const uint8_t  moduleAddress    = 0x27; 
const int      moduleOffset_inits[]  = {0,-31,-5}; // Default values for tuning offsets
const int      stepsPerRotation = 2048;            //number of steps per rotation
const int      magnetPosition   = 650;             //position of character drum when the magnet is detected, this sets position 0 to be the blank flap

int            moduleOffsets[N_DIGITS];    //Tuning offsets (55 steps per character) +ve values move backwards on the character drum, shift flaps upwards
uint32_t i = 0;
uint16_t rowStatus[0];
uint8_t     row;
MultiModule module;
long        last_ms;
uint16_t    demoTimer;

Preferences prefStore;


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
  }

  moduleOffsets[2] = prefStore.getInt("offset_2");
  moduleOffsets[1] = prefStore.getInt("offset_1");
  moduleOffsets[0] = prefStore.getInt("offset_0");
  
  Serial.printf("Loading o = %d %d %d\n", moduleOffsets[2],moduleOffsets[1],moduleOffsets[0]);
  prefStore.end();

  // Initialize each SplitFlapModule object with the correct parameters
  module = MultiModule(moduleAddress, stepsPerRotation, moduleOffsets, magnetPosition, N_DIGITS);

  module.init();
  Serial.print("Hi.\n");
  row = 0;
  demoTimer = 5000;
}



//--------------------------------------
void loop() {
//--------------------------------------
  uint8_t addr, n, error;
  addr = I2C_DEV_ADDR;
  long current_ms;


  //Read 2 bytes from the slave
  if(false) { 
    n = Wire.requestFrom(addr, 2);
    Serial.printf("Read %2x returned %2x\n", addr, n);
    while (Wire.available()) {
      rowStatus[row]  = Wire.read() << 8; 
      rowStatus[row] |= Wire.read();     
      Serial.printf("Row %x Status = %04x\n", addr, rowStatus[row]);
    }
    delay(PAUSE_TIME);
  }

  //Write message to the slave
  if(false) {
    Wire.beginTransmission(addr);
    Wire.printf("Roxane %d", i++);
    error = Wire.endTransmission(true);
    Serial.printf("Sent Roxane%d, rc=%u\n", i-1, error);
    delay(PAUSE_TIME);

    Wire.beginTransmission(addr);
    Wire.printf("Justin %d", i++);
    error = Wire.endTransmission(true);
    Serial.printf("Sent Justin%d, rc=%u\n", i-1, error);
    delay(PAUSE_TIME);
  }
  
  //  Serial command interpreter (for adjusting offsets)
  if (Serial.available() ) {

  }

  
  //  Run per-Millisecond tasks
  current_ms = millis();
  if (current_ms != last_ms) {

    // Module runtime
    module.tick();

    //--------[ Display Demo ]--------
    if (demoTimer) 
       demoTimer --;
    else {
      char c = random(0,9);
      c += '0';
      Serial.printf("Demo %c...\n", c);
      module.setTarget(0, c);
      module.setTarget(1, c);
      module.setTarget(2, c);
      demoTimer = 8000;
    }
    last_ms = current_ms;
  } 


}
