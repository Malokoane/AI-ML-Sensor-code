const int IR_PIN     = 27;
const int LED_PIN    = 2;              // onboard LED
const int BUZZER_PIN = 13;             // buzzer + goes to D13

const bool CLOSED_IS_LOW   = true;     // LOW = eyelid detected
const bool PASSIVE_BUZZER  = false;    // set true if your buzzer only clicks

const unsigned long DROWSY_MS   = 800; // closed longer than this = drowsy
const unsigned long DEBOUNCE_MS = 30;

bool eyeClosed = false;
bool alerted = false;
unsigned long closedStart = 0;

bool lastRaw = false;
bool stableState = false;
unsigned long lastChange = 0;

bool readClosed() {
  bool pinState = digitalRead(IR_PIN);
  return CLOSED_IS_LOW ? (pinState == LOW) : (pinState == HIGH);
}

// Drives the buzzer. Call every loop with true (sound) or false (silent).
void buzzerUpdate(bool on) {
  static bool pinLevel = false;
  static unsigned long lastToggle = 0;

  if (!on) {
    digitalWrite(BUZZER_PIN, LOW);
    pinLevel = false;
    return;
  }

  if (PASSIVE_BUZZER) {
    // ~2 kHz square wave
    if (micros() - lastToggle >= 250) {
      lastToggle = micros();
      pinLevel = !pinLevel;
      digitalWrite(BUZZER_PIN, pinLevel);
    }
  } else {
    digitalWrite(BUZZER_PIN, HIGH);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(IR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  delay(500);
  Serial.println("Drowsiness detector started.");
  stableState = readClosed();
  lastRaw = stableState;
}

void loop() {
  // Debounce the sensor
  bool raw = readClosed();
  if (raw != lastRaw) {
    lastRaw = raw;
    lastChange = millis();
  }
  bool closedNow = (millis() - lastChange > DEBOUNCE_MS) ? raw : stableState;

  if (closedNow && !eyeClosed) {
    eyeClosed = true;
    closedStart = millis();
    Serial.println("-> CLOSED");
  } else if (!closedNow && eyeClosed) {
    eyeClosed = false;
    alerted = false;
    digitalWrite(LED_PIN, LOW);
    Serial.print("-> OPEN (was closed for ");
    Serial.print(millis() - closedStart);
    Serial.println(" ms)");
  }
  stableState = closedNow;

  // Alert: beep in 200 ms on/off pulses while drowsy
  bool drowsy = eyeClosed && (millis() - closedStart > DROWSY_MS);

  if (drowsy) {
    if (!alerted) {
      Serial.println("DROWSY!");
      alerted = true;
    }
    bool beepPhase = (millis() / 200) % 2 == 0;
    digitalWrite(LED_PIN, beepPhase);
    buzzerUpdate(beepPhase);
  } else {
    buzzerUpdate(false);
  }
}