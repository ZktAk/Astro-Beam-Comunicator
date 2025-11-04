// === Manchester Transmitter ===

const int laserPin = 9;         // Laser output pin
const int clockPulseRate_Hz = 700;   // halfBitRate_Hz  // Transmission frequency (bits per second)
unsigned long clockPulse_ms = 1000 / clockPulseRate_Hz; 
unsigned long clockPulse_us = 1000000 / clockPulseRate_Hz; 

String endOfTransmission = "00000100"; // i.e. 0x04

bool mid = 1;
bool laserOn = 0;
bool nextBit = 0;

int n = 0;
int nTotal = 0;

String message = "";
String messageBin = "";

void setup() {
  Serial.begin(115200);
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
    nTotal = 0;
    return;
  }
  else {
    mid = 1;
  }

  while (messageBin.length() > 0) {
    mid = !mid;
    nextBit = messageBin[0] == '1';

    if (mid) {
      //if (nTotal == 0) Serial.println("");
      // midpoint: encode bit
      digitalWrite(laserPin, nextBit ? HIGH : LOW);
      //Serial.print(messageBin[0]);
      messageBin.remove(0, 1);  // advance after full period      
      // n++;
      // nTotal++;
      // if (n==8){
      //   Serial.print(" ");
      //   n=0;
      // }
      // if (nTotal==4) {
      //   n = 0;
      //   Serial.print(" ");
      // }
      // if (n==4){
      //   Serial.print("_");
      // }

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
  messageBin = "0101" + messageBin + endOfTransmission;
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
