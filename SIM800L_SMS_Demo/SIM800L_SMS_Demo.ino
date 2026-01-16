/*
 * Last Updated: 27/08/2025
 *
 * This program implements a simple SMS-only interface using the SIM800L module,
 * a 4x4 matrix keypad, and a 16x2 I2C LCD. It allows the user to enter an
 * 11-digit phone number, then press A, B or C to send one of three hardcoded SMS messages.
 *
 * Features:
 *  - On startup, the program checks SIM800L network registration.
 *  - If connected: displays "Phone Ready" for 2s, then "Enter Number:"
 *  - Number entry:
 *      * Digits (0-9) appended and shown on LCD.
 *      * 'C' deletes the last digit (like a backspace).
 *      * 'A' confirms the number (must actually be exactly 11 digits).
 *      * Any other key during entry is invalid.
 *  - After confirmation (by pressing "A"), press 'A','B' or 'C' to send three different messages.
 *  - When an incoming SMS arrives, the LCD shows an alert and the full
 *    SMS is printed to the Serial Monitor.
 *
 * Hardware Connections:
 *  SIM800L RX = 16 (TX2 ON ESP32)
 *  SIM800L TX = 17 (RX2 ON ESP32)
 *  LCD I2C = (SDA = 21, SCL = 22)
 *
 *  Keypad:
 *  	ROWS:  Pins {23, 13, 12, 14 };
 *    COLUMNS: Pins {27, 26, 25, 33 };
 */

#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

