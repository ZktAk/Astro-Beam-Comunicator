// ============================================================
// === Manchester Transmitter ===
// ============================================================
// This sketch transmits an ASCII message as a Manchester-encoded
// bitstream using a laser diode connected to the Arduino output pin.
//
// Encoding scheme:
// - Logical '1' → LOW→HIGH transition (rising edge mid-bit)
// - Logical '0' → HIGH→LOW transition (falling edge mid-bit)
// Each bit period consists of two equal half-bit intervals.
// ============================================================


// -----------------------------
// Hardware and transmission setup
// -----------------------------
const int laserPin = 9;                // Laser diode control pin
const int clockPulseRate_Hz = 1000;    // Half-bit frequency (bit transitions per second)
unsigned long clockPulse_ms = 1000 / clockPulseRate_Hz; 
unsigned long clockPulse_us = 1000000 / clockPulseRate_Hz; 

// Special transmission markers
int8_t END_OF_TRANSMISSION = 0x4;      // End-of-Transmission marker (ASCII EOT)
#define CHARACTER_LIMIT 64              // Max message length (in characters, excluding pre/postamble)


// -----------------------------
// Global variables
// -----------------------------
bool nextBit;                           // Holds the next data bit to be transmitted
String message = "";                    // Message buffer (user input string)
uint8_t messageBinary[CHARACTER_LIMIT + 2];  // Encoded bytes (includes preamble + postamble)
int byteIndex = 0;                      // Index of the last valid byte in messageBinary
bool sending = 0;                       // Transmission state flag


// ============================================================
// setup()
// Initializes serial communication and configures laser pin.
// ============================================================
void setup() {
  Serial.begin(115200);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);          // Laser is off by default
  Serial.println("Transmitter ready. Please enter a message.");
}


// ============================================================
// loop()
// - Waits for new serial input.
// - When a message is available, transmits it bit-by-bit.
// - When idle, keeps the laser off.
// ============================================================
void loop() {

  getMessage();   // Poll for serial input and prepare binary data if available

  // --- Idle State ---
  // If not currently transmitting, ensure the laser is off
  // and maintain the clock timing before looping again.
  if (!sending) {
    digitalWrite(laserPin, LOW);
    delay(clockPulse_ms);
    return;
  }

  // --- Transmission Phase ---
  // Outer loop iterates through each byte in the encoded message,
  // including the preamble at index 0 and postamble at the end.
  for (int i = 0; i < byteIndex + 1; i++) {    
    
    // Inner loop transmits bits from MSB to LSB
    for (int j = 7; j >= 0; j--) {      
      nextBit = (messageBinary[i] >> j) & 0b1;  // Extract bit j from the current byte

      // Manchester encoding:
      // - For bit '1', output LOW→HIGH transition (rising edge mid-bit)
      // - For bit '0', output HIGH→LOW transition (falling edge mid-bit)
      //
      // The laser output toggles once per half-bit period.
      // Each full bit = two equal half-bit delays.
      digitalWrite(laserPin, nextBit ? LOW : HIGH);     // First half-bit (pre-transition)
      delayMicroseconds(clockPulse_us);

      digitalWrite(laserPin, nextBit ? HIGH : LOW);     // Second half-bit (transition occurs)
      delayMicroseconds(clockPulse_us);

      // Optional debug section (commented out):
      // Allows visual inspection of Manchester waveform structure.
      // Serial.print(nextBit);
      // if (j == 7 - 3) Serial.print("_");
      // if (j == 0) Serial.print(" ");
    }
  }

  // Transmission complete
  sending = 0;
}


// ============================================================
// getMessage()
// Checks for user input over Serial. If a new message is found:
//  1. Reads and trims the input string.
//  2. Converts the message into binary form.
//  3. Adds preamble and postamble bytes.
// ============================================================
void getMessage() {
  if (Serial.available() > 0) {
    message = Serial.readStringUntil('\n');  // Read user input
    message.trim();

    // Ignore empty inputs (user pressed Enter with no message)
    if (message.length() == 0) {      
      sending = 0;
      return;
    }

    // Display message for confirmation
    Serial.println("\nTransmitting Message:");
    Serial.println(message);

    // Prepare for transmission
    sending = 1;
    messageToBinary();   // Convert string to binary byte array
    bookendMessage();    // Add preamble and EOT byte
  }
}


// ============================================================
// messageToBinary()
// Converts each ASCII character of the input message into an
// 8-bit binary representation, stored in `messageBinary`.
// 
// Note: Index 0 is reserved for the preamble, added later.
// ============================================================
void messageToBinary() {
  byteIndex = 0;  // Reset byte counter

  for (int i = 0; i < message.length(); i++) { 
    char c = message[i];
    uint8_t currentByte = 0b0;
    byteIndex = i + 1;

    // Extract each bit from MSB to LSB of the ASCII code
    for (int j = 0; j < 8; j++) {
      bool bit = (c >> (7 - j)) & 0b1;
      currentByte = (currentByte << 1) | bit;
    }

    messageBinary[byteIndex] = currentByte;
  }
}


// ============================================================
// bookendMessage()
// Adds preamble and postamble markers to the binary message.
//
// Preamble (0b10101010):
//   - Used by the receiver for clock recovery and bit alignment.
//   - Alternating pattern ensures consistent transitions.
//
// End-of-Transmission (EOT, 0x04):
//   - Signals message termination to the receiver.
//
// The messageBinary array layout becomes:
//   [0] = PREAMBLE
//   [1...N] = message bytes
//   [N+1] = END_OF_TRANSMISSION
// ============================================================
void bookendMessage() {
  uint8_t temp = 0b10101010;             // Preamble pattern
  messageBinary[0] = temp;
  messageBinary[byteIndex + 1] = END_OF_TRANSMISSION;
  byteIndex++;
}
