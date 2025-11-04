// === Manchester Receiver ===

const int sensorPin = A0;
const int THRESHOLD = 20;  // adjust based on light level (0–1023)
    
const int samplePeriod_us = 25; //1000000UL / (halfBitRate_Hz * oversampleFactor);

int pinADC;

bool bit;
bool lastBit = 0b0;


unsigned long prevDelta = 0;
unsigned long nowDelta = 0;

unsigned long lastTransitionTime = 0;
unsigned long now;

int recievedBits = 0;

bool mid;
bool halfBit;

String endOfTransmission = "00000100"; // i.e. 0x04
String currentByte = "";
int byteLength = 0;
String bitStream = "";
String decodedMessage = "";

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);
  Serial.println("Manchester Receiver Ready");
}

void loop() {
  
  while (recievedBits < 5){
    pinADC = analogRead(sensorPin);
    bit = pinADC < THRESHOLD;

    bool transition = bit^lastBit;   

    if (transition){
      recievedBits ++;
      // On recievedBits = 1: nowDelta = now - 0 and prevDelta = 0 are meaningless
      // On recievedBits = 2: nowDelta = now - lastTransitionTime is valid but prevDelta = now - 0 is meaningless
      // On recievedBits = 3: nowDelta = now - lastTransitionTime is valid and prevDelta = now - lastTransitionTime is valid

      now = micros();
      nowDelta = now - lastTransitionTime;

      // ratio = nowDelta / prevDelta

      lastTransitionTime = now;
      prevDelta = nowDelta;
    }
    lastBit = bit;

    // set mid and halfBit for when we exit out of while-loop. we don't want it to set every time the main loop runs
    mid = 1;
    halfBit = 0;

    if (recievedBits == 5) Serial.println("\nMessage Inbound");

    delayMicroseconds(samplePeriod_us);
  }
  

  pinADC = analogRead(sensorPin);
  bit = pinADC < THRESHOLD;

  bool transition = bit^lastBit;

  if (transition) {
    now = micros();
    nowDelta = now - lastTransitionTime;
    

    float ratio = (float)nowDelta / (float)prevDelta;
    //Serial.println(ratio);

    byte condLow  = (ratio < 0.75);  // 1 if true, else 0
    byte condHigh = (ratio > 1.5);   // 1 if true, else 0
    
    // Update halfBit based on conditions
    halfBit = condLow | (halfBit & ~condHigh);

    // mid logic:
    // - Set to 0 if condLow
    // - Set to 1 if condHigh
    // - Toggle if halfBit and not condLow or condHigh
    mid = (mid & ~(condLow | condHigh)) ^ (halfBit & ~(condLow | condHigh));
    mid |= condHigh;
    mid &= ~condLow;

    if (mid) {
      currentByte += bit ? "1" : "0";
      byteLength ++;
      //Serial.println(currentByte);      

      if (byteLength == 8){

        if (currentByte == endOfTransmission){
          Serial.println("Received Message:");
          Serial.println(bitStream);
          bitStream = "";
          recievedBits = 0;
          prevDelta = 0;
          nowDelta = 0;

          lastTransitionTime = 0;
        }
        else{
          char value = (char) strtol(currentByte.c_str(), NULL, 2);
          bitStream += value;  
        }
        
        currentByte = "";
        byteLength = 0;
      }
    }

    lastTransitionTime = now;
    prevDelta = nowDelta;
    lastBit = bit;
  }
  delayMicroseconds(samplePeriod_us);
}