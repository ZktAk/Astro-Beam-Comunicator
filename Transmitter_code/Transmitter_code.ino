// === Manchester Transmitter (String -> char[] version) ===
#include <Arduino.h>
#include <ctype.h>
#include <string.h>

const int laserPin = 9;         // Laser output pin
const int clockPulseRate_Hz = 1000;   // halfBitRate_Hz  // Transmission frequency (bits per second)
unsigned long clockPulse_ms = 1000 / clockPulseRate_Hz; 
unsigned long clockPulse_us = 1000000 / clockPulseRate_Hz; 

#define MAX_MSG_CHARS 64
#define BIN_BUF_SIZE 600

const char endOfTransmission[] = "00000100"; // i.e. 0x04

// message buffers (replaces String usage)
char message[MAX_MSG_CHARS + 1];
char messageBin[BIN_BUF_SIZE];
int messageBinLen = 0;   // length of messageBin (number of chars)
int messageBinIdx = 0;   // index of next bit to send

bool mid = 1;
bool laserOn = 0;
bool nextBit = 0;

int n = 0;
int nTotal = 0;

void setup() {
  Serial.begin(115200);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);  // laser off initially
  Serial.println("Transmitter ready. Please enter a message.");
  message[0] = '\0';
  messageBin[0] = '\0';
  messageBinLen = 0;
  messageBinIdx = 0;
}

void loop() {

  mid = !mid;  
  if (!mid) getMessage(); // check for new message

  // --- When idle, laser is off ---
  if (messageBinLen == 0) {
    digitalWrite(laserPin, LOW);
    delay(clockPulse_ms);
    nTotal = 0;
    return;
  }
  else {
    mid = 1;
  }

  while (messageBinIdx < messageBinLen) {
    mid = !mid;
    nextBit = messageBin[messageBinIdx] == '1';

    if (mid) {
      // midpoint: encode bit
      digitalWrite(laserPin, nextBit ? HIGH : LOW);
      messageBinIdx++;  // advance after full period      
    } else {
      // bit boundary: opposite of midpoint
      digitalWrite(laserPin, nextBit ? LOW : HIGH);
    }

    delayMicroseconds(clockPulse_us);
  }
  
  // finished sending -> reset buffer
  messageBinLen = 0;
  messageBinIdx = 0;

  delay(clockPulse_ms);
}

void trimWhitespace(char *s) {
  int start = 0;
  while (s[start] && isspace((unsigned char)s[start])) start++;
  int end = (int)strlen(s) - 1;
  while (end >= start && isspace((unsigned char)s[end])) end--;
  int newlen = 0;
  if (start <= end) {
    newlen = end - start + 1;
    if (start > 0) memmove(s, s + start, newlen);
  }
  s[newlen] = '\0';
}

void getMessage() {
  if (Serial.available() > 0) {
    int len = Serial.readBytesUntil('\n', message, MAX_MSG_CHARS);
    if (len <= 0) return;
    message[len] = '\0';
    trimWhitespace(message);
    if (message[0] == '\0') return;
    messageToBinary();
    bookendMessage();
  }
}

void messageToBinary() {
  textToBinary(message, messageBin, &messageBinLen);
  messageBinIdx = 0;
}

void bookendMessage() {
  const char preamble[] = "0101";
  char temp[BIN_BUF_SIZE];
  int newLen = 0;
  int preLen = (int)strlen(preamble);
  int eotLen = (int)strlen(endOfTransmission);

  // copy preamble
  for (int i = 0; i < preLen && newLen < BIN_BUF_SIZE - 1; i++) temp[newLen++] = preamble[i];
  // copy payload
  for (int i = 0; i < messageBinLen && newLen < BIN_BUF_SIZE - 1; i++) temp[newLen++] = messageBin[i];
  // copy EOT
  for (int i = 0; i < eotLen && newLen < BIN_BUF_SIZE - 1; i++) temp[newLen++] = endOfTransmission[i];

  temp[newLen] = '\0';
  memcpy(messageBin, temp, newLen + 1);
  messageBinLen = newLen;
  messageBinIdx = 0;
}

void textToBinary(const char *text, char *binBuf, int *outLen) {
  int len = (int)strlen(text);
  int idx = 0;
  for (int i = 0; i < len; i++) {
    unsigned char c = (unsigned char)text[i];
    for (int j = 7; j >= 0; j--) {
      if (idx < BIN_BUF_SIZE - 1) {
        binBuf[idx++] = ((c >> j) & 1) ? '1' : '0';
      } else {
        // buffer full; stop
        break;
      }
    }
  }
  binBuf[idx] = '\0';
  *outLen = idx;
}
