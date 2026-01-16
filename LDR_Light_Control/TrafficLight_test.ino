/*
LDR test code
*/
int sensorPin = A0;
int relayPin = 10;

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(9600);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
}

void loop() {
  int analogReadings = analogRead(sensorPin);
  int brightness = map(analogReadings, 0, 1024, 0, 100);
  Serial.print("SensorReadings: ");
  Serial.println(analogReadings);

  Serial.print("Percentage darkness: ");
  Serial.print(brightness);
  Serial.println("%");
  delay(500);  // wait for some time

  if (brightness >= 50) {
    digitalWrite(relayPin, LOW);
  } else {
    digitalWrite(relayPin, HIGH);
  }
}
