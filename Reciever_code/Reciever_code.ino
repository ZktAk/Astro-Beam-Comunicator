// === Manchester Receiver ===

const int sensorPin = A0;
const int THRESHOLD = 50;  // adjust based on light level (0–1023)
    
const int halfBitRate_Hz = 1000; // only used to derive sampling rate
const int oversampleFactor = 4; // >= 4x faster than half-bit rate. This makes sure that we are sampling at a higher rate than we expect data to be recieved so that we never miss a transition, regardless of timing drift.
const unsigned long samplePeriod_us = 1000000UL / (halfBitRate_Hz * oversampleFactor);

bool lastLevel = 0;
unsigned long lastTransitionTime = 0;
unsigned long lastHalfBit_us = 0;
bool halfBit = 1; // on startup, we know that we will be recieved a manchester encoded 0 stream from the transmitter. Meaning that we are recieveing only halfbit deltas.
bool mid = 0;

unsigned long prevDelta = 0;
unsigned long nowDelta = 10 * oversampleFactor * samplePeriod_us; // just make sure that this is initially longer than anything we would expect

String bitStream = "";
String decodedMessage = "";
bool receiving = false;

void setup() {
  Serial.begin(9600);
  pinMode(sensorPin, INPUT);
  Serial.println("Manchester Receiver Ready");
}

void loop() {
  // Read and threshold the sensor
  int sensorValue = analogRead(sensorPin);
  bool currentLevel = sensorValue < THRESHOLD; // if sensorValue < THRESHOLD then bit is a 1, else 0
  
  // Detect transitions (edges)
  if (currentLevel != lastLevel) {    
    unsigned long now = micros();    
    nowDelta = now - lastTransitionTime;
    //Serial.println(currentLevel ? '1' : '0');

    if (prevDelta > 0) {
      float ratio = (float)nowDelta / (float)prevDelta;

      if (ratio < 0.75) {
        // if delta is shorter than before, 
        // we are going into the halfBit domain 
        // and we are not at a mid-bit transition.
        // The logic here is that the longer deltas
        // are from midpoint to midpoint, and if we
        // then encounter a shorter delta that means
        // we are not yet at the next midpoint but
        // instead are at a bit edge.
        halfBit = 1;
        mid = 0;
      }
      else if (ratio > 1.5) { 
        // if delta is longer than before, we are 
        // going into the fullBit domain and we are 
        // indeed at a mid-bit transition. The logic 
        // here is that the longer deltas are from 
        // midpoint to midpoint, skipping the edge
        // transitions between bits.
        halfBit = 0;
        mid = 1;
      }
      else if (halfBit) { 
        // if the above two if's are false, then the delta is same as previouse.
        // Therefore, we may be in the halfbit domain or we may be in the fullbit domain. We
        // look at the value of "halfBit" to determine which domain we are in.
        // If we are in the fullwidth domain, then we are at a midpoint, and "mid"
        // would have already been set to 1 in the previouse ratio > 1.5, and we
        // have nothing we need to do in this "else if". BUT, if we are in the halfbit domain
        // then we need to invert "mid".
        mid = !mid;
      }

      if (mid) {
        // Rising midpoint = bit '1'
        // Falling midpoint = bit '0'
        bool bitValue = currentLevel; // rising=HIGH=true=1, falling=LOW=false=0
        bitStream += bitValue ? '1' : '0';

        // Check for preamble (start of message)
        if (!receiving && bitStream.endsWith("11111111")) {
          receiving = true;
          bitStream = "";  // clear buffer, start fresh
          Serial.println("Preamble detected. Receiving...");
        }

        // Check for postamble (end of message)
        if (receiving && bitStream.endsWith("11111111")) {
          receiving = false;
          decodedMessage = decodeBinaryToText(bitStream.substring(0, bitStream.length() - 8));
          Serial.print("Received Message: ");
          Serial.println(decodedMessage);
          bitStream = "";
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
