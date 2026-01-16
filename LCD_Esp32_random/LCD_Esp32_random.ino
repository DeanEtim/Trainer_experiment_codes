#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27 or 0x3F depending on your module

void displayScrollingMessage(const String& message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);

  // If message is longer than LCD width, scroll it
  if (message.length() > 16) {
    delay(1000);  // Show the beginning for a moment

    // Scroll the entire message
    for (int i = 0; i < message.length() - 16 + 1; i++) {
      lcd.scrollDisplayLeft();
      delay(300);  // Adjust speed of scrolling
    }

    delay(500);   // Pause at the end
    lcd.clear();  // Reset display
  }
}

void setup() {
  Serial.begin(115200);  //initialize serial monitor
  randomSeed(analogRead(0));

  // initialize LCD
  lcd.init();
  lcd.backlight();

  //Welcome screen
  lcd.setCursor(0, 0);
  displayScrollingMessage("Hello World!");
  delay(2000);

}  // End setup

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);  // Set the cursor to the first line
  lcd.print("Random Numbers");

  lcd.setCursor(0, 1);        // Set the cursor to the second line
  lcd.print(random(1, 101));  // Print random numbers between 1 and 100

  delay(1000);
}  // End main loop
