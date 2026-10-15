#include <Wire.h>

// Modular Test Flags
const bool ENABLE_DETECTOR = true;
const bool ENABLE_SERVO    = true;
const bool ENABLE_BINS     = true; // Set to true once Node 3 is plugged in

// I2C Addresses
#define ADDR_DETECTOR 0x08
#define ADDR_SERVO    0x09  
#define ADDR_BINS     0x0A

// Hardware Pins
const int BTN_START = 2;
const int BTN_STOP  = 3;
const int LED_GREEN = 6;
const int LED_RED   = 7;

// System State
bool isSystemRunning = false;
bool lastStartReading = HIGH;
bool lastStopReading  = HIGH;

const unsigned long POLL_INTERVAL_MS = 150;
const unsigned long FLAP_HOLD_MS     = 700;

unsigned long lastPollTime = 0;
unsigned long flapOpenTime = 0;
bool isFlapDiverted        = false;

void broadcastRunState(bool state) {
  byte code = state ? 1 : 0;
  if (ENABLE_DETECTOR) {
    Wire.beginTransmission(ADDR_DETECTOR);
    Wire.write(code);
    Wire.endTransmission();
  }
  if (ENABLE_SERVO) {
    Wire.beginTransmission(ADDR_SERVO);
    Wire.write(code);
    Wire.endTransmission();
  }
  if (ENABLE_BINS) {
    Wire.beginTransmission(ADDR_BINS);
    Wire.write(code);
    Wire.endTransmission();
  }
}

void sendServoCommand(char target) {
  if (!ENABLE_SERVO) return;
  Wire.beginTransmission(ADDR_SERVO);
  Wire.write(target);
  byte err = Wire.endTransmission();

  if (err != 0) {
    Serial.print(F("[I2C ERR] Servo unreachable (0x09). Code: "));
    Serial.println(err);
  }
}

// byte pollBins() {
//   if (!ENABLE_BINS) return 0;
//   Wire.requestFrom(ADDR_BINS, 1);
//   if (Wire.available()) {
//     return Wire.read();
//   }
//   return 0;
// }
byte pollBins() {
  if (!ENABLE_BINS) return 0;

  byte received = Wire.requestFrom((uint8_t)ADDR_BINS, (uint8_t)1);

  if (received == 1 && Wire.available()) {
    byte status = Wire.read();

    if (status <= 3) {
      return status;
    }
  }

  Serial.println("[I2C ERROR] Bin monitor unavailable");
  return 255; // Unknown status: block automatic sorting
}

char pollDetector() {
  if (!ENABLE_DETECTOR) return '0';
  Wire.requestFrom(ADDR_DETECTOR, 1);
  if (Wire.available()) {
    return Wire.read();
  }
  return '0';
}

void setLEDs(bool greenLed, bool redLed) {
  digitalWrite(LED_GREEN, greenLed ? HIGH : LOW);
  digitalWrite(LED_RED,   redLed   ? HIGH : LOW);
}

void setup() {
  Serial.begin(9600);

  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_STOP,  INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED,   OUTPUT);

  setLEDs(false, false);
  Wire.begin();

  Serial.println(F("=================================================="));
  Serial.println(F("         NODE 4 MASTER CONTROLLER ONLINE          "));
  Serial.println(F("=================================================="));
}

void loop() {
  // 1. Hardware Buttons
  bool startReading = digitalRead(BTN_START);
  bool stopReading  = digitalRead(BTN_STOP);

  if (lastStartReading == HIGH && startReading == LOW) {
    if (!isSystemRunning) {
      Serial.println(F("[EVENT] START Pressed -> Modules Active"));
      isSystemRunning = true;
      setLEDs(true, false);
      broadcastRunState(true);
      sendServoCommand('C');
      isFlapDiverted = false;
    }
    delay(50);
  }

  if (lastStopReading == HIGH && stopReading == LOW) {
    if (isSystemRunning) {
      Serial.println(F("[EVENT] STOP Pressed -> Modules Dormant"));
      isSystemRunning = false;
      setLEDs(false, false);
      broadcastRunState(false);
      sendServoCommand('C');
      isFlapDiverted = false;
    }
    delay(50);
  }

  lastStartReading = startReading;
  lastStopReading  = stopReading;

  // 2. Serial Console Mocking

  if (Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd == '1' && !isSystemRunning) {
      isSystemRunning = true;
      setLEDs(true, false);
      broadcastRunState(true);
      sendServoCommand('C');
      isFlapDiverted = false;
    }
    else if (cmd == '0' && isSystemRunning) {
      isSystemRunning = false;
      setLEDs(false, false);
      broadcastRunState(false);
      sendServoCommand('C');
      isFlapDiverted = false;
    }
    else if (cmd == 's' || cmd == 'S') {
      if (isSystemRunning && pollBins() == 0) {
        sendServoCommand('S');
      } else {
        Serial.println("[BLOCKED] System stopped or bins full");
      }
    }
    else if (cmd == 'l' || cmd == 'L') {
      if (isSystemRunning && pollBins() == 0) {
        sendServoCommand('L');
      } else {
        Serial.println("[BLOCKED] System stopped or bins full");
      }
    }
    else if (cmd == 'c' || cmd == 'C') {
      sendServoCommand('C');
    }
  }

  // 3. Autonomous Flap Auto-Reset (Non-blocking)
  if (isFlapDiverted && (millis() - flapOpenTime >= FLAP_HOLD_MS)) {
    sendServoCommand('C');
    isFlapDiverted = false;
    Serial.println(F("[AUTO] Parcel passed -> Flap recentered"));
  }

      // 4. Polling Pipeline
  if (isSystemRunning &&
      (millis() - lastPollTime >= POLL_INTERVAL_MS)) {

    lastPollTime = millis();

    // Check bin status
    byte binStatus = pollBins();

    Serial.print("[DEBUG] Bin status: ");
    Serial.println(binStatus);

    // Block sorting when bin is full or I2C fails
    if (binStatus != 0) {

      Serial.print("[WARNING] Bin full! Status: ");
      Serial.println(binStatus);

      // Stop the entire system
      isSystemRunning = false;

      // Red LED ON, Green LED OFF
      setLEDs(false, true);

      // Tell all nodes to stop
      broadcastRunState(false);

      // Return servo to center
      sendServoCommand('C');

      // Reset flap state
      isFlapDiverted = false;

      Serial.println("[SYSTEM] STOPPED - Empty bins and press START");

      return;
}

    // Both bins available
    setLEDs(true, false);

    // Detect new parcel
    if (!isFlapDiverted) {
      char parcelSize = pollDetector();

      Serial.print("[MASTER] Parcel size received: ");
      Serial.println(parcelSize);

      if (parcelSize == 'S') {
        Serial.println("[ROUTE] SMALL parcel");

        sendServoCommand('S');

        isFlapDiverted = true;
        flapOpenTime = millis();
      }
      else if (parcelSize == 'L') {
        Serial.println("[ROUTE] LARGE parcel");

        sendServoCommand('L');

        isFlapDiverted = true;
        flapOpenTime = millis();
      }
    }
  }
}