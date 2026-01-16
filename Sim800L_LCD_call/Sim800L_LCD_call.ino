/*
* This code illstrtates how the SIM800L module  can be used for making and receiving calls
* SIM800L Audio Connections:
* SPK+ -----> Speaker positive terminal
* SPK- -----> Speaker negative terminal
* MIC+ -----> Microphone positive (with bias resistor ~10kΩ to VCC)
* MIC- -----> Microphone negative
* AGND -----> Audio ground (connect to main ground)
*/
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// SIM800L module pins
SoftwareSerial sim800l(2, 3);  // RX, TX pins

// LCD setup (I2C address 0x27, 16x2 display)
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
byte rowPins[ROWS] = { 4, 5, 6, 7 };    // Connect to the row pinouts of the keypad
byte colPins[COLS] = { 8, 9, 10, 11 };  // Connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Variables
String phoneNumber = "";
bool callInProgress = false;
bool incomingCall = false;
String incomingNumber = "";
unsigned long lastKeyTime = 0;
const unsigned long DISPLAY_TIMEOUT = 30000;  // 30 seconds timeout

void setup() {
  // Initialize serial communications
  Serial.begin(9600);
  sim800l.begin(9600);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Phone Dialer");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  delay(1000);

  // Initialize SIM800L module
  initSIM800L();

  // Display ready message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready to dial");
  lcd.setCursor(0, 1);
  lcd.print("Enter number:");
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    lastKeyTime = millis();
    handleKeyPress(key);
  }

  // Handle SIM800L responses
  if (sim800l.available()) {
    String response = sim800l.readString();
    Serial.println("SIM800L: " + response);
    handleSIMResponse(response);
  }

  // Clear display after timeout if no call in progress
  if (!callInProgress && phoneNumber.length() == 0 && millis() - lastKeyTime > DISPLAY_TIMEOUT && lastKeyTime > 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ready to dial");
    lcd.setCursor(0, 1);
    lcd.print("Enter number:");
    lastKeyTime = 0;
  }
}

void initSIM800L() {
  lcd.setCursor(0, 1);
  lcd.print("Checking SIM...");

  // Test AT command
  sim800l.println("AT");
  delay(1000);

  // Check SIM card status
  sim800l.println("AT+CPIN?");
  delay(1000);

  // Check network registration
  sim800l.println("AT+CREG?");
  delay(1000);

  // Check signal quality
  sim800l.println("AT+CSQ");
  delay(1000);

  // Set to text mode for better compatibility
  sim800l.println("AT+CMGF=1");
  delay(1000);

  // Enable caller ID
  sim800l.println("AT+CLIP=1");
  delay(1000);

  // Set ring indicator
  sim800l.println("AT+CRSL=1");
  delay(1000);

  lcd.setCursor(0, 1);
  lcd.print("SIM Ready      ");
}

void handleKeyPress(char key) {
  if (incomingCall) {
    // Handle incoming call
    switch (key) {
      case 'A':  // Answer call
        answerCall();
        break;

      case 'D':  // Reject call
        rejectCall();
        break;
    }
  } else if (!callInProgress) {
    switch (key) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '*':
      case '#':
        // Add digit to phone number
        if (phoneNumber.length() < 11) {  // Limit phone number length
          phoneNumber += key;
          updateDisplay();
        }
        break;

      case 'A':  // Call button
        if (phoneNumber.length() > 0) {
          makeCall();
        }
        break;

      case 'B':  // Backspace
        if (phoneNumber.length() > 0) {
          phoneNumber.remove(phoneNumber.length() - 1);
          updateDisplay();
        }
        break;

      case 'C':  // Clear
        phoneNumber = "";
        updateDisplay();
        break;

      case 'D':  // Cancel/Hang up
        phoneNumber = "";
        updateDisplay();
        break;
    }
  } else {
    // Call in progress - only allow hang up
    if (key == 'D' || key == 'A') {
      hangUpCall();
    }
  }
}

