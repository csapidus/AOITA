 /////////////////////////////////////////////////////////
// Mahdi Al-Husseini, RPN Arduino Nixie Calculator v6   //
// An Old Inspiration to Arithmetic (AOITA)             //
// 27 September 2019                                    //
//////////////////////////////////////////////////////////

/********************************************************************************************************************************************/
// Useful links: 
// https://www.instructables.com/id/Driving-two-Nixie-tubes-with-an-Arduino-via-a-shif/?amp_page=true
//
/********************************************************************************************************************************************/

//********************* libraries ***********************//
#include <Keypad.h> // matrix keypad
#include <Wire.h>
#include <SPI.h>

// TFT Libraries
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoOblique24pt7b.h>

// LCD Libraries
// #include <LiquidCrystal_I2C.h>
// I2C pins declaration
// LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

//********************* calculator pre p ***********************//
double num1, num2, num3, num4; // number of spaces in the stack, for final product or during addition of 4th while loop, turn to an array
double myStack[4];
long number;
double total, nixie;
char operation, button;
int thousands, hundreds, tens, ones, tenths, hundredths, a, b, c, d, value;
boolean operationsTwo = false;
int stackLocation = 0;
int decimal = 0;
String pointer;

//********************* TFT screen prep ***********************//

// CORRECT FOR ATMEGA1284
#define TFT_DC 3
#define TFT_CS 4

// FOR MEGA TROUBLESHOOTING ONLY
// #define TFT_DC 9
// #define TFT_CS 10

// ATMEGA1284: Use hardware SPI (RST, 5 for MOSI, and 7 for SCK/CLK) and the above for CS/DC
// ARD MEGA: Use hardware SPI (RST, 51 for MOSI, and 52 for SCK/CLK) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

//********************* keypad screen prep ***********************//
const byte ROWS = 4;
const byte COLS = 5;

char keys[ROWS][COLS] = {
  {'U','*', '7', '8', '9'},
  {'D', '/', '4', '5', '6'},
  {'^', '+', '1', '2', '3'}, 
  {'C', '-', '0', '.', 'E'}
};

// byte rowPins[ROWS] = {23, 25, 27, 29}; //connect to the row pinouts of the keypad
byte rowPins[ROWS] = {22, 23, 31, 30}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {25, 26, 27, 28, 29}; //connect to the column pinouts of the keypad

Keypad customKeypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);

//********************* shift-register screen prep ***********************//
// two-digit value to byte translation for shift registers
int charTable[] = {
0,1,2,3,4,5,6,7,8,9,16,17,18,19,20,21,22,23,24,25,
32,33,34,35,36,37,38,39,40,41,48,49,50,51,52,53,54,55,56,57,
64,65,66,67,68,69,70,71,72,73,80,81,82,83,84,85,86,87,88,89,
96,97,98,99,100,101,102,103,104,105,
112,113,114,115,116,117,118,119,120,121,
128,129,130,131,132,133,134,135,136,137,
144,145,146,147,148,149,150,151,152,153};

byte latchPinTHL = 2;
byte dataPinTHL = 1;
byte clockPinTHL = 0;

byte latchPinTO = 12;
byte dataPinTO = 11;
byte clockPinTO = 10;

byte latchPinTHS = 15;
byte dataPinTHS = 14;
byte clockPinTHS = 13;

void setup() {

  // tft initialization
  tft.begin();
  tft.setRotation(3);
  tft.setFont(&FreeMonoOblique24pt7b);
  nixieLoadingScreen();
  
  // initializing output pins
  for (int i = 0 ; i <= 2 ; i++) {
    pinMode(i, OUTPUT);
  }
  for (int i = 10 ; i <= 31 ; i++) {
    pinMode(i, OUTPUT);
  }

  // shift register set-up
  // thousands and hundreds (large) shift-register
  // pinMode(latchPinTHL, OUTPUT);
  // pinMode(dataPinTHL, OUTPUT);
  // pinMode(clockPinTHL, OUTPUT);

  // tens and ones shift-register
  // pinMode(latchPinTO, OUTPUT);
  // pinMode(dataPinTO, OUTPUT);
  // pinMode(clockPinTO, OUTPUT);

  // tenths and hundredths (small) shift-register
  // pinMode(latchPinTHS, OUTPUT);
  // pinMode(dataPinTHS, OUTPUT);
  // pinMode(clockPinTHS, OUTPUT);

  // lcd.begin(16, 2); // initializes the lcd
  // lcd.backlight(); // power on lcd backlight

  nixieIlluminate(0,0,0);
  nixieIlluminate(0,0,1);
  nixieIlluminate(0,0,2);
  
  Serial.begin(9600);
  delay(5000);
  Serial.println("Beginning AOITA");
  
  tft.setFont(&FreeMono12pt7b);
  nixieTFTsetUP();
}

