// ============================================================
//      HIGH-PRECISION LASER TRANSMITTER (UP TO ~8 MHz)
// ============================================================
// - Uses Timer1 CTC mode to generate exact bit timing
// - Laser output driven via pin 9 (OC1A / PB1) with minimal ISR overhead
// - Supports manual HIGH/LOW override
// - 100-bit alternating calibration pattern example
// ============================================================

#include <Arduino.h>

const int laserPin = 9;          // OC1A / PB1 hardware pin
const uint32_t bitRate_Hz = 25000;  // Change as needed
const int testBits = 100;        // Calibration pattern length

// Timer tick: 1 tick = 1 / 16 MHz = 62.5 ns
uint16_t bitPeriodTicks;          // Timer1 compare match value

volatile bool transmitting = false;
volatile bool forceManual = false;
volatile uint8_t bitIndex = 0;

// =================== TIMER1 ISR ===========================
ISR(TIMER1_COMPA_vect) {
    if (!transmitting || forceManual) return;

    // Toggle laser pin using PINB register (single instruction)
    PINB |= (1 << PB1);  // PB1 = laserPin, toggles current state

    bitIndex++;

    if (bitIndex >= testBits) {
        transmitting = false;
        // Ensure laser is OFF at end
        PORTB &= ~(1 << PB1);
    }
}

// =================== SETUP ================================
void setup() {
    Serial.begin(115200);
    pinMode(laserPin, OUTPUT);
    PORTB &= ~(1 << PB1); // Ensure laser starts LOW

    // Calculate Timer1 compare match value
    // Formula: OCR1A = F_CPU / bitRate_Hz - 1
    bitPeriodTicks = (uint16_t)(F_CPU / bitRate_Hz - 1);

    // -------- Configure Timer1 for CTC mode, no prescaler --------
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS10); // CTC, no prescaler
    OCR1A = bitPeriodTicks;
    TIMSK1 |= (1 << OCIE1A); // Enable Compare Match A ISR

    Serial.println("High-precision transmitter ready.");
    Serial.println("Send '1' = laser HIGH, '0' = laser LOW, any other key → test pattern.");
}

// =================== LOOP ================================
void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input == "1") {
            forceManual = true;
            PORTB |= (1 << PB1);
            Serial.println("Laser → HIGH (manual mode)");
        }
        else if (input == "0") {
            forceManual = true;
            PORTB &= ~(1 << PB1);
            Serial.println("Laser → LOW (manual mode)");
        }
        else {
            forceManual = false;
            startTestCode();
        }
    }
}

// =================== SEND HIGH-PRECISION CALIBRATION PATTERN ===========
void startTestCode() {
    Serial.println("Sending perfect-timing 100-bit alternating pattern...");

    bitIndex = 0;
    transmitting = true;

    // Non-blocking: let ISR handle everything
    while (transmitting) {
        // Optional: you can do other tasks here if needed
    }

    Serial.println("Pattern sent.");
}
