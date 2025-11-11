// ============================================================
// === Manchester Receiver (String → char[] version) ===
// ============================================================
// This sketch decodes a Manchester-encoded optical signal
// received via a photodiode or phototransistor connected to A0.
//
// The receiver continuously samples the analog input and detects
// transitions (edges) in the signal to recover timing and data bits.
//
// Operation overview:
//  1. Wait for light transitions indicating the preamble pattern.
//  2. Use timing ratios between transitions to identify half- vs full-bit spacing.
//  3. Reconstruct each bit from edge timing and polarity.
//  4. Stop reading once an End-of-Transmission (EOT) byte (0x04) is received.
//
// The code uses a software timing-based edge detector rather than
// hardware interrupts, allowing simple adaptation across platforms.
// ============================================================

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

// -----------------------------
// Hardware configuration
// -----------------------------
const int sensorPin = A0;      // Analog input from photodiode or phototransistor
const int THRESHOLD = 20;      // ADC threshold separating light/dark (tune per setup)

// -----------------------------
// Protocol parameters
// -----------------------------
#define CHARACTER_LIMIT 64     // Maximum message size (excluding pre/postamble)
#define SAMPLE_PERIOD_US 25    // Sampling interval (higher = slower, but less CPU load)

uint8_t END_OF_TRANSMISSION = 0x4; // ASCII EOT (signals end of message)
uint8_t PREAMBLE = 0b10101010;     // Known alternating bit pattern for sync detection

// -----------------------------
// Data storage
// -----------------------------
uint8_t messageBinary[CHARACTER_LIMIT + 2]; // Full message buffer including pre/postamble
uint8_t currentChar = 0b0;                  // Currently assembled byte
int byteIndex = 0;                          // Index of current byte in messageBinary

// -----------------------------
// State and signal tracking
// -----------------------------
bool recieving = 0;             // Indicates if currently in active reception

int valueADC;                   // Latest analog read value
bool bit;                       // Current digital-level state (1/0)
bool lastBit = 0b0;             // Previous sampled bit
bool transition;                // TRUE when signal changes polarity

// Timing variables for measuring bit intervals
unsigned long prevDelta = 0;    // Duration between previous two transitions
unsigned long nowDelta = 0;     // Duration of current transition spacing
unsigned long lastTransitionTime = 0;  // Timestamp of last detected transition
unsigned long now;              // Current timestamp (micros())

// Bit assembly helpers
int recievedBits = 0;           // Number of bits currently accumulated in currentChar
bool mid;                       // Mid-bit indicator (used for timing-based decoding)
bool halfBit;                   // Tracks half-bit vs full-bit transitions


// ============================================================
// setup()
// Initializes serial output and input pin configuration.
// ============================================================
void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);
  Serial.println("Manchester Receiver Ready");
}


// ============================================================
// loop()
// Continuously samples the optical input to detect and decode
// incoming Manchester-encoded data streams.
// ============================================================
void loop() {

  // Regular sampling delay for consistent read timing
  delayMicroseconds(SAMPLE_PERIOD_US);

  // Convert analog reading to binary logic level
  valueADC = analogRead(sensorPin);
  bit = valueADC < THRESHOLD;   // Normal logic: dark (low light) = 0

  // --- Wait for incoming signal ---
  // If not currently receiving and the signal is idle (no light), skip loop
  if (!recieving && !bit) {
    return;
  }

  // --- Start of transmission detected ---
  // Once the signal goes active, initialize tracking variables
  recieving = 1;
  lastBit = 0;
  mid = 1;
  halfBit = 0;
  currentChar = 0b0;

  // ============================================================
  // PREAMBLE DETECTION PHASE
  // Wait until the byte reconstructed from transitions equals
  // the known preamble pattern (0b10101010).
  // This ensures the receiver is aligned to the transmitter’s timing.
  // ============================================================
  while (currentChar != PREAMBLE) {
    valueADC = analogRead(sensorPin);
    bit = valueADC < THRESHOLD;
    transition = bit ^ lastBit;   // XOR detects edge (change in signal)

    if (transition) {
      // Shift in the latest bit based on transition polarity
      currentChar = (currentChar << 0b1) | bit;

      // Record transition timing for synchronization reference
      now = micros();
      nowDelta = now - lastTransitionTime;

      lastTransitionTime = now;
      prevDelta = nowDelta;
      lastBit = bit;
    }

    delayMicroseconds(SAMPLE_PERIOD_US);
  }

  Serial.println("\nMessage Inbound...");

  // Store preamble as first byte
  byteIndex = 0;
  messageBinary[byteIndex] = currentChar;

  // ============================================================
  // MESSAGE RECONSTRUCTION PHASE
  // Read and reconstruct each byte until End-of-Transmission (0x04)
  // is received.
  // ============================================================
  while (messageBinary[byteIndex] != END_OF_TRANSMISSION) {

    byteIndex++;
    recievedBits = 0;
    currentChar = 0b0;  

    // --- Bit accumulation loop (8 bits per character) ---
    while (recievedBits < 8) {
      valueADC = analogRead(sensorPin);
      bit = valueADC < THRESHOLD;
      transition = bit ^ lastBit;     // Detect edge
      lastBit = bit;

      if (transition) {
        // Record transition timing for bit-interval comparison
        now = micros();
        nowDelta = now - lastTransitionTime;        

        // Compute timing ratio between current and previous edges
        // Used to distinguish half-bit vs full-bit spacing
        float ratio = (float)nowDelta / (float)prevDelta;

        // Determine timing conditions:
        // condLow  → short interval (half-bit)
        // condHigh → long interval (double-bit or missing transition)
        byte condLow  = (ratio < 0.75);
        byte condHigh = (ratio > 1.5);
        
        // ------------------------------------------------------------
        // Half-bit state tracking logic:
        // halfBit is set if a short interval is detected,
        // or retained unless reset by a long interval.
        // ------------------------------------------------------------
        halfBit = condLow | (halfBit & ~condHigh);

        // ------------------------------------------------------------
        // Mid-bit decoding logic:
        // - mid = 0 when condLow (short)
        // - mid = 1 when condHigh (long)
        // - toggles when a valid half-bit pair occurs
        // ------------------------------------------------------------
        mid = (mid & ~(condLow | condHigh)) ^ (halfBit & ~(condLow | condHigh));
        mid |= condHigh;
        mid &= ~condLow;

        // ------------------------------------------------------------
        // Bit reconstruction:
        // Each full bit corresponds to one "mid" state,
        // so bits are sampled only when mid == 1.
        // ------------------------------------------------------------
        if (mid) {
          recievedBits++;
          currentChar = (currentChar << 0b1) | bit;
        }

        // Update transition timing references for next loop iteration
        lastTransitionTime = now;
        prevDelta = nowDelta;
      }

      delayMicroseconds(SAMPLE_PERIOD_US);
    }

    // Store the fully reconstructed byte
    messageBinary[byteIndex] = currentChar;
  }

  // ============================================================
  // MESSAGE COMPLETION
  // Print decoded message characters to the serial monitor,
  // skipping the preamble and excluding the EOT byte.
  // ============================================================
  Serial.println("Received Message:");
  for (int i = 1; i < byteIndex; i++) {  // Skip preamble at index 0
    Serial.print(char(messageBinary[i]));
  }
  Serial.println("");

  // Reset receiver state for next transmission
  recieving = 0;
}
