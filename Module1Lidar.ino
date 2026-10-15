#include <Wire.h>

#define I2C_DETECTOR_ADDR 0x08

const int TRIG_PIN = 9;
const int ECHO_PIN = 10;

// --- CALIBRATION FOR 12 CM CHUTE ---
const float EMPTY_RAMP_DISTANCE = 12.0; // Distance to bare chute floor (cm)
const float SIZE_THRESHOLD      = 3.0;  // >= 3.0 cm is LARGE, < 3.0 cm is SMALL
const float DETECTION_MARGIN    = 1.0;  // Minimum parcel height to trigger (cm)
const float CLEAR_MARGIN        = 1.8;  // Height must drop below 1.8 cm to clear

// --- STATES ---
volatile bool isSystemRunning = true; // Default true so it runs immediately
volatile char queuedParcel    = '0';
bool objectPresent            = false;

unsigned long lastTelemetryTime = 0;

float readRawDistanceCM() {
  // Clear any residual charge on the pins
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(4);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 18ms timeout (~3 meters max range)
  long duration = pulseIn(ECHO_PIN, HIGH, 18000); 
  if (duration <= 0) return 0.0; // Timeout / deflected

  return (duration * 0.0343) / 2.0;
}

void receiveEvent(int howMany) {
  while (Wire.available()) {
    byte cmd = Wire.read();
    if (cmd == 1) {
      isSystemRunning = true;
      objectPresent   = false;
      queuedParcel    = '0';
      Serial.println(F("\n[I2C RX] Master commanded: START"));
    } else if (cmd == 0) {
      isSystemRunning = false;
      objectPresent   = false;
      queuedParcel    = '0';
      Serial.println(F("\n[I2C RX] Master commanded: STOP"));
    }
  }
}

void requestEvent() {
  Wire.write(queuedParcel);
  queuedParcel = '0'; // Master collected the parcel; clear queue
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Wire.begin(I2C_DETECTOR_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  Serial.println(F("=========================================="));
  Serial.println(F("  Node 1: Chute Detector (Telemetry Mode) "));
  Serial.println(F("=========================================="));
}

void loop() {
  if (!isSystemRunning) {
    delay(100);
    return;
  }

  float distance = readRawDistanceCM();

  // If sensor times out (0) or reads higher than empty chute + margin, treat as bare chute
  if (distance <= 1.0 || distance > (EMPTY_RAMP_DISTANCE + 4.0)) {
    distance = EMPTY_RAMP_DISTANCE;
  }

  float parcelHeight = EMPTY_RAMP_DISTANCE - distance;

  // 1. TRIGGER: Parcel enters beam
  if (!objectPresent && parcelHeight >= DETECTION_MARGIN && queuedParcel == '0') {
    if (parcelHeight >= SIZE_THRESHOLD) {
      queuedParcel = 'L';
      Serial.print(F("\n>>> [DETECTED LARGE] Height: "));
    } else {
      queuedParcel = 'S';
      Serial.print(F("\n>>> [DETECTED SMALL] Height: "));
    }
    Serial.print(parcelHeight, 1);
    Serial.println(F(" cm"));

    objectPresent = true; // Lock detector until this parcel clears
  }

  // 2. UNLATCH: Parcel has slid away
  if (objectPresent && parcelHeight < CLEAR_MARGIN) {
    objectPresent = false;
    Serial.println(F("[BEAM CLEARED] Chute empty -> Ready for next parcel\n"));
  }

  // 3. LIVE SENSOR TELEMETRY (Prints every 300ms so you can monitor the beam)
  if (millis() - lastTelemetryTime >= 300) {
    lastTelemetryTime = millis();
    Serial.print(F("Raw: "));
    Serial.print(distance, 1);
    Serial.print(F(" cm | Height: "));
    Serial.print(parcelHeight, 1);
    Serial.print(F(" cm | Latched: "));
    Serial.print(objectPresent ? F("YES") : F("NO "));
    Serial.print(F(" | Queue: "));
    Serial.println(queuedParcel);
  }

  delay(60); // 60ms gap prevents acoustic crosstalk
}