void updateDisplay() {
  if (incomingCall) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Incoming Call");
    lcd.setCursor(0, 1);
    if (incomingNumber.length() > 0) {
      lcd.print(incomingNumber.substring(0, 16));  // Show up to 16 chars
    } else {
      lcd.print("Unknown Number");
    }
    return;
  }

  lcd.clear();
  if (phoneNumber.length() == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Ready to dial");
    lcd.setCursor(0, 1);
    lcd.print("Enter number:");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Number:");
    lcd.setCursor(0, 1);
    lcd.print(phoneNumber);

    // Show instructions on second line if number is short
    if (phoneNumber.length() <= 8) {
      lcd.setCursor(phoneNumber.length(), 1);
      lcd.print(" A=Call");
    }
  }
}

void makeCall() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calling...");
  lcd.setCursor(0, 1);
  lcd.print(phoneNumber);

  // Send call command to SIM800L
  String callCommand = "ATD" + phoneNumber + ";";
  sim800l.println(callCommand);

  callInProgress = true;

  Serial.println("Making call to: " + phoneNumber);
}

void hangUpCall() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hanging up...");

  // Send hang up command
  sim800l.println("ATH");

  delay(2000);

  callInProgress = false;
  phoneNumber = "";
  incomingCall = false;
  incomingNumber = "";
  updateDisplay();

  Serial.println("Call ended");
}

// Answer incoming call
void answerCall() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Answering...");

  sim800l.println("ATA");  // Answer call command

  incomingCall = false;
  callInProgress = true;

  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Call Active");
  lcd.setCursor(0, 1);
  if (incomingNumber.length() > 0) {
    lcd.print(incomingNumber.substring(0, 16));
  } else {
    lcd.print("Connected");
  }

  Serial.println("Call answered");
}

// Reject incoming call
void rejectCall() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rejecting call...");

  sim800l.println("ATH");  // Hang up command

  delay(2000);

  incomingCall = false;
  incomingNumber = "";
  callInProgress = false;

  updateDisplay();

  Serial.println("Call rejected");
}

void handleSIMResponse(String response) {
  response.trim();

  if (response.indexOf("RING") != -1) {
    // Incoming call detected
    if (!callInProgress && !incomingCall) {
      incomingCall = true;
      Serial.println("Incoming call detected");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Incoming Call");
      lcd.setCursor(0, 1);
      lcd.print("A=Answer D=Reject");
    }
  } else if (response.indexOf("+CLIP:") != -1) {
    // Caller ID information
    int startQuote = response.indexOf('"');
    int endQuote = response.indexOf('"', startQuote + 1);
    if (startQuote != -1 && endQuote != -1) {
      incomingNumber = response.substring(startQuote + 1, endQuote);
      Serial.println("Incoming from: " + incomingNumber);
      if (incomingCall) {
        updateDisplay();  // Update display with caller ID
      }
    }
  } else if (response.indexOf("OK") != -1) {
    Serial.println("Command executed successfully");
  } else if (response.indexOf("ERROR") != -1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error occurred");
    lcd.setCursor(0, 1);
    lcd.print("Check SIM/Signal");
    delay(3000);

    if (!callInProgress && !incomingCall) {
      updateDisplay();
    }
  } else if (response.indexOf("NO CARRIER") != -1 || response.indexOf("BUSY") != -1 || response.indexOf("NO ANSWER") != -1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Call ended");

    if (response.indexOf("BUSY") != -1) {
      lcd.setCursor(0, 1);
      lcd.print("Line busy");
    } else if (response.indexOf("NO ANSWER") != -1) {
      lcd.setCursor(0, 1);
      lcd.print("No answer");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Disconnected");
    }

    delay(3000);
    callInProgress = false;
    incomingCall = false;
    incomingNumber = "";
    phoneNumber = "";
    updateDisplay();
  } else if (response.indexOf("+CREG:") != -1) {
    // Network registration status
    if (response.indexOf(",1") != -1 || response.indexOf(",5") != -1) {
      Serial.println("Network registered");
    } else {
      Serial.println("Network not registered");
    }
  } else if (response.indexOf("+CSQ:") != -1) {
    // Signal quality response
    Serial.println("Signal quality: " + response);
  }
}

// Function to display current status (can be called for debugging)
void displayStatus() {
  Serial.println("=== Status ===");
  Serial.println("Phone Number: " + phoneNumber);
  Serial.println("Call in Progress: " + String(callInProgress));
  Serial.println("Incoming Call: " + String(incomingCall));
  Serial.println("Incoming Number: " + incomingNumber);
  Serial.println("=============");
}