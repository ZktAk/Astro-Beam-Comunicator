// === Manchester Receiver ===

const int sensorPin = A0;
const int THRESHOLD = 20;  // adjust based on light level (0–1023)
    
const int halfBitRate_Hz = 500;
const int oversampleFactor = 20;
const unsigned long samplePeriod_us = 1000000UL / (halfBitRate_Hz * oversampleFactor);

bool lastLevel = 0;
unsigned long lastTransitionTime = 0;
unsigned long lastHalfBit_us = 0;

bool halfBit = 0;
bool mid = 1;

unsigned long prevDelta = 0;
unsigned long nowDelta = 0;

String bitStream = "";
String decodedMessage = "";
bool receiving = false;

void setup() {
  Serial.begin(9600);
  pinMode(sensorPin, INPUT);
  Serial.println("Manchester Receiver Ready");
}

void loop() {
  // Read and threshold sensor
  int sensorValue = analogRead(sensorPin);
  bool currentLevel = sensorValue < THRESHOLD;

  // Start recording on first HIGH
  if (!receiving && currentLevel) {
    receiving = true;
    bitStream = "";
    
    Serial.println("\nSignal detected, starting reception...");
  }

  if (!receiving) {
    lastLevel = currentLevel;
    delayMicroseconds(samplePeriod_us);
    return;
  }

  // Detect transitions
  if (currentLevel != lastLevel) {    
    unsigned long now = micros();    
    nowDelta = now - lastTransitionTime;

    if (prevDelta > 0) {
      float ratio = (float)nowDelta / (float)prevDelta;

      if (ratio < 0.75) {
        halfBit = 1;
        mid = 0;
      }
      else if (ratio > 1.5) { 
        halfBit = 0;
        mid = 1;
      }
      else if (halfBit) { 
        mid = !mid;
      }

      if (mid) {
        bool bitValue = currentLevel;
        bitStream += bitValue ? '1' : '0';

        // Check for preamble
        if (bitStream.endsWith("1111111111111111111111110") && bitStream.length() <= 50) {
          Serial.println("Preamble detected...");
          bitStream = "";
        }

        // Check for postamble
        if (bitStream.endsWith("0111111111111111111111111") && bitStream.length() > 50) {
          receiving = false;
          decodedMessage = decodeBinaryToText(bitStream.substring(0, bitStream.length() - 25));
          Serial.println("Received Message:");
          //Serial.println(bitStream.substring(0, bitStream.length() - 9));
          Serial.println(decodedMessage);
          bitStream = "";
          lastLevel = 0;
          //Serial.println("Reception complete.");
        }
      }    
    }   

    prevDelta = nowDelta;
    lastTransitionTime = now;
    lastLevel = currentLevel;
  }

  delayMicroseconds(samplePeriod_us);
}

String decodeBinaryToText(String bits) {
  String text = "";
  for (int i = 0; i + 7 < bits.length(); i += 8) {
    byte c = 0;
    for (int j = 0; j < 8; j++) {
      c = (c << 1) | (bits[i + j] == '1');
    }
    text += char(c);
  }
  return text;
}