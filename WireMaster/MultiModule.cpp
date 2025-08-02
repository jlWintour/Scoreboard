//----------------------------------------------------------------------------------------------
// File: MultiModule_h.cpp 
// Description: Iimplementation file for for the diaply module version that runs 3 motors and 3 Hall 
//              Effect sensors from the same PCF8575 board.
//

//\Arduino\libraries\PCF8575_library\PCF8575_library.h
#include "MultiModule.h"
#include <cctype> // Required for tolower()

#define MAX_RPM 15.0f

const uint16_t allStopState = 0b0000000011100001;

// Array of characters, in order, the first item is located on the magnet on the character drum
const char MultiModule::chars[37] = { ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 
                                          'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
                                        };

const int MultiModule::numChars = sizeof(MultiModule::chars) / sizeof(MultiModule::chars[0]);
      int MultiModule::charPositions[37]; // Written by init function

const uint8_t motorStates[] = {0b0011, 0b1001, 0b1100, 0b0110};

// Not used but useful to have
//
// Bits of PCF8575 data word for motor pins {in1, in2, in3, in4}, for each digit
// const int MultiModule::motorBits[2:0][4:1] = { 0: {11, 10,  9,  8},  //   1's
//                                                1: {15, 14, 13, 12},  //  10's 
//                                                2: { 4,  3,  2,  1}   // 100's
//                                              }; 

// Input pins for Hall effect sensors
// const int MultiModule::sensorBits[2:0] = {5, 6, 7}; 

//Default Constructor
MultiModule::MultiModule()
    : address(0), position(0), stepNumber(0), stepsPerRotation(0), checkInterval(20) { //default values
  for (int i=0; i<N_DIGITS; i++) magnetPosition[i] = 710; //sort of guessing
}

// Constructor implementation
MultiModule::MultiModule(uint8_t I2Caddress, int stepsPerFullRotation, const int stepOffset[], int magnetPos, int nDigits)
    : address(I2Caddress), position(0), stepNumber(0), stepsPerRotation(stepsPerFullRotation), checkInterval(20), numDigits(nDigits) {
  for (int i=0; i<nDigits; i++) {
    magnetPosition[i]  = magnetPos + stepOffset[i];
    stopped[i]         = true;
    magnetSeen[i]      = false;
    targetPositions[i] = 0;
  }

  float stepsPerSecond = (MAX_RPM/60.) * stepsPerRotation;
  // Round step interval to nearest ms
  stepInterval = int( (1000. + stepsPerSecond-1.)/stepsPerSecond);
  // Serial.printf("stepsPerSecond %f, stepInterval %d ms\n", stepsPerSecond, stepInterval);
  delay(1000);
}


//--------------------------------------------------
void MultiModule::setTarget(uint8_t digit, char c) {
//--------------------------------------------------
  // Set one digit to display a character
  uint16_t tgt;

  //Constrain to avoid errors with incorrect inputs
  tgt = constrain(getCharPosition(c), 0, stepsPerRotation-1);
  targetPositions[digit] = tgt;
  //Serial.printf("Tgt[%d] to %d\n", digit, tgt);
  start(digit);
}


//--------------------------------------------------
void MultiModule::writeStates() {
//--------------------------------------------------
  // Write motor states to I/O
  uint16_t data =  allStopState;

  data |= (stopped[0]) ? 0b0000 : motorStates[stepNumber[0]] <<  8;
  data |= (stopped[1]) ? 0b0000 : motorStates[stepNumber[1]] << 12;
  data |= (stopped[2]) ? 0b0000 : motorStates[stepNumber[2]] <<  1;

  writeIO(data);
}


// Write data to I/O
void MultiModule::writeIO(uint16_t data) {
  if (data != lastWrite) {
//    Serial.printf(" %04x : %04x, s=%x%x%x\n", lastWrite, data, stopped[2], stopped[1], stopped[0]);

    Wire.beginTransmission(address);
    Wire.write(data & 0xFF);       // Send lower byte
    Wire.write((data >> 8) & 0xFF); // Send upper byte

    byte error = Wire.endTransmission();
    if (error == 0) {
      // Serial.println("Data written successfully.");
    } else {
      Serial.print("Error writing data, error code: ");
      Serial.println(error);  // Error codes:
                              // 0 = success
                              // 1 = data too long to fit in transmit buffer
                              // 2 = received NACK on transmit of address
                              // 3 = received NACK on transmit of data
                              // 4 = other error
    }
  }
  lastWrite = data;
}


