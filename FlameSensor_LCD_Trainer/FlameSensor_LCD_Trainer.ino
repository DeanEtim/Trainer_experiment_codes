#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Create an instance of the class
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define flameSensorPin A0  // Flame sensor pin
#define buzzerPin 8        // buzzer pin

// Threshold for flame detection
const int flameThreshold = 500;  // Lower value means more sensitive

void setup() {
  // Initialize serial monitor
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Display Startup message
  lcd.setCursor(0, 0);
  lcd.print("Flame Detection");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
}

void loop() {
  // Read flame sensor value
  int flameValue = analogRead(flameSensorPin);

  // Display sensor value on serial monitor
  Serial.print("Flame Sensor Value: ");
  Serial.println(flameValue);

  // Clear LCD and display status
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sensor Value:");
  lcd.print(flameValue);

  lcd.setCursor(0, 1);
  if (flameValue < flameThreshold) {
    lcd.print("Flame Detected!");
    warningBeep();
  }

  else {
    lcd.print("No Flame");
  }
  delay(500);  // Short delay between readings
}  // end main loop

void warningBeep(void) {
  digitalWrite(buzzerPin, HIGH);
  delay(300);
  digitalWrite(buzzerPin, LOW);
  delay(300);
}  // end function