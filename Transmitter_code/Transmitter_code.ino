// === Manchester Transmitter ===

const int laserPin = 9;         // Laser output pin
const int clockPulseRate_Hz = 500;   // halfBitRate_Hz  // Transmission frequency (bits per second)
unsigned long clockPulse_ms = 1000 / clockPulseRate_Hz; 
unsigned long clockPulse_us = 1000000 / clockPulseRate_Hz; 

bool mid = 1;
bool laserOn = 0;
bool nextBit = 0;

String message = "";
String messageBin = "";

void setup() {
  Serial.begin(9600);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);  // laser off initially
  Serial.println("Transmitter ready. Please enter a message.");
}

void loop() {

  mid = !mid;  
  if (!mid) getMessage(); // check for new message

  // --- When idle, laser is off ---
  if (messageBin.length() == 0) {
    digitalWrite(laserPin, LOW);
    delay(clockPulse_ms);
    return;
  }
  else {
    mid = !mid;
  }

  while (messageBin.length() > 0) {
    mid = !mid;
    nextBit = messageBin[0] == '1';

    if (mid) {
      // midpoint: encode bit
      digitalWrite(laserPin, nextBit ? HIGH : LOW);
      messageBin.remove(0, 1);  // advance after full period
    } else {
      // bit boundary: opposite of midpoint
      digitalWrite(laserPin, nextBit ? LOW : HIGH);
    }

    delayMicroseconds(clockPulse_us);
    //delay(clockPulse_ms);
  }
  
  //delayMicroseconds(clockPulse_us);  
  delay(clockPulse_ms);
}

void getMessage() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;
    message = input; 
    messageToBinary();
    bookendMessage();
  }
}

void messageToBinary() {
  messageBin = textToBinary(message);
}

void bookendMessage() {
  // keep preamble and postamble
  messageBin = "1111111111111111111111110" + messageBin + "0111111111111111111111111";
}

String textToBinary(String text) { 
  String binary = ""; 
  binary.reserve(text.length() * 8 + 1); 
  for (int i = 0; i < text.length(); i++) { 
    char c = text[i]; 
    for (int j = 7; j >= 0; j--) { 
      binary += ((c >> j) & 1) ? '1' : '0'; 
    } 
  } 
  return binary; 
}