//  Using ESP32 UART2
HardwareSerial sim800l(2);
// SIM800L communication pin
#define SIM_RX 16
#define SIM_TX 17

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
byte rowPins[ROWS] = { 23, 13, 12, 14 };
byte colPins[COLS] = { 27, 26, 25, 33 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Fixed messages
String msgA = "Hello from Group 5; Embedded Systems Trainer!";
String msgB = "This Demo shows how SMS notification can be integrated into an embedded system projects.";
String msgC = "Messages are hardcoded because the keypad will not allow the user to type any meaningful message due to its number of limited keys.";

// UI / state variables
String phoneNumber = "";
bool selectingMessage = false;  // true when user confirmed number and should pick A/B/C
unsigned long msgTimer = 0;
unsigned long msgDuration = 2000;  // default 2s for temporary messages

// Function prototypes
void showEnterNumber();
void showTempMessage(String msg, unsigned long duration = 2000);
void checkNetwork();
void sendSMS(String messageBody);
void handleKeyPress(char key);
void handleSim800l();
void readAndPrintSMSByIndex(int index);

void setup() {
  Serial.begin(115200);                             // ESP32 Serial Monitor
  sim800l.begin(9600, SERIAL_8N1, SIM_RX, SIM_TX);  // SIM800L Intialisation

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

  // clear temporary messages after msgDuration
  if (msgTimer > 0 && millis() - msgTimer > msgDuration) {
    msgTimer = 0;
    // If we were in selectingMessage and it timed out, still show Enter Number
    showEnterNumber();
  }
}  // end main loop

void showEnterNumber() {
  selectingMessage = false;  // ensure we are back to number entry mode
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Number:");
  lcd.setCursor(0, 1);
  lcd.print(phoneNumber);
}  // end showEnterNumber()

void showTempMessage(String msg, unsigned long duration) {
  lcd.clear();
  lcd.print(msg);
  msgTimer = millis();     // start timer
  msgDuration = duration;  // set duration for this message
}  // end showTempMessage()

void checkNetwork() {
  // ask for registration status
  sim800l.println("AT+CREG?");
  delay(500);

  if (sim800l.available()) {
    String resp = sim800l.readString();
    // Accept either home network (1) or roaming (5)
    if (resp.indexOf("+CREG: 0,1") != -1 || resp.indexOf("+CREG: 0,5") != -1) {
      lcd.clear();
      lcd.print("Phone Ready");
      delay(2000);
      showEnterNumber();
    } else {
      lcd.clear();
      lcd.print("No Network");
      delay(2000);
      checkNetwork();  // recursion (retry)
    }
  } else {
    // No response, treat as no network and retry (recursive)
    lcd.clear();
    lcd.print("No Network");
    delay(2000);
    checkNetwork();
  }
}  // end checkNetwork()

void sendSMS(String messageBody) {
  // Basic blocking send flow (keeps code simple like your call example)
  lcd.clear();
  lcd.print("Sending SMS...");
  // Ensure text mode
  sim800l.println("AT+CMGF=1");
  delay(300);
  // Start send
  sim800l.print("AT+CMGS=\"");
  sim800l.print(phoneNumber);
  sim800l.println("\"");
  delay(500);
  // Message body and terminate with Ctrl+Z
  sim800l.print(messageBody);
  sim800l.write(26);  // Ctrl+Z
  // wait for modem to send
  delay(1500);

  // Read any other modem reply and print to Serial for debugging
  if (sim800l.available()) {
    String resp = sim800l.readString();
    Serial.println("SIM RESP (send):");
    Serial.println(resp);
  }

  // Inform user
  lcd.clear();
  lcd.print("SMS Sent");
  delay(2000);

  // Reset to entry state
  phoneNumber = "";
  selectingMessage = false;
  showEnterNumber();
}  // end sendSMS()

void handleKeyPress(char key) {
  if (!selectingMessage) {
    // Number entry mode
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

    else if (key == 'A') {  // confirm / Enter
      if (phoneNumber.length() == 11) {
        selectingMessage = true;
        // Ask user to choose message A/B/C
        lcd.clear();
        lcd.print("Press A, B or C");
        lcd.setCursor(0, 1);
        lcd.print("to send message");
      } else {
        showTempMessage("Invalid length", 2000);
      }
    }

    else {  // any other key is invalid during number entry
      phoneNumber = "";
      showTempMessage("Invalid number", 2000);
    }
  }

  else {
    // selectingMessage == true; waiting for A/B/C to choose message
    if (key == 'A') {
      sendSMS(msgA);
    } else if (key == 'B') {
      sendSMS(msgB);
    } else if (key == 'C') {
      sendSMS(msgC);
    }
  }
}  // end handleKeyPress()

void handleSim800l() {
  // Read whatever the module has sent
  String resp = sim800l.readString();
  resp.trim();
  Serial.println(resp);

  // Incoming SMS indication from SIM800L looks like: +CMTI: "SM",<index>
  if (resp.indexOf("+CMTI:") != -1) {
    // parse index
    int comma = resp.indexOf(',');
    int idx = -1;
    if (comma != -1) {
      String idxStr = resp.substring(comma + 1);
      idxStr.trim();
      idx = idxStr.toInt();
    }

    // Alert user immediately
    lcd.clear();
    lcd.print("SMS Received!");
    delay(500);

    // Show "View message on Serial Monitor" for 3 seconds
    lcd.clear();
    lcd.print("View message on");
    lcd.setCursor(0, 1);
    lcd.print("Serial Monitor");
    delay(3000);

    // If we have a valid index, read and print message
    if (idx > 0) {
      readAndPrintSMSByIndex(idx);
    } else {
      // print whatever modem sent
      Serial.println("Raw incoming SMS indication:");
      Serial.println(resp);
    }

    // Reset our UI to Phone Ready, Enter Number:
    lcd.clear();
    lcd.print("Phone Ready");
    delay(2000);
    phoneNumber = "";
    selectingMessage = false;
    showEnterNumber();
  }
}  // end handleSim800l()

void readAndPrintSMSByIndex(int index) {
  // Request message body by index
  sim800l.print("AT+CMGR=");
  sim800l.println(index);
  delay(500);

  // Read available response and print to Serial
  String resp = "";
  unsigned long t0 = millis();
  while (millis() - t0 < 1500) {
    while (sim800l.available()) {
      char c = sim800l.read();
      resp += c;
    }
  }

  resp.trim();
  Serial.println("NEW MESSAGE RECEIVED:");
  Serial.println(resp);
  Serial.println(" ");
}  // end readAndPrintSMSByIndex()
