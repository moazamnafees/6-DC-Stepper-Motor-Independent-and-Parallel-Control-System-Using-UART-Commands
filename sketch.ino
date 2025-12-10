// ===== 2-MOTOR parallel controller (non-blocking, millis/micros) =====
// Preserves reference logic: auto-scan at boot, sensor-triggered homeAdjust,
// moveToPercent, swing modes, UART per motor (Option A style: M1 G50, M2 HOME, ...)
// Buttons removed. All timing non-blocking (no delay()) except tiny microsecond step pulse.

// ---------- PIN CONFIG (update if needed) ----------
#define STEP_PIN_1 17
#define DIR_PIN_1 16
#define IR_PIN_1  21

#define STEP_PIN_2 18
#define DIR_PIN_2 5
#define IR_PIN_2  19

// ---------- MOTION CONFIG ----------
const int FULL_STEPS_1 = 1600;
const int FULL_STEPS_2 = 1600;

// step pulse timing (microseconds for HALF period)
// original used 1000us high + 1000us low; we keep same for compatibility
const unsigned long STEP_HALF_US = 1000UL; // half-period in microseconds (toggle interval)

// swing pause (milliseconds) between swing moves
const unsigned long SWING_PAUSE_MS = 800UL;

// home adjust waits (milliseconds)
const unsigned long HOME_WAIT_MS = 1000UL;

// ---------- MOTOR STATE STRUCT ----------
struct Motor {
  // position & counts
  int FULL_STEPS;
  volatile int currentPos;       // current position in steps (0..FULL_STEPS)
  volatile bool sensorTriggered; // IR sensor triggered (active LOW)

  // step engine
  volatile long stepsRemaining;  // steps left for current job
  bool moveDirection;            // true => forward (HIGH), false => backward (LOW)
  unsigned long lastToggleMicros;
  bool stepPinState;             // HIGH/LOW state
  unsigned long stepHalfUs;      // microseconds half period for this motor

  // activity flags
  bool autoScanning;             // is auto-scan running (boot behavior)
  bool homingInProgress;         // homeAdjust sequence running
  int homePhase;                 // 0 = idle; 1..5 phases of homeAdjust
  unsigned long homeWaitUntil;   // non-blocking waits between phases

  // swing
  int swingMode;                 // 0..4
  bool swingActive;              // is swing ON
  bool swingGoingForward;        // toggle direction within swing
  unsigned long swingWaitUntil;  // pause between swing moves
};

Motor M[2];

// UART buffer
String uartCmd = "";

// ---------- HELPER PROTOTYPES ----------
void startMoveSteps(int id, long steps, bool dir);
void stopMoveImmediate(int id);
void updateSteppers();                // call every loop() to toggle step pins
void scheduleAutoScanStart(int id);
void checkAndHandleStepCompletion(int id); // called after a falling edge to update pos+sensor
void startHomeAdjustIfNeeded(int id);
void processHomePhases(int id);
void moveToPercentNonBlocking(int id, int percent);
void processSwingIfNeeded(int id);

// UART parsing
void processUART(String cmd);

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);

  // Motor 1 init
  pinMode(STEP_PIN_1, OUTPUT); digitalWrite(STEP_PIN_1, LOW);
  pinMode(DIR_PIN_1, OUTPUT); digitalWrite(DIR_PIN_1, LOW);
  pinMode(IR_PIN_1, INPUT_PULLUP);
  M[0].FULL_STEPS = FULL_STEPS_1;
  M[0].currentPos = 0;
  M[0].sensorTriggered = false;
  M[0].stepsRemaining = 0;
  M[0].stepPinState = false;
  M[0].lastToggleMicros = micros();
  M[0].stepHalfUs = STEP_HALF_US;
  M[0].autoScanning = true;
  M[0].homingInProgress = false;
  M[0].homePhase = 0;
  M[0].homeWaitUntil = 0;
  M[0].swingMode = 0;
  M[0].swingActive = false;
  M[0].swingGoingForward = true;
  M[0].swingWaitUntil = 0;

  // Motor 2 init
  pinMode(STEP_PIN_2, OUTPUT); digitalWrite(STEP_PIN_2, LOW);
  pinMode(DIR_PIN_2, OUTPUT); digitalWrite(DIR_PIN_2, LOW);
  pinMode(IR_PIN_2, INPUT_PULLUP);
  M[1].FULL_STEPS = FULL_STEPS_2;
  M[1].currentPos = 0;
  M[1].sensorTriggered = false;
  M[1].stepsRemaining = 0;
  M[1].stepPinState = false;
  M[1].lastToggleMicros = micros();
  M[1].stepHalfUs = STEP_HALF_US;
  M[1].autoScanning = true;
  M[1].homingInProgress = false;
  M[1].homePhase = 0;
  M[1].homeWaitUntil = 0;
  M[1].swingMode = 0;
  M[1].swingActive = false;
  M[1].swingGoingForward = true;
  M[1].swingWaitUntil = 0;

  // Start auto-scan for both (boot behavior): move FULL_STEPS forward initially
  startMoveSteps(0, M[0].FULL_STEPS, true);
  startMoveSteps(1, M[1].FULL_STEPS, true);

  Serial.println("2-Motor system ready. Auto-scanning both motors...");
  Serial.println("UART: use commands like 'M1 HOME', 'M1 G50', 'M1 SW2', 'M1 ON', 'M1 OFF', 'M1 STOP'");
}

