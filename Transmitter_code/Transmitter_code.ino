// === Manchester Transmitter ===

const int laserPin = 9;         // Laser output pin
const int clockPulseRate_Hz = 1000;   // halfBitRate_Hz  // Transmission frequency (bits per second)
unsigned long clockPulse_ms = 1000 / clockPulseRate_Hz; // Microseconds per bit
unsigned long clockPulse_us = 1000000 / clockPulseRate_Hz; // Microseconds per bit

//bool clock = 1; 
bool mid = 1;
bool laserOn = 0;

String message = "";
String messageBin = "";

void setup() {
  Serial.begin(9600);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);
  Serial.println("Transmitter ready. Please enter a message.");
}

void loop() {
  getMessage();
  // Note: if there is no serial string to read, then we keep the current message.

  //clock = !clock;
  mid = !mid;
  bool nextBit;
  if (messageBin.length() == 0) nextBit = 0; // if there is no message, just send Manchester encoded 0's
  else nextBit = messageBin[0] == '1';

  if (mid) {
    // midpoint: encode the bit value
    digitalWrite(laserPin, nextBit ? HIGH : LOW); // if bit=1, we want low to high transition.
  } else {
    // bit boundary: opposite of midpoint
    digitalWrite(laserPin, nextBit ? LOW : HIGH); // if next bit=1, we want low to high transition, so we must set low right not.
    messageBin.remove(0, 1);  // Advance one bit after full period
  }
  //delay(clockPulse_ms);
  delayMicroseconds(clockPulse_us);
}

void getMessage() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read input until newline
    input.trim(); // Remove whitespace and newline characters
    message = input; 

    messageToBinary();
    bookendMessage();
  }
}

void messageToBinary() {
  messageBin = textToBinary(message);
}

void bookendMessage() {
  messageBin = "11111111" + messageBin + "11111111";
}

String textToBinary(String text) { 
  String binary = ""; 
  binary.reserve(text.length() * 8 + 1); // Reserve for binary bits 
  for (int i = 0; i < text.length(); i++) { 
    char c = text[i]; 
    for (int j = 7; j >= 0; j--) { 
      binary += ((c >> j) & 1) ? '1' : '0'; 
    } 
  } 
  return binary; 
} 
