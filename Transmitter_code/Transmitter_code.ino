// === Manchester Transmitter ===

const int laserPin = 9;         // Laser output pin
const int clockPulseRate_Hz = 700;   // halfBitRate_Hz  // Transmission frequency (bits per second)
unsigned long clockPulse_ms = 1000 / clockPulseRate_Hz; 
unsigned long clockPulse_us = 1000000 / clockPulseRate_Hz; 

int8_t END_OF_TRANSMISSION = 0x4; // i.e. 0x04
#define CHARACTER_LIMIT 64

bool nextBit;

String message = "";
uint8_t messageBinary[CHARACTER_LIMIT];
int byteIndex = 0;
bool sending = 0;

void setup() {
  Serial.begin(115200);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);  // laser off initially
  Serial.println("Transmitter ready. Please enter a message.");
}

void loop() {

  getMessage();

  // --- When idle, laser is off ---
  if (!sending) {
    digitalWrite(laserPin, LOW);
    delay(clockPulse_ms);
    return;
  }

  for (int i = 0; i < byteIndex+1; i++) {    
    
    for (int j = 7; j >= 0; j--){      
      nextBit = (messageBinary[i] >> j) & 0b1; 

      digitalWrite(laserPin, nextBit ? LOW : HIGH);
      delayMicroseconds(clockPulse_us);
      //delay(clockPulse_ms);

      digitalWrite(laserPin, nextBit ? HIGH : LOW);
      delayMicroseconds(clockPulse_us);
      //delay(clockPulse_ms);  
    }
  }  
  sending = 0;
}

void getMessage() {
  if (Serial.available() > 0) {
    message = Serial.readStringUntil('\n');
    message.trim();
    if (message.length() == 0){      
      sending = 0;
      return;
    }
    Serial.println("\nTransmitting Message:");
    Serial.println(message);
    sending = 1;
    messageToBinary();
    bookendMessage();
  }
}

void messageToBinary() {
  byteIndex = 0;

  for (int i = 0; i < message.length(); i++) { 
    char c = message[i];
    uint8_t currentByte = 0b0;
    byteIndex = i+1;
    for (int j = 0; j < 8; j++) {
      bool bit = (c >> (7-j)) & 0b1;
      currentByte = (currentByte << 1) | bit;
    }
    messageBinary[byteIndex] = currentByte;
  }
}

void bookendMessage() {
  // keep preamble and postamble

  uint8_t temp = 0b10101010;
  messageBinary[0] = temp;
  messageBinary[byteIndex+1] = END_OF_TRANSMISSION;
  byteIndex++;
}