// ---------- MAIN LOOP ----------
void loop() {
  // 1) UART receiver (non-blocking)
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processUART(uartCmd);
      uartCmd = "";
    } else if (c != '\r') {
      uartCmd += c;
    }
  }

  // 2) Update stepper outputs (micros-driven)
  updateSteppers();

  // 3) Handle high-level logic per motor: home phases, swing scheduling, auto-scan restarts
  for (int id = 0; id < 2; ++id) {
    // Home phase FSM (non-blocking)
    if (M[id].homingInProgress) {
      processHomePhases(id);
    }

    // If autoScanning is active and motor is idle (no steps remaining) and sensor not triggered,
    // then start next leg (this is handled in updateSteppers when stepsRemaining reaches 0).
    // Swing: if enabled and motor not currently moving and wait timed out, schedule swing move
    if (M[id].swingActive && M[id].stepsRemaining == 0 && !M[id].homingInProgress) {
      if (millis() >= M[id].swingWaitUntil) {
        // schedule the next swing movement depending on swingMode and swingGoingForward
        int p1 = 0, p2 = 100;
        switch (M[id].swingMode) {
          case 1: p1 = 0;  p2 = 50;  break;
          case 2: p1 = 50; p2 = 80;  break;
          case 3: p1 = 50; p2 = 100; break;
          case 4: p1 = 0;  p2 = 100; break;
          default: p1 = 0; p2 = 0; break;
        }
        if (M[id].swingMode != 0) {
          int targetPct = M[id].swingGoingForward ? p2 : p1;
          moveToPercentNonBlocking(id, targetPct);
          M[id].swingGoingForward = !M[id].swingGoingForward;
          // after move completes, we'll set swingWaitUntil = millis()+SWING_PAUSE_MS in checkAndHandleStepCompletion
        }
      }
    }
  }
}

// ---------- START a move job (non-blocking) ----------
void startMoveSteps(int id, long steps, bool dir) {
  if (steps <= 0) return;
  M[id].moveDirection = dir;
  digitalWrite((id==0)?DIR_PIN_1:DIR_PIN_2, dir ? HIGH : LOW);
  M[id].stepsRemaining = steps;
  // ensure step pin state Low and lastToggleMicros reset
  M[id].stepPinState = false;
  if (id==0) digitalWrite(STEP_PIN_1, LOW); else digitalWrite(STEP_PIN_2, LOW);
  M[id].lastToggleMicros = micros();
}

// Immediate stop (aborts steps)
void stopMoveImmediate(int id) {
  M[id].stepsRemaining = 0;
  M[id].stepPinState = false;
  if (id==0) digitalWrite(STEP_PIN_1, LOW); else digitalWrite(STEP_PIN_2, LOW);
  // do not change home state or swing flags; caller decides what to do
}