void loop() {
  /* for (int i = 0; i <= 99999; i++) {
    nixieTranslate(i);
    delay(500);
  } */
  while (1) {
    // first loop in which user submits then enters first number
    button = customKeypad.getKey(); // reads button
    stackLocation = 0;
    if (button == 'C') // reset while writing first number
    {
      decimal = 0;
      memset(myStack, 0, sizeof(myStack));
      total = 0;
      operation = 0;
      // lcd.clear(); // clears lcd
      stackLocation = 0;
      nixieTranslate(0.00); // sets all nixies to zero
    }

    if (button == 'U') {
      peekUp();
    }

    if (button == 'D') {
      peekDown();
    }

    if (button == '.') // decimal place
    {
      decimal++;
    }

    if (button >= '0' && button <= '9') // submit numeric value one digit at a time
    {
      if (decimal == 0) {
        myStack[0] = myStack[0] * 10 + (button - '0'); // uses ASCII
      } else {
        switch (decimal) {
          case 1: // tenths
            myStack[0] = myStack[0] + (button - '0') / 10.00;
            decimal++;
            break;
          case 2: // hundredths
            myStack[0] = myStack[0] + (button - '0') / 100.00;
            decimal++;
            break;
          case 3: // nopes
            break;
        }
      }

      // lcd.setCursor(0, 0); // first lcd row
      // lcd.print(myStack[0]); // prints first number on lcd
      nixieTranslate( (double) myStack[0]); // prints first number on nixies
    }

    if (myStack[0] != 0 && (button == 'E')) // enter numeric value, pushes first number onto stack
    {
      decimal = 0;
      break;
    }

  }

  while (1) {
    // second while loop in which user enters second number into stack, then selects an operation. the result of the operation is then pushed onto the stack. additional operations may occur until cleared.
    stackLocation = 1;
    if (button == 'C') {
      break; // resets while selecting an operator
    }
    button = customKeypad.getKey();
    if (button == 'C') // additional reset coverage
    {
      decimal = 0;
      memset(myStack, 0, sizeof(myStack));
      total = 0;
      operation = 0;
      // lcd.clear(); // clears lcd
      stackLocation = 0;
      nixieTranslate(0.00); // sets all nixies to zero
      break;
    }

    if (button == 'U') {
      peekUp();
    }

    if (button == 'D') {
      peekDown();
    }

    if (button == '.') // decimal place
    {
      decimal++;
    }
    if (button >= '0' && button <= '9') // second number
    {
      if (decimal == 0) {
        myStack[1] = myStack[1] * 10 + (button - '0');
      } else {
        switch (decimal) {
          case 1: // tenths
            myStack[1] = myStack[1] + (button - '0') / 10.00;
            decimal++;
            break;
          case 2: // hundredths
            myStack[1] = myStack[1] + (button - '0') / 100.00;
            decimal++;
            break;
          case 3: // nopes
            break;
        }
      }
      // lcd.clear();
      // lcd.setCursor(0, 0); // first lcd row
      // lcd.print(myStack[1]); // prints second number on lcd
      nixieTranslate( (double) myStack[1]); // prints second number on nixies
    }

    if (myStack[1] != 0 && (button == 'E')) // enter numeric value, pushes first number onto stack
    {
      stackLocation = 1;
      decimal = 0;
      break;
    }

    else if (button == '-' || button == '+' || button == '*' || button == '/' || button == '^') // if operation is selected, preform domath() and nixieTranslate() subroutines using only the first number popped off stack
    {
      operation = button;
      if (myStack[1] != 0) {
        domath(myStack[0], myStack[1]); // preforms calculations and prints result to lcd
      } else {
        domath(myStack[0], myStack[0]); // preforms calculations and prints result to lcd
      }
      myStack[0] = total;
      myStack[1] = 0;
      stackLocation = 0;
      nixieTranslate(myStack[0]); //uses domath() results and prints result to nixies
      button = 'N'; // reset operation

      // break;
    }
  }
  while (1) {
    // third while loop
    stackLocation = 2;
    if (button == 'C') {
      break; // resets while selecting an operator
    }
    button = customKeypad.getKey();
    if (button == 'C') // additional reset coverage
    {
      decimal = 0;
      memset(myStack, 0, sizeof(myStack));
      total = 0;
      operation = 0;
      // lcd.clear(); // clears lcd
      stackLocation = 0;
      nixieTranslate(0.00); // sets all nixies to zero

      break;
    }

    if (button == 'U') {
      peekUp();
    }

    if (button == 'D') {
      peekDown();
    }

    if (button == '.') // decimal place
    {
      decimal++;
    }
    if (button >= '0' && button <= '9') // third number
    {
      if (decimal == 0) {
        myStack[2] = myStack[2] * 10 + (button - '0');
      } else {
        switch (decimal) {
          case 1: // tenths
            myStack[2] = myStack[2] + (button - '0') / 10.00;
            decimal++;
            break;
          case 2: // hundredths
            myStack[2] = myStack[2] + (button - '0') / 100.00;
            decimal++;
            break;
          case 3: // nopes
            break;
        }
      }
      // lcd.clear();
      // lcd.setCursor(0, 0); // first lcd row
      // lcd.print(myStack[2]); // prints second number on lcd
      nixieTranslate( (double) myStack[2]); // prints second number on nixies
    }

    if ((button == '-' || button == '+' || button == '*' || button == '/' || button == '^') && operationsTwo == false) // if operation is selected, preform domath() and nixieTranslate() subroutines using the first and second numbers popped off stack
    {
      operation = button;
      if (myStack[2] != 0) {
        domath(myStack[1], myStack[2]); // preforms calculations and prints result to lcd
      } else {
        domath(myStack[1], myStack[1]); // preforms calculations and prints result to lcd
      }
      myStack[1] = total;
      myStack[2] = 0;
      stackLocation = 1;
      nixieTranslate(myStack[1]); //uses domath() results and prints result to nixies
      operationsTwo = true;

      button = 'N'; // reset operation, otherwise 2nd operation is automatically triggered

      // break;
    }
    if ((button == '-' || button == '+' || button == '*' || button == '/' || button == '^') && operationsTwo == true) // second round of operations
    {
      operation = button;
      domath(myStack[1], myStack[0]); // preforms calculations and prints result to lcd
      myStack[0] = total;
      myStack[1] = 0;
      stackLocation = 0;
      nixieTranslate(myStack[0]); //uses domath() results and prints result to nixies
      operationsTwo = false;

      // break;
    }

  }
  while (1) {
    // After all is done this loop waits for 'C' key to be pressed so it can reset program and start over.
    if (button == 'C') {
      break; // This line is side effect of previous loop since if user pressed 'C' it breaks out of previous loop and continues here.So we need to break this one aswell or user would need to press 'C' 2 times
    }
    button = customKeypad.getKey();
    if (button == 'C')
    {
      decimal = 0;
      // lcd.clear();
      // lcd.setCursor(0, 0);
      stackLocation = 0;
      nixieTranslate(0.00);
      memset(myStack, 0, sizeof(myStack));
      total = 0;
      // operation = 0;
      break;
    }
  }

}

