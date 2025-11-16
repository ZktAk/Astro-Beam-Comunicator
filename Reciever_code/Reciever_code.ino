#include <Arduino.h>

// -----------------------------
// Hardware configuration
// -----------------------------
const int sensorPin = 2;        // Digital pin with interrupt support
const int THRESHOLD = 512;      // Only relevant if you use analogRead first

// -----------------------------
// Protocol parameters
// -----------------------------
#define CHARACTER_LIMIT 66
uint8_t END_OF_TRANSMISSION = 0x04;
uint8_t PREAMBLE = 0b10101010;

// -----------------------------
// Data storage
// -----------------------------
uint8_t messageBinary[CHARACTER_LIMIT + 2]; // Message buffer
volatile uint8_t currentChar = 0;
volatile int byteIndex = 0;

// -----------------------------
// Timing and state variables
// -----------------------------
volatile bool lastBit = 0;
volatile unsigned long lastTransitionTime = 0;
volatile unsigned long prevDelta = 0;
volatile unsigned long nowDelta = 0;

// Reception state
volatile bool receiving = false; // true when preamble detected
volatile int receivedBits = 0;
volatile bool mid = 1;
volatile bool halfBit = 0;

// Edge detection flag
volatile bool edgeDetected = false;
volatile bool bitState = 0;

// ============================================================
// ISR: Triggered on RISING or FALLING edge
// ============================================================
void edgeISR() {
  unsigned long now = micros();
  bitState = digitalRead(sensorPin);
  nowDelta = now - lastTransitionTime;
  lastTransitionTime = now;
  edgeDetected = true;
}

// ============================================================
// setup()
// ============================================================
void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);

  // Attach interrupt on CHANGE (detect both edges)
  attachInterrupt(digitalPinToInterrupt(sensorPin), edgeISR, CHANGE);

  Serial.println("Manchester Receiver Ready");
}

// ============================================================
// loop()
// ============================================================
void loop() {
  if (!edgeDetected) return;
  edgeDetected = false;

  bool transition = bitState ^ lastBit;
  lastBit = bitState;

  if (!receiving) {
    // --- Waiting for preamble ---
    if (transition) {
      currentChar = (currentChar << 1) | bitState;

      // Keep only last 8 bits
      currentChar &= 0xFF;

      if (currentChar == PREAMBLE) {
        receiving = true;
        lastTransitionTime = micros();
        prevDelta = nowDelta;
        byteIndex = 0;
        receivedBits = 0;
        mid = 1;
        halfBit = 0;

        // Store preamble as first byte
        messageBinary[byteIndex++] = currentChar;
        Serial.println("\nPreamble detected. Message inbound...");
        currentChar = 0;
      }
    }
    return; // do not process message until preamble detected
  }

  // --- Receiving message ---
  if (transition) {
    float ratio = prevDelta ? (float)nowDelta / (float)prevDelta : 1.0;

    bool condLow = ratio < 0.75;
    bool condHigh = ratio > 1.5;

    halfBit = condLow | (halfBit & ~condHigh);
    mid = (mid & ~(condLow | condHigh)) ^ (halfBit & ~(condLow | condHigh));
    mid |= condHigh;
    mid &= ~condLow;

    if (mid) {
      receivedBits++;
      currentChar = (currentChar << 1) | bitState;
    }

    prevDelta = nowDelta;

    // Check for full byte
    if (receivedBits >= 8) {
      messageBinary[byteIndex++] = currentChar;
      receivedBits = 0;
      currentChar = 0;

      // End-of-Transmission check
      if (messageBinary[byteIndex - 1] == END_OF_TRANSMISSION || byteIndex >= CHARACTER_LIMIT) {
        receiving = false;

        Serial.println("Received Message:");
        for (int i = 1; i < byteIndex - 1; i++) { // skip preamble, exclude EOT
          Serial.print(char(messageBinary[i]));
        }
        Serial.println("");

        byteIndex = 0;
      }
    }
  }
}
