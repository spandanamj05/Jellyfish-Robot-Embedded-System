#include <IRremote.h>

#define PLAY_PAUSE 0xFD60AF
#define STOP_MODE  0xFD609F
#define BUTTON_1   0xBA45FF00 // straight
#define BUTTON_2   0xB946FF00 // turn right
#define BUTTON_3   0xB847FF00 // turn left
#define BUTTON_4   0xBB44FF00
#define BUTTON_5   0xBF40FF00
#define BUTTON_6   0xBC43FF00
#define BUTTON_7   0xF807FF00
#define BUTTON_8   0xEA15FF00
#define BUTTON_9   0xF609FF00
#define BUTTON_HOLD 0xFFFFFFFF

const float PUMP_OP_VOLTAGE = 6.0;
const unsigned long LOOP_PERIOD = 50;

const byte pmp2Pn = 2;
const byte pmp1Pn = 3;
const byte rPn = 4;
const byte gPn = 5;
const byte bPn = 6;
const byte IR_RECEIVE_PIN = 9;
const byte led = 13;
const byte batPn = 19;
const byte flx2Pn = 20;
const byte flx1Pn = 21;
const byte tmpPn = 22;
const byte dpthPn = 23;

unsigned long pCase = PLAY_PAUSE;
unsigned long sTime, loopTime, actTime;
float batVolt, flex1, flex2, temp, depth;
float pGain = 0;
byte stage;
byte vRecoveryCount = 5;
int pump1, pump2;
int p1Write, p2Write;

// 🆕 Soft-start current values
int p1Current = 0;
int p2Current = 0;

void setup() {
  delay(2000);  // Prevent auto bootloader reentry
  Serial.begin(9600);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  pinMode(rPn, OUTPUT);
  pinMode(gPn, OUTPUT);
  pinMode(bPn, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(pmp1Pn, OUTPUT);
  pinMode(pmp2Pn, OUTPUT);

  pCase = PLAY_PAUSE;
}

void loop() {
  sTime = millis();

  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.decodedRawData != BUTTON_HOLD) {
      pCase = IrReceiver.decodedIRData.decodedRawData;
    }
    IrReceiver.resume();
  }

  switch (pCase) {
    case PLAY_PAUSE:
    case STOP_MODE:
      color(175, 255, 175);
      pump1 = 0;
      pump2 = 0;
      stage = 0;
      break;
    case BUTTON_1: // straight
      swim(80, 0, 80, 0, 1000, 2000);
      break;
    case BUTTON_2: // turn right
      swim(50, 0, 100, 0, 1000, 2000);
      break;
    case BUTTON_3: // turn left
      swim(100, 0, 50, 0, 1000, 2000);
      break;
    case BUTTON_4:
      color(0, 63, 52);
      break;
    case BUTTON_5:
      color(255, 0, 0);
      break;
    case BUTTON_6:
      color(0, 90, 255);
      break;
    case BUTTON_7:
      color(0, 0, 0);
      break;
    case BUTTON_8:
      color(255, 255, 255);
      break;
    case BUTTON_9:
      color(116, 186, 236);
      break;
  }

  batVolt = analogRead(batPn) * (9.24 / 1023.0);
  if (pump1 == 0 && pump2 == 0) {
    vRecoveryCount--;
    if (vRecoveryCount == 0) {
      if (batVolt > PUMP_OP_VOLTAGE) {
        pGain = (PUMP_OP_VOLTAGE / 100.0) * (255.0 / batVolt);
      } else {
        digitalWrite(led, 1);
        pGain = 180.0;
      }
    }
  } else {
    vRecoveryCount = 5;
  }

  if (pump1 <= 100 && pump2 <= 100) {
    // Calculate PWM output
    p1Write = constrain(round(pump1 * pGain), 0, 180);
    p2Write = constrain(round(pump2 * pGain), 0, 180);

    // 🆕 Soft-start logic (limits ramp rate)
    const int rampStep = 5;

    if (p1Current < p1Write)
      p1Current += min(rampStep, p1Write - p1Current);
    else if (p1Current > p1Write)
      p1Current -= min(rampStep, p1Current - p1Write);

    if (p2Current < p2Write)
      p2Current += min(rampStep, p2Write - p2Current);
    else if (p2Current > p2Write)
      p2Current -= min(rampStep, p2Current - p2Write);

    analogWrite(pmp1Pn, p1Current);
    analogWrite(pmp2Pn, p2Current);

    Serial.print("Pump1 PWM: "); Serial.print(p1Current);
    Serial.print(" | Pump2 PWM: "); Serial.println(p2Current);
  } else {
    analogWrite(pmp1Pn, 0);
    analogWrite(pmp2Pn, 0);
  }

  flex1 = analogRead(flx1Pn);
  flex2 = analogRead(flx2Pn);
  temp = analogRead(tmpPn);
  depth = analogRead(dpthPn);

  Serial.print(pCase, HEX); Serial.print(", ");
  Serial.print(pump1); Serial.print(", ");
  Serial.print(pump2); Serial.print(", ");
  Serial.print(flex1); Serial.print(", ");
  Serial.print(flex2); Serial.print(", ");
  Serial.print(temp); Serial.print(", ");
  Serial.print(depth); Serial.print(", ");
  Serial.println(batVolt);

  loopTime = millis() - sTime;
  if (loopTime < LOOP_PERIOD) {
    delay(LOOP_PERIOD - loopTime);
  } else {
    Serial.print("Over loop error ");
    Serial.println(loopTime);
  }
}

void swim(int c1, int r1, int c2, int r2, unsigned int cT, unsigned int rT) {
  switch (stage) {
    case 0:
      actTime = millis();
      stage++;
      break;
    case 1:
      color(255, 0, 255);
      pump1 = c1;
      pump2 = c2;
      if ((millis() - actTime) > cT) stage++;
      break;
    case 2:
      color(255, 255, 0);
      pump1 = r1;
      pump2 = r2;
      if ((millis() - actTime) > (cT + rT)) stage = 0;
      break;
  }
}

void color(int r, int g, int b) {
  analogWrite(rPn, r);
  analogWrite(gPn, g);
  analogWrite(bPn, b);
}

