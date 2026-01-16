#include <SoftwareSerial.h>
SoftwareSerial sim800(10, 11);  // SIM800L TX, SIM800L RX

void setup() {
  Serial.begin(9600);  // Serial monitor
  sim800.begin(9600);  // Initial baud rate of SIM800L

  Serial.println("Initialising...");
  delay(2000);
  Serial.println("SIM800L ready");
}

void loop() {
  // Forward data from SIM800L to Serial Monitor
  if (sim800.available()) {
    Serial.write(sim800.read());
  }

  // Forward data from Serial Monitor to SIM800L
  if (Serial.available()) {
    sim800.write(Serial.read());
  }
}
