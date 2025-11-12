// Reciever code
const int analogPin = A0;             // Photodiode input
const int bitRate_Hz = 1000;          // Must match transmitter
const int threshold = 20;             // Raw ADC threshold for bit=1 (voltage < ~1V)
const int testBits = 100;             // Length of alternating test code (101010...)


unsigned long bitDuration_us = 1000000 / bitRate_Hz; // Microseconds per bit (initial)


void setup() {
  Serial.begin(115200);
  Serial.println("Receiver ready. Waiting for test code...");
}


void calibrateReceiver() {
  Serial.println("\nWaiting for calibration pattern (alternating 1s and 0s)...");
  unsigned long startTimes[testBits];
  int bitCount = 0;
  bool lastBit = false;
  bool inCalibration = false;

  while (bitCount < testBits) {
    int raw = analogRead(analogPin);
    //Serial.println(raw);
    bool bit = (raw < threshold) ? 1 : 0;
    if (!inCalibration && bit == 1) {
      // Detect start of calibration (first transition to 1)    
      inCalibration = true;
      startTimes[bitCount] = micros();
      bitCount++;
    
    } else if (inCalibration && bit != lastBit) {
      // Record time of each bit transition
      
      startTimes[bitCount] = micros();
      bitCount++;
    }
    lastBit = bit;
    delayMicroseconds(25); // Sample frequently
  }

  // Calculate average bit duration
  unsigned long totalDuration = startTimes[testBits - 1] - startTimes[0];
  bitDuration_us = totalDuration / (testBits - 1); // Average over transitions
  Serial.println("Calibrated: bitDuration_us = " + String(bitDuration_us));
}

void loop() {
  calibrateReceiver();
}