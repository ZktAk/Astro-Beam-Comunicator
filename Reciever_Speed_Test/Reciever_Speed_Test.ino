// ===== Receiver Using Timer1 Input Capture for Manchester Timing =====
//
//  Comparator output MUST be connected to Arduino UNO pin 8 (ICP1)
//  This gives hardware-timestamped edges with 62.5 ns resolution.
//
// ===============================================================

const int ICP_PIN = 8;      // Dedicated Input Capture pin
const int testBits = 100;

volatile uint16_t timestamps[testBits];
volatile int transitionCount = 0;
volatile bool calibrationDone = false;

unsigned long bitDuration_ticks = 0;   // Bit duration in Timer1 ticks

// ---------- INPUT CAPTURE ISR ----------
// Fires on rising OR falling edge (we will toggle edge in software).
ISR(TIMER1_CAPT_vect) {
    if (calibrationDone) return;

    if (transitionCount < testBits) {
        timestamps[transitionCount] = ICR1;   // Hardware timestamp
        transitionCount++;

        if (transitionCount >= testBits) {
            calibrationDone = true;
        }

        // Toggle edge detection so we capture both edges
        TCCR1B ^= (1 << ICES1);    // Flip ICES1 bit
    }
}


// =================== SETUP ====================
void setup() {
    Serial.begin(115200);
    pinMode(ICP_PIN, INPUT);

    Serial.println("Receiver ready. Waiting for calibration pattern...");

    // -------- Timer1 setup --------
    TCCR1A = 0;              // Normal mode
    TCCR1B = 0;

    TCCR1B |= (1 << CS10);   // No prescaler → 16 MHz clock = 62.5 ns ticks
    TCCR1B |= (1 << ICES1);  // Initially capture rising edge

    TIMSK1 |= (1 << ICIE1);  // Enable Input Capture interrupt
}


// =================== CALIBRATION ====================
void calibrateReceiver() {
    transitionCount = 0;
    calibrationDone = false;

    Serial.println("\nWaiting for calibration pattern (alternating bits)...");

    // Wait until ISR fills the buffer
    while (!calibrationDone) {
        // Do nothing — hardware collects timestamps
    }

    // Compute average delta between transitions
    unsigned long first = timestamps[0];
    unsigned long last  = timestamps[testBits - 1];
    unsigned long totalTicks = (uint16_t)(last - first);

    bitDuration_ticks = totalTicks / (testBits);

    Serial.println("Calibration complete!");
    Serial.print("Measured bitDuration_ticks = ");
    Serial.println(bitDuration_ticks);

    // Convert to microseconds if you want:
    float bitDuration_us = bitDuration_ticks * 0.0625; // 1 tick = 62.5 ns
    Serial.print("Measured bitDuration_us ≈ ");
    Serial.println(bitDuration_us);

    Serial.print("Total Transmission time (us) = ");
    Serial.println(bitDuration_us * 100);
}


// =================== MAIN LOOP ====================
void loop() {
    calibrateReceiver();

    // Continue into normal Manchester decoding here…
}
