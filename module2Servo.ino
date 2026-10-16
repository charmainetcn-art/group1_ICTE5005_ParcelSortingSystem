#include <Wire.h>
#include <Servo.h>

///////////////////////////
// for I2C
///////////////////////////
#define I2C_SERVO_ADDR 0x09

///////////////////////////
// for Servo pins
///////////////////////////
Servo sorterFlap;
const int SERVO_PIN = 9;

///////////////////////////
// calibration (in degrees)
///////////////////////////
// SET HERE
// Chute angles (adjust to fit your physical frame)
const int FLAP_SMALL  = 45;
const int FLAP_CENTER = 90;
const int FLAP_LARGE  = 135;

///////////////////////////
// initial state
///////////////////////////
int currentAngle = FLAP_CENTER;
volatile char pendingCommand = 0;

///////////////////////////
// functions 
///////////////////////////
void setFlapSmooth(int targetAngle){
  // if no change, do nothing
  if ( targetAngle == currentAngle) return;
  int step = (targetAngle > currentAngle) ? 1 : -1;

  // move the servo smoothly
  for (int pos = currentAngle; pos != targetAngle; pos += step) {
    sorterFlap.write(pos);
    delay(5);
  }
  sorterFlap.write(targetAngle);
  currentAngle = targetAngle;
}

void receiveEvent(int howMany) {
  while (Wire.available()){
    char cmd = Wire.read();

    // start servo
    if (cmd == 1) {
      Serial.println("[I2C] Master commanded: START");
    } 
    // end servo
    else if (cmd == 0) {
      Serial.println("[I2C] Master commanded: STOP -> Centering Flap");
      pendingCommand = 'C';
    } else {
      // Receives 'S', 'L', or 'C' from Master (including mock inputs)
      pendingCommand = cmd;
    }
  }
}

///////////////////////////
// main
///////////////////////////
void setup() {
  Serial.begin(9600);

  // used to limit the angle of the servo
  // 544 = minimum pulse width in microseconds
  // 2400 = maximum pulse width in microseconds
  sorterFlap.attach(SERVO_PIN, 544, 2400);
  sorterFlap.write(currentAngle);

  Wire.begin(I2C_SERVO_ADDR);
  Wire.onReceive(receiveEvent);

  Serial.println("--- Node 2 (Flap 0x09) Initialized: DORMANT ---");
}

void loop() {
  if (pendingCommand != 0) {
    char cmd = pendingCommand;
    pendingCommand = 0; // Clear the pending flag

    switch (cmd) {
      case 'S':
      case 's':
        Serial.println("I2C Received: Route to SMALL");
        setFlapSmooth(FLAP_SMALL);
        break;

      case 'L':
      case 'l':
        Serial.println("I2C Received: Route to LARGE");
        setFlapSmooth(FLAP_LARGE);
        break;

      case 'C':
      case 'c':
        Serial.println("I2C Received: Reset to CENTER");
        setFlapSmooth(FLAP_CENTER);
        break;
    }

    delay(20);
  }
}
