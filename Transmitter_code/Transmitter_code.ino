// === Manchester Transmitter ===

const int laserPin = 9;         // Laser output pin
const int clockPulseRate_Hz = 900;   // halfBitRate_Hz  // Transmission frequency (bits per second)
unsigned long clockPulse_ms = 1000 / clockPulseRate_Hz; 
unsigned long clockPulse_us = 1000000 / clockPulseRate_Hz; 

int8_t END_OF_TRANSMISSION = 0x4; // i.e. 0x04

bool nextBit;

String message = "";
uint64_t messageBinary = 0b0;
int messageSize = 0;
int msbIndex = 0;

void setup() {
  Serial.begin(115200);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);  // laser off initially
  Serial.println("Transmitter ready. Please enter a message.");
}

void loop() {

  getMessage();

  // --- When idle, laser is off ---
  if (messageSize == 0) {
    digitalWrite(laserPin, LOW);
    delay(clockPulse_ms);
    return;
  }
  
  // Serial.println("");

  for (int i = msbIndex; i >= 0; i--) {
    nextBit = (messageBinary >> i) & 0b1;

    // Serial.print(nextBit);    
    // if (i % 8 == 0){
    //   Serial.print(" ");
    // }
    // else if (i==4) {
    //   Serial.print(" ");
    // }
    // else if (i % 4 == 0){
    //   Serial.print("_");
    // }

    digitalWrite(laserPin, nextBit ? LOW : HIGH);
    delayMicroseconds(clockPulse_us);
    //delay(clockPulse_ms);

    digitalWrite(laserPin, nextBit ? HIGH : LOW);
    delayMicroseconds(clockPulse_us);
    //delay(clockPulse_ms);  
    messageSize--;
  }
}

void getMessage() {
  if (Serial.available() > 0) {
    message = Serial.readStringUntil('\n');
    message.trim();
    if (message.length() == 0){      
      messageSize = 0;
      msbIndex = messageSize - 1;
      return;
    }
    Serial.println("\nTransmitting Message:");
    Serial.println(message);
 
    messageToBinary();
    bookendMessage();
  }
}

void messageToBinary() {
  messageBinary = 0b0;
  messageSize = 0;

  for (int i = 0; i < message.length(); i++) { 
    char c = message[i];
    for (int j = 0; j < 8; j++) {
      bool bit = (c >> (7-j)) & 0b1;
      messageBinary = (messageBinary << 1) | bit;
      messageSize++;
    }
  }
}

void bookendMessage() {
  // keep preamble and postamble
  uint64_t temp = 0b101;
  temp = (temp << messageSize) | messageBinary;
  messageSize += 3;
  messageBinary = (temp << 0x8) | END_OF_TRANSMISSION;
  messageSize += 8;
  msbIndex = messageSize - 1;


  // for (int i = msbIndex; i >= 0; i--) {
  //   bool bit = (messageBinary >> i) & 0b1;
  //   Serial.print(bit);
  //   if (i % 8 == 0) Serial.print(' '); // Optional: add space every byte
  // }
}