// top is the front of the stack!

void peekDown() {
  if (stackLocation < 3) {
    stackLocation++;
    // lcd.clear();
    // lcd.setCursor(0, 0);
    // lcd.print(myStack[stackLocation]);
    nixieTranslate(myStack[stackLocation]);
  }
  Serial.print("current stack location: ");
  Serial.println(stackLocation);
}

void peekUp() {
  if (stackLocation > 0) {
    stackLocation--;
    // lcd.clear();
    // lcd.setCursor(0, 0);
    // lcd.print(myStack[stackLocation]);
    nixieTranslate(myStack[stackLocation]);
  }
  Serial.print("current stack location: ");
  Serial.println(stackLocation);
}

void domath(double num1, double num2) // switch case preforms math operations. results then printed to lcd
{
  switch (operation)
  {
    case '+': // addition
      total = num1 + num2;
      break;

    case '-': // subtraction
      total = num1 - num2;
      break;

    case '/': // division
      total = (float)num1 / (float)num2;
      break;

    case '*': // multiplication
      total = num1 * num2;
      break;

    case '^': // raised to a power
      total = pow(num1, num2);
      break;
  }

  // moves stack forward
  num1 = total;
  num2 = 0;

  // print results to lcd
  // lcd.clear();
  // lcd.setCursor(0, 0);
  // lcd.print(total);

}