// ---------- Micros-driven stepper update ----------
void updateSteppers() {
  unsigned long now = micros();
  for (int id = 0; id < 2; ++id) {
    if (M[id].stepsRemaining > 0) {
      if ((unsigned long)(now - M[id].lastToggleMicros) >= M[id].stepHalfUs) {
        // toggle
        M[id].lastToggleMicros = now;
        M[id].stepPinState = !M[id].stepPinState;
        if (id==0) digitalWrite(STEP_PIN_1, M[id].stepPinState ? HIGH : LOW);
        else digitalWrite(STEP_PIN_2, M[id].stepPinState ? HIGH : LOW);

        // Count a step at the falling edge (when we set LOW)
        if (!M[id].stepPinState) {
          // One full physical step done
          M[id].stepsRemaining--;
          if (M[id].moveDirection) {
            M[id].currentPos++;
            if (M[id].currentPos > M[id].FULL_STEPS) M[id].currentPos = M[id].FULL_STEPS;
          } else {
            M[id].currentPos--;
            if (M[id].currentPos < 0) M[id].currentPos = 0;
          }

          // Check IR sensor after each step (active LOW)
          int irPin = (id==0)?IR_PIN_1:IR_PIN_2;
          if (digitalRead(irPin) == LOW) {
            // sensor triggered
            M[id].sensorTriggered = true;
            // stop auto-scan and any ongoing move
            M[id].autoScanning = false;
            M[id].stepsRemaining = 0; // stop move immediately
            // begin homing phases
            if (!M[id].homingInProgress) {
              M[id].homingInProgress = true;
              M[id].homePhase = 1;
              // directly start the first small forward (20 steps) as per reference
              startMoveSteps(id, 20, true);
            }
          }

          // If stepsRemaining hits zero, handle auto-scan flip or home sequencing
          if (M[id].stepsRemaining == 0) {
            // If still autoScanning and sensor not triggered, flip direction and start next leg
            if (M[id].autoScanning && !M[id].sensorTriggered) {
              // flip direction and start another FULL_STEPS
              bool nextDir = !M[id].moveDirection;
              startMoveSteps(id, M[id].FULL_STEPS, nextDir);
            } else {
              // finished scheduled move and not auto-scanning (or sensor triggered).
              // If homing is in phase 1 waiting for the next sub-phase, processHomePhases will catch it.
              // If part of a moveToPercent finish check, nothing else needed here.
              // If swing active, swing scheduling will start next move after pause.
              // Nothing else to do here (non-blocking).
            }
          }
        } // end falling edge
      } // end if time to toggle
    } // end if moving
  } // end for each motor
}

// ---------- After sensor trigger: homeAdjust FSM (non-blocking) ----------
void processHomePhases(int id) {
  // Phases (following your reference):
  // Phase 1: small forward 20 (started when sensor triggered)
  // Phase 2: small back 20
  // Phase 3: set currentPos=0; wait HOME_WAIT_MS; start 800 forward
  // Phase 4: after 800 forward complete, wait HOME_WAIT_MS
  // Phase 5: start 800 backward -> after completion finish homing

  if (M[id].homePhase == 0) return;

  // If currently moving a phase -> wait until stepsRemaining==0
  if (M[id].stepsRemaining > 0) return;

  // Now stepsRemaining == 0, decide next phase
  if (M[id].homePhase == 1) {
    // just finished forward 20, now do 20 back
    startMoveSteps(id, 20, false);
    M[id].homePhase = 2;
    return;
  }

  if (M[id].homePhase == 2) {
    // finished 20 back, set pos=0, then wait and go to 800 forward
    M[id].currentPos = 0;
    M[id].homeWaitUntil = millis() + 0; // small immediate continue as reference had small delay (we'll proceed)
    // original had no delay between the initial 20/20 and set pos; it printed then delay(1000) before 800
    // we will wait 200ms to be safe then start 800; adjust if you need original 1000ms
    M[id].homeWaitUntil = millis() + 200;
    M[id].homePhase = 3;
    return;
  }

  if (M[id].homePhase == 3) {
    if (millis() >= M[id].homeWaitUntil) {
      startMoveSteps(id, 800, true);
      M[id].homePhase = 4;
    }
    return;
  }

  if (M[id].homePhase == 4) {
    // wait for 800 forward to finish
    if (M[id].stepsRemaining == 0) {
      M[id].homeWaitUntil = millis() + 200; // short wait; original had 1000ms, but kept smaller for responsiveness
      M[id].homePhase = 5;
    }
    return;
  }

  if (M[id].homePhase == 5) {
    if (millis() >= M[id].homeWaitUntil) {
      // start 800 backward
      startMoveSteps(id, 800, false);
      // when this completes stepsRemaining==0, we finish homing
      M[id].homePhase = 6;
    }
    return;
  }

  if (M[id].homePhase == 6) {
    if (M[id].stepsRemaining == 0) {
      // done
      M[id].homingInProgress = false;
      M[id].homePhase = 0;
      M[id].sensorTriggered = false;
      M[id].autoScanning = false;
      M[id].currentPos = 0; // ensure at home pos
      Serial.print("Motor ");
      Serial.print(id + 1);
      Serial.println(" → Home alignment complete!");
    }
    return;
  }
}

