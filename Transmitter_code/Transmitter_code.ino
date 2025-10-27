// === Manchester Transmitter ===

const int laserPin = 9;         // Laser output pin
const int clockPulseRate_Hz = 100;   // halfBitRate_Hz  // Transmission frequency (bits per second)
unsigned long clockPulse_ms = 1000 / clockPulseRate_Hz; // Microseconds per bit
unsigned long clockPulse_us = 1000000 / clockPulseRate_Hz; // Microseconds per bit

bool mid = 1;
bool laserOn = 0;
bool nextBit = 0;

String message = "";
String messageBin = "";

void setup() {
  Serial.begin(9600);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);
  Serial.println("Transmitter ready. Please enter a message.");
}

void loop() {

  mid = !mid;  
  if (!mid) getMessage(); // Note: if there is no serial string to read, then we keep the current message.
  if (messageBin.length() == 0) {
    if (!mid) nextBit = !nextBit;
  } // if there is no message, just send Manchester encoded alternating 1's and 0's
  else nextBit = messageBin[0] == '1';
  //Serial.println("mid: " + (String)mid + " | bit: " + (String)nextBit);

  if (mid) {
    // midpoint: encode the bit value
    //Serial.print(nextBit);
    //Serial.println(": mid");
    digitalWrite(laserPin, nextBit ? HIGH : LOW); // if bit=1, we want low to high transition.
    messageBin.remove(0, 1);  // Advance one bit after full period
  } else {
    // bit boundary: opposite of midpoint
    //Serial.print(!nextBit);
    //Serial.println(": boundry");
    digitalWrite(laserPin, nextBit ? LOW : HIGH); // if next bit=1, we want low to high transition, so we must set low right not.
    
  }
  delay(clockPulse_ms);
  //delayMicroseconds(clockPulse_us);
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
  messageBin = "0111111110" + messageBin + "0111111110";
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