//-------------------------------------------
void MultiModule::tick() {
//-------------------------------------------
  // Scheduler for stepping and sensor reads
  uint8_t sense;
  bool    f;

  // Check step timer and process steps if timer is expired.
  if (stepTimer) {
    stepTimer--;
  } 
  else {
    for (int d=0; d<numDigits; d++) {
      if (position[d] == targetPositions[d]) {
        stop(d);
      }
      else {
        step(d);
        //Serial.printf("step[%d] p=%04d t=%04d\n", d, position[d], targetPositions[d]);
      }
    }
    writeStates();
    stepTimer = stepInterval-1;
  }

  // Check sensor timer and process sensor read if timer is expired
  if (checkTimer) {
    checkTimer--;
  } 
  else {
    sense = readSensors();
    //Serial.printf("Sensors = %02d, %d\n", sense, checkInterval);
    for (uint8_t i=0; i<numDigits; i++) {
      // Act only on falling edge of sensor, for running digits. Sensor is active low.
      //Serial.printf("digit %d stopped=%x\n", i, stopped[i]);
      f = (sense >> i) & 1;

      if (f && !magnetSeen[i]) {
        magnetDetected(i);
        Serial.printf("Magnet %d seen\n", i);
      }

      if (!f && magnetSeen[i]) {
        ; //Serial.printf("Magnet %d gone\n", i);
      }
      magnetSeen[i] = f;
    }
    checkTimer = checkInterval-1;
  }
}


//--------------------------------------------------
void MultiModule::init() {
//--------------------------------------------------
  //Init Module, Setup IO Board
  int initDelay = 100;

  writeIO(allStopState);
  delay(initDelay);

  for (int i=0; i<numDigits; i++) {
    for (uint8_t digit=0; digit<numDigits; digit++) step(digit); 
    writeStates();
    delay(initDelay);
  }
  
  for (uint8_t digit=0; digit<numDigits; digit++) stop(digit); 
  writeStates();

  //Generate Character Position Array
  float stepSize = (float)stepsPerRotation / (float)numChars;
  float currentPosition = 0;
  for (int i = 0; i < numChars; i++) {
      charPositions[i] = (int)currentPosition;
      currentPosition += stepSize;
  }
}


int MultiModule::getCharPosition(char inputChar) {
    inputChar = toupper(inputChar);
    for (int i = 0; i < numChars; i++) {
        if (chars[i] == inputChar) {
            return charPositions[i];
        }
    }
    return 0;  // Character not found, return blank
}

void MultiModule::stop(uint8_t digit) {
  //if (!stopped[digit]) Serial.printf("Stop[%d] at %04d\n", digit, position[digit]);
  stopped[digit] = true;
}   

void MultiModule::start(uint8_t digit) {
  //if (stopped[digit]) Serial.printf("Start[%d]\n", digit);
  stepNumber[digit] = (stepNumber[digit] + 3) % 4; //effectively take one off stepNumber
  stopped[digit]    = false;
}

void MultiModule::step(uint8_t digit) {
  if (stopped[digit]) start(digit);
  position[digit]   = (position[digit]   + 1) % stepsPerRotation;
  stepNumber[digit] = (stepNumber[digit] + 1) % 4;
}


uint8_t MultiModule::readSensors(){
  // Read sensors
  Wire.requestFrom(address, 2);
  // Make sure the data is available
  if (Wire.available() == 2) {
    uint16_t inputState = 0;
    uint16_t flags = 0;
    
    // Read the two bytes and combine them into a 16-bit value, and invert so active sensors are '1'
    inputState = Wire.read();            // Read the lower byte
    inputState |= (Wire.read() << 8);    // Read the upper byte and shift it left
    inputState = ~inputState;
    //Serial.printf("readSensors got %4x\n", inputState );

    // make a set of per-digit active-high flags from the active-low pin values
    flags =  (inputState >> 7) & 0b001   // Digit 0 from bit 7 to bit 0
           | (inputState >> 5) & 0b010   // Digit 1 from bit 6 to bit 1
           | (inputState >> 3) & 0b100;  // Digit 2 from bit 5 to bit 2

    //Serial.printf("flags = %x\n", flags);
    return uint8_t(flags);
  }
}