void nixieTranslate(double nixie) // takes in either user-submitted data or the result of an operation. breaks down number into each digit, then sends to the appropriate nixie using nixieIlluminate
{
  number = (nixie * 100) / 1; // multiplies by 100 in order to account for the tenths and hundredths place. divides by int one to truncate remaining decimal digits.

  thousands = number / 100000;
  hundreds = (number % 100000) / 10000;
  nixieIlluminate(thousands, hundreds, 0); // sends individual digit values in pair format along with place marker, to nixieIlluminate

  Serial.print("Thousands Place: "); // troubleshooting
  Serial.print(thousands);
  Serial.print("\n");
  Serial.print("Hundreds Place: "); // troubleshooting
  Serial.print(hundreds);
  Serial.print("\n");


  tens = (number % 10000) / 1000;
  ones = (number % 1000) / 100;
  nixieIlluminate(tens, ones, 1);
  
  Serial.print("Tens Place: "); // troubleshooting
  Serial.print(tens);
  Serial.print("\n");
  Serial.print("Ones Place: "); // troubleshooting
  Serial.print(ones);
  Serial.print("\n");

  tenths = (number % 100) / 10;
  hundredths = number % 10;
  nixieIlluminate(tenths, hundredths, 2);
  
  Serial.print("Tenths Place: "); // troubleshooting
  Serial.print(tenths);
  Serial.print("\n");
  Serial.print("Hundredths Place: "); // troubleshooting
  Serial.print(hundredths);
  Serial.print("\n");
  Serial.print("\n\n");

  for (int i = 0; i < 4; i++) {
    Serial.print("myStack[");
    Serial.print(i);
    Serial.print("]: ");
    Serial.println(myStack[i]);
  }

  Serial.println("\n");

  // update operations center on TFT
  nixieTFT();

}

void nixieIlluminate(int front, int back, int set) // takes in pair values for thousands-hundreds (set = 0), tens-ones (set = 1), and tenths-hundreths places (set = 2).
{
  int concatenated = (front*10) + back; // correct orientation
  // int concatenated = (back*10) + front; // reversed due to wiring error + laziness, if needed
  byte close_hold = charTable[concatenated];
  
  switch (set)
  {
    case 0:   
      digitalWrite(latchPinTHL, LOW);
      shiftOut(dataPinTHL, clockPinTHL, MSBFIRST, close_hold);
      digitalWrite(latchPinTHL, HIGH);

      Serial.print("Thousand-Hundred Pair: ");
      Serial.println(concatenated);
      Serial.print("Thousand-Hundred Pair [byte]: ");
      Serial.println(close_hold);
      break;

    case 1:
      digitalWrite(latchPinTO, LOW);
      shiftOut(dataPinTO, clockPinTO, MSBFIRST, close_hold);
      digitalWrite(latchPinTO, HIGH);

      Serial.print("Tens-Ones Pair: ");
      Serial.println(concatenated);
      Serial.print("Tens-Ones Pair [byte]: ");
      Serial.println(close_hold);
      break;

    case 2:
      digitalWrite(latchPinTHS, LOW);
      shiftOut(dataPinTHS, clockPinTHS, MSBFIRST, close_hold);
      digitalWrite(latchPinTHS, HIGH);

      Serial.print("Tenths-Hundredths Pair: ");
      Serial.println(concatenated);
      Serial.print("Tenths-Hundredths Pair [byte]: ");
      Serial.println(close_hold);
      break;

  }
}

void nixieTFTsetUP() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.println();
  tft.println("Pointer:");
  tft.println();
  tft.println("Operand:");
  tft.println("S[0]:");
  tft.println("S[1]:");
  tft.println("S[2]:");
  tft.println("S[3]:");
  tft.println();
  tft.setTextColor(ILI9341_WHITE);
  tft.print("Mahdi Al-Husseini");
  // tft.setTextColor(ILI9341_BLUE);
}

void nixieScreenClear() {
  tft.fillRect(120, 0, 140, 200, ILI9341_BLACK);
}

void nixieLoadingScreen() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 120);
  tft.print("A");
  tft.setCursor(65, 120);
  tft.setTextSize(1);
  tft.print("OITA");
  tft.setCursor(180, 160);
  tft.print("2019");
}

void nixieTFT() {
  stackLocationDetermination();
  nixieScreenClear();
  tft.setCursor(120, 25);
  tft.print(pointer);
  tft.setCursor(120, 74);
  tft.print(operation);
  tft.setCursor(120, 98);
  tft.print(myStack[0]);
  tft.setCursor(120, 123);
  tft.print(myStack[1]);
  tft.setCursor(120, 147);
  tft.print(myStack[2]);
  tft.setCursor(120, 171);
  tft.print(myStack[3]);
}

void stackLocationDetermination(){
      switch (stackLocation) {
      case 0: // stack[0]
        pointer = "S[0]";
        break;
      case 1: // stack[1]
        pointer = "S[1]";
        break;
      case 2: // stack[2]
        pointer = "S[2]";
        break;
      case 3: // stack[3]
        pointer = "S[3]";
        break;    
        }
}

