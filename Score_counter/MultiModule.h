//--------------------------------------------------------------------------------------
//
//        File: MultiModule.h (header file)
// Description: Definitiions for the diaply module version that runs 3 motors and 3 Hall 
//              Effect sensors from the same PCF8575 board.
//
//
#ifndef MuiltiModule_h
#define MultiModule_h
#include "Arduino.h" 
#include <Wire.h> //I2C Communication Between Modules

#define N_DIGITS 3


class MultiModule {
  public:
    // Constructor declarationS
    MultiModule(); //default constructor required to allocate memory for SplitFlapDisplay class
    MultiModule(uint8_t I2Caddress,int stepsPerFullRotation, const int stepOffset[], int magnetPos, int numDigits);

    void init(); 
    void tick();
    void step(uint8_t digit); //step motor
    void stop(uint8_t digit); //write all motor input pins to low
    void start(uint8_t digit); //re-energize coils to last position, not stepping motor
    void setTarget(uint8_t digit, char c);

    int  getMagnetPosition(uint8_t digit) const { return magnetPosition[digit]; } //defined position of the magnet
    int  getCharPosition(char inputChar); //get integer position given single character
    int  getPosition(uint8_t digit) const { return position[digit]; } //get integer position

    uint8_t readSensors();  //return the states of the sensor in each digit
    void    magnetDetected(uint8_t digit) { position[digit] = magnetPosition[digit]; } //update position to magnetposition, called when magnet is detected
    void writeStates();           // Build and write I/O data from motor states

  private:
    void writeIO(uint16_t data);  // write explicit data to I/O
    
    uint8_t  address;              //i2c address of module
    uint8_t  numDigits;            // Number of character drums
    int      stepsPerRotation;          //number of steps per rotation
    int      checkInterval;            //How often to check each modules hall effect sensor, less than 20ms causes issues with bouncing
    int      stepInterval;             //How often to step motors when active (ms)
    int      magnetPosition[N_DIGITS];  // Modified by offsets

    int      targetPositions[N_DIGITS];
    int      position[N_DIGITS];   //character drum position
    int      stepNumber[N_DIGITS]; //current position in the stepping order, to make motor move
    bool     stopped[N_DIGITS];
    bool     magnetSeen[N_DIGITS];
    uint16_t lastWrite;
    int      stepTimer;
    int      checkTimer;

    static const char chars[10];          // All characters in order
    static int        charPositions[10];  // will be generated based on the characters and the magnetPosition variable
    static const int  numChars;           // Number of characters in module
};

#endif
//          __
// (QUACK)>(o )___
//          ( ._> /
//           `---'
// Morgan Manly
// 28/01/2025

// //PINs on the PCF8575 Board
// #define P00  	0
// #define P01  	1
// #define P02  	2
// #define P03  	3
// #define P04  	4
// #define P05  	5
// #define P06  	6
// #define P07  	7
// #define P10  	8
// #define P11  	9
// #define P12  	10
// #define P13  	11
// #define P14  	12
// #define P15  	13
// #define P16  	14
// #define P17  	15
