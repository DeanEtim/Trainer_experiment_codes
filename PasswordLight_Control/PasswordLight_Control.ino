/*
* This program turns ON the relay when the correct password is entered
* and turns it off when the same password is entered again the second time
*/
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 9, 8, 7, 6 };  // Connect to row pinouts
byte colPins[COLS] = { 5, 4, 3, 2 };  // Connect to column pinouts
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Relay pin
const int relayPin = 12;

// Password settings
const char password[] = "1234";  // Change this to your desired password
char enteredPassword[5];         // Container to hold entered password
byte passwordLength = 0;
byte maxPasswordLength = 4;  // Should match the length of your password

bool relayState = false;
bool enteringPassword = false;  // currently typing in the password?

void setup() {
  // Initialize relay pin
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // Relay off (active low)

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Display initial message
  lcd.setCursor(0, 0);
  lcd.print("Light Control");
  lcd.setCursor(0, 1);
  lcd.print("Press * to start");
}

void loop() {
  char key = keypad.getKey();  // checks to see if any key is pressed
  if (key) {
    if (key == '*' && !enteringPassword) {
      // Start password entry
      enteringPassword = true;
      passwordLength = 0;
      memset(enteredPassword, 0, sizeof(enteredPassword));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter Password:");
      lcd.setCursor(0, 1);
    }  // end if

    // Allow user to clear mistakenly entered character
    else if (key == '*' && passwordLength > 0) {
      // Backspace
      enteredPassword[--passwordLength] = '\0';
      lcd.setCursor(passwordLength, 1);
      lcd.print(" ");
      lcd.setCursor(passwordLength, 1);
    }

    // Encrypt the entered password
    else if (passwordLength < maxPasswordLength && (isdigit(key) || isAlpha(key))) {
      enteredPassword[passwordLength++] = key;  // Add character to password
      lcd.print('*');                           // Hides the entered password
    }

    // check if the entered password matches the preset password
    else if (enteringPassword) {
      if (key == '#') {
        // Submit password
        checkPassword();
      }
    }  // end else-if
  }    // end if
}  //end loop

void checkPassword() {
  if (strcmp(enteredPassword, password) == 0) {
    // Correct password - toggle relay
    relayState = !relayState;
    digitalWrite(relayPin, !relayState);  // Active low

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Light is ");
    lcd.print(relayState ? "ON" : "OFF");
    delay(2000);
  }  // end if

  else {
    // Wrong password
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wrong Password!");
    delay(2000);
  }  // end else

  // Reset for next attempt
  enteringPassword = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Light Control");
  lcd.setCursor(0, 1);
  lcd.print("Press * to start");
}  // end checkPassword()