/*
 * Last Updated: 26/08/2025
 *
 * This program implements a simple GSM phone interface using the SIM800L module,
 * a 4x4 matrix keypad, and a 16x2 I2C LCD. It allows the user to make and receive calls
 * in a phone-like manner.
 *
 *  How it works:
 *   - On startup, checks SIM800L network registration.
 *     If connected: displays "Phone Ready", then "Enter Number:".
 *     If not connected: displays "No Network" and keeps retrying.
 *
 *   - Number entry:
 *       * Digits (0–9) are appended and shown on the LCD.
 *       * Key 'C' deletes the last entered digit (backspace).
 *       * Key 'D' initiates a call (only if exactly 11 digits are entered).
 *       * Any other key ('*', '#', 'A', 'B') during number entry is Invalid
 *
 *   - Outgoing calls:
 *       * Dialed the entered phone number with key 'D'.
 *       * Key 'C' ends the call.
 *
 *   - Incoming calls:
 *       * The system continuously checks for "RING" from the SIM800L.
 *       * LCD displays "Incoming call...".
 *       * Key 'A' answers the call.
 *       * Key 'C' rejects the call.
 *
 *   - Call status:
 *       * If the call is ended by the user (with key 'C') or remotely by the caller,
 *         LCD shows "Call Ended" briefly, then resets.
 *
 * Hardware Connections:
 *  SIM800L GSM Module:
 *    SIM800L TX = ESP32 Pin 16 (RX2)
 *    SIM800L RX = ESP32 Pin 17 (TX2)
 *
 *  16x2 I2C LCD:
 *    I2C Address = 0x27
 *    Connect SDA and SCL to ESP32 I2C pins (A4 and A5 respectively)
 *
 *  4x4 Matrix Keypad:
 *    Row Pins = ESP32 Pins 23, 13, 12, 14
 *    Column Pins = ESP32 Pins 27, 26, 25, 33
 */

#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// SIM800L communication pin
#define SIM_RX 16
#define SIM_TX 17

//  Using ESP32 UART2
HardwareSerial sim800l(2);
// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 4x4 Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 23, 13, 12, 14};
byte colPins[COLS] = { 27, 26, 25, 33};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String phoneNumber = "";
bool callInProgress = false;
bool ringing = false;
unsigned long msgTimer = 0;

// Fucntion Prototypes
void showEnterNumber();                          // Displays the entered phone number
void showTempMessage(String msg, int duration);  // Displays messages on the LCD
void checkNetwork();                             // Checks if the SIM800L module is connected to the network
void makeCall();                                 // Handles the making of calls
void hangUpCall();                               // Hangs up current call
void answerCall();                               // Handles the answering of calls
void handleKeyPress(char key);                   // Handles the keypad inputs
void handleSim800l();                            // Handles all the SIM800L functionalities

void setup() {
  Serial.begin(115200);  // ESP32 Serial Monitor
  sim800l.begin(9600, SERIAL_8N1, SIM_RX, SIM_TX);  // SIM800L baud rate

  lcd.init();
  lcd.backlight();

  // Keep checking until network is ready
  while (true) {
    sim800l.println("AT+CREG?");
    delay(500);

    if (sim800l.available()) {
      String resp = sim800l.readString();
      Serial.println(resp);  // debug

      if (resp.indexOf("+CREG: 0-1") != -1 || resp.indexOf("+CREG: 0-5") != -1) {
        lcd.clear();
        lcd.print("Phone Ready");
        delay(2000);
        showEnterNumber();
        break;  // exit while loop
      }

      else {
        lcd.clear();
        lcd.print("No Network");
        lcd.setCursor(0, 1);
        lcd.print("Connecting...");
        delay(1000);
      }
    }
  }  // end while
}  // end setup()

void loop() {
  // Get the key that is pressed
  char key = keypad.getKey();

  if (key) {
    handleKeyPress(key);
  }

  if (sim800l.available()) {
    handleSim800l();
  }

  // clear temporary messages after 2s
  if (msgTimer > 0 && millis() - msgTimer > 2000) {
    msgTimer = 0;
    showEnterNumber();
  }
}  // end main loop

void showEnterNumber() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Number:");
  lcd.setCursor(0, 1);
  lcd.print(phoneNumber);
}  // end showEnterNumber()

void showTempMessage(String msg, int duration = 2000) {
  lcd.clear();
  lcd.print(msg);
  msgTimer = millis();  // auto clear after duration
}  // end showTempMessage()

void checkNetwork() {
  sim800l.println("AT+CREG?");
  delay(500);
  if (sim800l.available()) {
    String resp = sim800l.readString();
    if (resp.indexOf("+CREG: 0,1") != -1 || resp.indexOf("+CREG: 0,5") != -1) {
      lcd.clear();
      lcd.print("Phone Ready");
      delay(2000);
      showEnterNumber();
    }

    else {
      lcd.clear();
      lcd.print("No Network");
      delay(2000);
      checkNetwork();  // recursion (retry)
    }
  }
}  // end checkNetwork()

void makeCall() {
  lcd.clear();
  lcd.print("Calling...");
  lcd.setCursor(0, 1);
  lcd.print(phoneNumber);
  sim800l.println("ATD" + phoneNumber + ";");
  callInProgress = true;
}  // end makeCall()

void hangUpCall() {
  sim800l.println("ATH");
  lcd.clear();
  lcd.print("Call Ended");
  delay(1000);
  callInProgress = false;
  phoneNumber = "";
  showEnterNumber();
}  // end hangUpCall()

void answerCall() {
  sim800l.println("ATA");
  lcd.clear();
  lcd.print("Call Connected");
  callInProgress = true;
  ringing = false;
}  // end answerCall()

void handleKeyPress(char key) {
  if (!callInProgress && !ringing) {
    if (isdigit(key)) {
      if (phoneNumber.length() < 11) {
        phoneNumber += key;
      }
      showEnterNumber();
    }

    else if (key == 'C') {  // backspace
      if (phoneNumber.length() > 0) {
        phoneNumber.remove(phoneNumber.length() - 1);
      }
      showEnterNumber();
    }

    else if (key == 'D') {  // dial the number
      if (phoneNumber.length() == 11) {
        makeCall();
      }

      else {
        showTempMessage("Invalid length");
      }
    }

    else {  // any other key = invalid
      phoneNumber = "";
      showTempMessage("Invalid number");
    }
  }

  else if (ringing) {
    if (key == 'A') {
      answerCall();
    }

    else if (key == 'C') {
      hangUpCall();
      ringing = false;
    }
  }

  else if (callInProgress) {
    if (key == 'C') hangUpCall();
  }
}  // end handleKeyPress()

void handleSim800l() {
  String resp = sim800l.readString();
  resp.trim();
  Serial.println(resp);

  if (resp.indexOf("RING") != -1) {
    ringing = true;
    lcd.clear();
    lcd.print("Incoming call...");
  }

  else if (resp.indexOf("NO CARRIER") != -1) {
    // call ended by the caller
    callInProgress = false;
    ringing = false;
    lcd.clear();
    lcd.print("Call Ended");
    delay(1000);
    phoneNumber = "";
    showEnterNumber();
  }
}  // end handleSim800l()
