const int laserPin = 9;         // Laser output pin
const int bitRate_Hz = 2000;     // Transmission frequency (bits per second)
unsigned long bitDuration_us = 1000000 / bitRate_Hz; // Microseconds per bit
const int testBits = 100;       // Length of alternating test code (101010...)

void setup() {
  Serial.begin(9600);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);
  Serial.println("Transmitter ready. Press enter to send test code.");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read input until newline

    input.trim(); // Remove whitespace and newline characters

    if (input == "1") {
      digitalWrite(laserPin, HIGH); // Set pin HIGH
      Serial.println("Pin 9 set HIGH");
    } 
    else if (input == "0") {
      digitalWrite(laserPin, LOW); // Set pin LOW
      Serial.println("Pin 9 set LOW");
    } 
    else {
      sendTestCode(); // Keep original behavior
    }
  }
}


void sendTestCode() {
  Serial.println("Sending test code: alternating 1s and 0s for 100 bits");
  for (int i = 0; i < testBits; i++) {
    digitalWrite(laserPin, (i % 2) == 0 ? HIGH : LOW); // Alternate 1 (HIGH) and 0 (LOW)
    delayMicroseconds(bitDuration_us);
  }
  digitalWrite(laserPin, LOW);
  Serial.println("Test code sent.");
}