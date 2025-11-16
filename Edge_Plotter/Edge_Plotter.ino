// Reciever code
const int analogPin = 8;             // Photodiode input

unsigned long raws[200];

bool printed = 0;
int n = 0;


void setup() {
  Serial.begin(115200);
}

void loop() {
  int raw = digitalRead(analogPin); 
  Serial.println(raw);

  // if (raw > 4 and raw < 1018) {
  //   raws[n] = raw;
  //   n++;
  //   printed = 1;
  // }

  // else if (printed){
  //   for (int i = 0; i < n; i++){
  //     Serial.println(raws[i]);
  //   }
  //   n=0;  
  //   printed = 0;
  // }

  delayMicroseconds(1000);


}