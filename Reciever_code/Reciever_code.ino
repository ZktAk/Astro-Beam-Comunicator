// === Manchester Receiver ===

const int sensorPin = A0;
const int THRESHOLD = 50;  // adjust based on light level (0–1023)
    
const int halfBitRate_Hz = 100; // only used to derive sampling rate
const int oversampleFactor = 10; // >= 4x faster than half-bit rate. This makes sure that we are sampling at a higher rate than we expect data to be recieved so that we never miss a transition, regardless of timing drift.
const unsigned long samplePeriod_us = 1000000UL / (halfBitRate_Hz * oversampleFactor);

bool lastLevel = 0;
unsigned long lastTransitionTime = 0;
unsigned long lastHalfBit_us = 0;

bool halfBit = 0; // on startup, we know that we will be recieved a manchester encoded 10101010... stream from the transmitter. Meaning that we are recieveing only fullbit deltas.
bool mid = 1; // on startup, we know that we will be recieved a manchester encoded 10101010... stream from the transmitter. Meaning that we are only recieving midpoint transitions
// Something to note here is that if we were to startup here and in the
// transmitter with sending just encoded 0's, we would know that we would start
// up with recieveing only halfbit deltas, but crucially we wouldn't know which
// transitions are boundry or midpoint transitions untill we recieve a fullbit delta.
// Therefore, depending on the initial phase offset, the reciver could interpret
// the bit stream as 1's or 0's, even though the transmitter is sending 0's.
// If we are offset and are seeing a bit stream oif 1's instead of 0's, that
// will mess up our preamble detection - all decoded idle bits are 1's instead 
// of 0's and we detect a preamble and then immediatly a postamble when actually
// neither where actyually sent. Crucually, this can be avoided by sending
// alternating encoded 1's and 0's on startup, allowing the reciever to assume
// that each delta is a fullbit delta and is therefore recieved at the midpoit.
// Therefore, there is no ambiguity.


unsigned long prevDelta = 0;
unsigned long nowDelta = 0; // just make sure that this is initially longer than anything we would expect

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
    //Serial.println(nowDelta);
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
      //Serial.print(currentLevel);
      //Serial.println(mid ? ": mid" : ": boundry");     

      if (mid) {
        // Rising midpoint = bit '1'
        // Falling midpoint = bit '0'
        bool bitValue = currentLevel; // rising=HIGH=true=1, falling=LOW=false=0
        bitStream += bitValue ? '1' : '0';
        //Serial.println(bitValue);

        // Check for preamble (start of message)
        if (!receiving && bitStream.endsWith("0111111110")) {
          receiving = true;
          bitStream = "";  // clear buffer, start fresh
          Serial.println("Preamble detected. Receiving...");
        }

        // Check for postamble (end of message)
        if (receiving && bitStream.endsWith("0111111110")) {
          receiving = false;
          decodedMessage = decodeBinaryToText(bitStream.substring(0, bitStream.length() - 10));
          Serial.println("Received Message:");
          Serial.println(bitStream.substring(0, bitStream.length() - 10));
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