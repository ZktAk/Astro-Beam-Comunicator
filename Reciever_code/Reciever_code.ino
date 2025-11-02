// === Manchester Receiver ===

const int sensorPin = A0;
const int THRESHOLD = 50;  // adjust based on light level (0–1023)
    
const int halfBitRate_Hz = 10;
const int oversampleFactor = 10;
const unsigned long samplePeriod_us = 1000000UL / (halfBitRate_Hz * oversampleFactor);

bool lastLevel = 0;
unsigned long lastTransitionTime = 0;
unsigned long lastHalfBit_us = 0;

bool halfBit = 0;
bool mid = 1;

unsigned long prevDelta = 0;
unsigned long nowDelta = 0;

int transitionCount = 0;

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
    //Serial.println("RECIEVING!");

    lastLevel = 0;
    lastTransitionTime = 0;

    halfBit = 1;
    mid = 0;

    prevDelta = 0;
    nowDelta = 0;

    receiving = true;
    bitStream = "";

    transitionCount = 0;

    Serial.println("Signal detected, starting reception...");
  }

  if (!receiving) {
    lastLevel = currentLevel;
    delayMicroseconds(samplePeriod_us);
    return;
  }

  // Detect transitions
  if (currentLevel != lastLevel) {  
    transitionCount++;
    unsigned long now = micros();
    nowDelta = now - lastTransitionTime;

    // Delta 1: Calculated at transitionCount=1. Measures between receiving=False and receiving=True. Value is micros()-0. Verdict: Meaningless, skip
    // Delta 2: Calculated at transitionCount=2. Measures between transtion1 and transition2. Value is micros() - old_micros. Verdict: Valid, but prevDelta is still Meaningless, so still skip
    // Delta 3: Calculated at transitionCount=3. Measures between transtion2 and transition3. Value is micros() - old_micros. Verdict: Valid, and prevDelta is also Valid, so do not skip

    if (transitionCount < 3){ // if we are currently skipping deltas due to the criteria above, we know that we are in the preamble, halfBit domain, and each transition therefore needs a toggle of mid.
      halfBit = 1;
      mid = !mid;
    }

    else if (transitionCount >= 3 && prevDelta <= 0) {
      Serial.println("ERROR!!!!");
    }      

    else {
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
    }    
    prevDelta = nowDelta;
    lastTransitionTime = now;            
  }

  if (mid) {
    bool bitValue = currentLevel;
    bitStream += bitValue ? '1' : '0';
    //Serial.println(bitStream);

    // Check for preamble
    if (bitStream.endsWith("111111110") && bitStream.length() <= 18) {
      Serial.println("Preamble detected...");
      bitStream = "";
    }

    // Check for postamble
    if (bitStream.endsWith("011111111") && bitStream.length() > 18) {
      receiving = false;
      decodedMessage = decodeBinaryToText(bitStream.substring(0, bitStream.length() - 9));
      Serial.println("Received Message:");
      Serial.println(bitStream.substring(0, bitStream.length() - 9));
      Serial.println(decodedMessage);
      bitStream = "";
      mid = 0;
      Serial.println("Reception complete.");
    }
  }
  lastLevel = currentLevel;
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
