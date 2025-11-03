// Reciever code
const int analogPin = A0;             // Photodiode input

unsigned long raws[200];

bool printed = 0;
int n = 0;


void setup() {
  Serial.begin(115200);
}

void loop() {
  int raw = analogRead(analogPin); 
  Serial.println(raw);

  // if (raw > 0 and raw < 930) {
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

  delayMicroseconds(100);


}