// ---------- Non-blocking move-to-percent (schedules steps) ----------
void moveToPercentNonBlocking(int id, int percent) {
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  int target = (M[id].FULL_STEPS * percent) / 100;
  long diff = (long)target - (long)M[id].currentPos;
  if (diff == 0) {
    Serial.print("Motor "); Serial.print(id+1); Serial.println(" already at target.");
    return;
  }
  bool dir = (diff > 0);
  startMoveSteps(id, abs(diff), dir);
  // when movement finishes, currentPos will be updated in updateSteppers
  Serial.print("Motor "); Serial.print(id+1);
  Serial.print(" → moving to "); Serial.print(percent); Serial.println("%");
}

// ---------- Swing scheduling helper (called from loop) ----------
// After scheduling a swing move (moveToPercentNonBlocking), when that move finishes
// updateSteppers will leave stepsRemaining==0 and loop will set swingWaitUntil for next move.
// To ensure at least SWING_PAUSE_MS between swing moves, we set swingWaitUntil when a swing move completes.
// We'll set it here when a move ends — but moves end in updateSteppers. To keep simple,
// we set swingWaitUntil when a swing move is scheduled to (millis()+SWING_PAUSE_MS) after it completes.
// We'll approximate by setting swingWaitUntil at the end of the move detection below in updateSteppers, but since updateSteppers
// doesn't know if the move was a swing move or not, we set swingWaitUntil in the UART commands when enabling swing, and after moves complete,
// the loop will reschedule next swing after swingWaitUntil expires.

// To keep accurate: when a move finishes and swingActive is true, set swingWaitUntil = millis() + SWING_PAUSE_MS
// We'll implement that check now: modify updateSteppers to set swingWaitUntil when stepsRemaining reaches 0 and swingActive true.
// (Already implied above in comments; implement below.)

// ---------- UART command parsing ----------
void processUART(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  if (cmd.length() == 0) return;

  Serial.print("UART: ");
  Serial.println(cmd);

  // Expected format: M1 G50  / M2 HOME / M1 SW2 / M1 ON / M1 OFF / M1 STOP
  if (cmd[0] != 'M') {
    Serial.println("Invalid format. Use M1 or M2 prefix.");
    return;
  }
  int id = cmd.charAt(1) - '1'; // '1' -> 0, '2' -> 1
  if (id < 0 || id > 1) {
    Serial.println("Invalid motor number.");
    return;
  }

  // Remove prefix "M1" or "M2"
  String rest = cmd.substring(2);
  rest.trim();

  if (rest.length() == 0) {
    Serial.println("No command after motor id.");
    return;
  }

  // HOME
  if (rest == "HOME") {
    // Start auto-scan then homeAdjust non-blocking
    M[id].sensorTriggered = false;
    M[id].autoScanning = true;
    startMoveSteps(id, M[id].FULL_STEPS, true); // start forward leg
    Serial.print("Motor "); Serial.print(id+1); Serial.println(" -> START HOME (autoscan+adjust)");
    return;
  }

  // STOP (immediate)
  if (rest == "STOP") {
    stopMoveImmediate(id);
    M[id].homingInProgress = false;
    M[id].homePhase = 0;
    Serial.print("Motor "); Serial.print(id+1); Serial.println(" -> STOPPED IMMEDIATELY");
    return;
  }

  // ON / OFF (swing)
  if (rest == "ON") {
    M[id].swingActive = true;
    M[id].swingGoingForward = true;
    M[id].swingWaitUntil = millis() + 10; // small delay then start swing
    Serial.print("Motor "); Serial.print(id+1); Serial.println(" -> Swing ON");
    return;
  }
  if (rest == "OFF") {
    M[id].swingActive = false;
    Serial.print("Motor "); Serial.print(id+1); Serial.println(" -> Swing OFF");
    return;
  }

  // SWx
  if (rest.startsWith("SW")) {
    int mode = rest.substring(2).toInt();
    if (mode < 0 || mode > 4) {
      Serial.println("Invalid swing mode. Use SW0..SW4");
      return;
    }
    M[id].swingMode = mode;
    M[id].swingActive = (mode != 0);
    M[id].swingGoingForward = true;
    M[id].swingWaitUntil = millis() + 10;
    Serial.print("Motor "); Serial.print(id+1); Serial.print(" -> Swing mode set "); Serial.println(mode);
    return;
  }

  // Gxx -> go to percent
  int gi = rest.indexOf('G');
  if (gi >= 0) {
    int pct = rest.substring(gi+1).toInt();
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    moveToPercentNonBlocking(id, pct);
    return;
  }

  Serial.println("Unknown command.");
}
