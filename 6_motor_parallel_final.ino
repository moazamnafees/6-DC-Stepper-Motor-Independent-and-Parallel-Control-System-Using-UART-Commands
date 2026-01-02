// ===== 6-MOTOR PARALLEL CONTROLLER (non-blocking) with EEPROM State Persistence =====
//  Added EEPROM functionality to save and restore motor states across power cycles
//  On startup: Motors auto-scan until sensor triggers, then restore previous state
//  Each motor saves: currentPos, swingMode, swingActive state

#include <EEPROM.h>

// ---------- PIN CONFIG ----------

#define STEP_PIN_1 13
#define DIR_PIN_1 12
#define IR_PIN_1  14

#define STEP_PIN_2 27
#define DIR_PIN_2 26
#define IR_PIN_2  25

#define STEP_PIN_3 21
#define DIR_PIN_3 22
#define IR_PIN_3  32

#define STEP_PIN_4 15
#define DIR_PIN_4 2
#define IR_PIN_4  0

#define STEP_PIN_5 4
#define DIR_PIN_5 16
#define IR_PIN_5  17

#define STEP_PIN_6 5
#define DIR_PIN_6 18
#define IR_PIN_6  19


// ---------- MOTION CONFIG ----------
const int FULL_STEPS_1 = 1600;
const int FULL_STEPS_2 = 1600;
const int FULL_STEPS_3 = 1600;
const int FULL_STEPS_4 = 1600;
const int FULL_STEPS_5 = 1600;
const int FULL_STEPS_6 = 1600;

const unsigned long STEP_HALF_US = 1000UL;


// ---------- EEPROM CONFIG ----------
#define EEPROM_SIZE 512
#define EEPROM_MAGIC 0xA5B6  // Magic number to verify valid EEPROM data

// EEPROM memory layout per motor (12 bytes each):
// Offset 0-1:   Magic number (shared, only at start)
// Motor 1: 2-13   (currentPos: 4 bytes, swingMode: 4 bytes, swingActive: 4 bytes)
// Motor 2: 14-25
// Motor 3: 26-37
// Motor 4: 38-49
// Motor 5: 50-61
// Motor 6: 62-73

#define EEPROM_MAGIC_ADDR 0
#define MOTOR_EEPROM_SIZE 12
#define MOTOR_1_ADDR 2
#define MOTOR_2_ADDR (MOTOR_1_ADDR + MOTOR_EEPROM_SIZE)
#define MOTOR_3_ADDR (MOTOR_2_ADDR + MOTOR_EEPROM_SIZE)
#define MOTOR_4_ADDR (MOTOR_3_ADDR + MOTOR_EEPROM_SIZE)
#define MOTOR_5_ADDR (MOTOR_4_ADDR + MOTOR_EEPROM_SIZE)
#define MOTOR_6_ADDR (MOTOR_5_ADDR + MOTOR_EEPROM_SIZE)

const int MOTOR_EEPROM_ADDRS[6] = {
  MOTOR_1_ADDR, MOTOR_2_ADDR, MOTOR_3_ADDR,
  MOTOR_4_ADDR, MOTOR_5_ADDR, MOTOR_6_ADDR
};


// ---------- MOTOR STATE ----------
struct Motor {

  int FULL_STEPS;
  volatile int currentPos;
  volatile bool sensorTriggered;

  volatile long stepsRemaining;
  bool moveDirection;
  unsigned long lastToggleMicros;
  bool stepPinState;
  unsigned long stepHalfUs;

  bool autoScanning;
  bool homingInProgress;
  int homePhase;
  unsigned long homeWaitUntil;

  int swingMode;
  bool swingActive;
  bool swingGoingForward;
  unsigned long swingWaitUntil;
  
  // EEPROM restoration flags
  bool hasSavedState;           // Does this motor have saved state in EEPROM?
  int savedPos;                 // Saved position from EEPROM
  int savedSwingMode;           // Saved swing mode from EEPROM
  bool savedSwingActive;        // Saved swing active state from EEPROM
  bool waitingForSensorToRestore;  // Flag: waiting for sensor to restore state
};

Motor M[6];
String uartCmd;


// ---------- FUNCTION PROTOTYPES ----------
void startMoveSteps(int id,long steps,bool dir);
void stopMoveImmediate(int id);
void updateSteppers();
void processHomePhases(int id);
void moveToPercentNonBlocking(int id,int percent);
void processUART(String cmd);
void saveMotorState(int id);
bool loadMotorStateFromEEPROM(int id);
void saveAllMotorStates();
void initEEPROM();
void writeIntToEEPROM(int address, int value);
int readIntFromEEPROM(int address);
void restoreSavedStateAfterHoming(int id);


// ---------- EEPROM FUNCTIONS ----------

void initEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  
  // Check if EEPROM has valid data
  int magic = (EEPROM.read(EEPROM_MAGIC_ADDR) << 8) | EEPROM.read(EEPROM_MAGIC_ADDR + 1);
  
  if (magic != EEPROM_MAGIC) {
    Serial.println("EEPROM: First time initialization");
    // Write magic number
    EEPROM.write(EEPROM_MAGIC_ADDR, (EEPROM_MAGIC >> 8) & 0xFF);
    EEPROM.write(EEPROM_MAGIC_ADDR + 1, EEPROM_MAGIC & 0xFF);
    
    // Initialize all motor states to default (position 0, no swing)
    for (int i = 0; i < 6; i++) {
      writeIntToEEPROM(MOTOR_EEPROM_ADDRS[i], 0);      // currentPos = 0
      writeIntToEEPROM(MOTOR_EEPROM_ADDRS[i] + 4, 0);  // swingMode = 0
      writeIntToEEPROM(MOTOR_EEPROM_ADDRS[i] + 8, 0);  // swingActive = false
    }
    EEPROM.commit();
    Serial.println("EEPROM: Initialized with default values");
  } else {
    Serial.println("EEPROM: Valid data found");
  }
}

void writeIntToEEPROM(int address, int value) {
  EEPROM.write(address, (value >> 24) & 0xFF);
  EEPROM.write(address + 1, (value >> 16) & 0xFF);
  EEPROM.write(address + 2, (value >> 8) & 0xFF);
  EEPROM.write(address + 3, value & 0xFF);
}

int readIntFromEEPROM(int address) {
  return ((int)EEPROM.read(address) << 24) |
         ((int)EEPROM.read(address + 1) << 16) |
         ((int)EEPROM.read(address + 2) << 8) |
         ((int)EEPROM.read(address + 3));
}

void saveMotorState(int id) {
  if (id < 0 || id >= 6) return;
  
  int addr = MOTOR_EEPROM_ADDRS[id];
  
  writeIntToEEPROM(addr, M[id].currentPos);
  writeIntToEEPROM(addr + 4, M[id].swingMode);
  writeIntToEEPROM(addr + 8, M[id].swingActive ? 1 : 0);
  
  EEPROM.commit();
  
  Serial.print("EEPROM: Motor ");
  Serial.print(id + 1);
  Serial.print(" saved (Pos=");
  Serial.print(M[id].currentPos);
  Serial.print(", Swing=");
  Serial.print(M[id].swingMode);
  Serial.println(")");
}

bool loadMotorStateFromEEPROM(int id) {
  if (id < 0 || id >= 6) return false;
  
  int addr = MOTOR_EEPROM_ADDRS[id];
  
  int savedPos = readIntFromEEPROM(addr);
  int savedSwingMode = readIntFromEEPROM(addr + 4);
  int savedSwingActive = readIntFromEEPROM(addr + 8);
  
  // Validate restored values
  if (savedPos < 0 || savedPos > M[id].FULL_STEPS) {
    Serial.print("EEPROM: Motor ");
    Serial.print(id + 1);
    Serial.println(" invalid position in EEPROM");
    return false;
  }
  
  if (savedSwingMode < 0 || savedSwingMode > 4) {
    savedSwingMode = 0;
    savedSwingActive = 0;
  }
  
  // Store the saved state but don't apply it yet
  M[id].savedPos = savedPos;
  M[id].savedSwingMode = savedSwingMode;
  M[id].savedSwingActive = (savedSwingActive != 0);
  M[id].hasSavedState = true;
  M[id].waitingForSensorToRestore = true;
  
  Serial.print("EEPROM: Motor ");
  Serial.print(id + 1);
  Serial.print(" loaded state (Pos=");
  Serial.print(M[id].savedPos);
  Serial.print(", Swing=");
  Serial.print(M[id].savedSwingMode);
  Serial.println(") - will restore after sensor trigger");
  
  return true;
}

void restoreSavedStateAfterHoming(int id) {
  if (!M[id].hasSavedState) return;
  
  Serial.print("Motor ");
  Serial.print(id + 1);
  Serial.print(" restoring saved state → moving to position ");
  Serial.println(M[id].savedPos);
  
  // Restore swing settings
  M[id].swingMode = M[id].savedSwingMode;
  M[id].swingActive = M[id].savedSwingActive;
  M[id].swingGoingForward = true;
  M[id].swingWaitUntil = millis() + 1000;
  
  // Move to saved position
  if (M[id].savedPos > 0) {
    startMoveSteps(id, M[id].savedPos, true);
  }
  
  M[id].waitingForSensorToRestore = false;
  
  Serial.print("Motor ");
  Serial.print(id + 1);
  Serial.println(" state restoration complete");
}

void saveAllMotorStates() {
  for (int i = 0; i < 6; i++) {
    saveMotorState(i);
  }
}


// ---------- SETUP ----------
void setup() {

  Serial.begin(115200);
  delay(500);  // Allow serial to stabilize
  
  Serial.println("=== 6-MOTOR CONTROLLER WITH EEPROM ===");

  // Initialize EEPROM
  initEEPROM();

  pinMode(STEP_PIN_1,OUTPUT); digitalWrite(STEP_PIN_1,LOW);
  pinMode(DIR_PIN_1,OUTPUT);  digitalWrite(DIR_PIN_1,LOW);
  pinMode(IR_PIN_1,INPUT_PULLUP);

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
  M[0].hasSavedState = false;
  M[0].savedPos = 0;
  M[0].savedSwingMode = 0;
  M[0].savedSwingActive = false;
  M[0].waitingForSensorToRestore = false;

  // clone properties to others
  pinMode(STEP_PIN_2,OUTPUT); digitalWrite(STEP_PIN_2,LOW);
  pinMode(DIR_PIN_2,OUTPUT);  digitalWrite(DIR_PIN_2,LOW);
  pinMode(IR_PIN_2,INPUT_PULLUP);
  M[1] = M[0]; M[1].FULL_STEPS = FULL_STEPS_2;

  pinMode(STEP_PIN_3,OUTPUT); digitalWrite(STEP_PIN_3,LOW);
  pinMode(DIR_PIN_3,OUTPUT);  digitalWrite(DIR_PIN_3,LOW);
  pinMode(IR_PIN_3,INPUT_PULLUP);
  M[2] = M[0]; M[2].FULL_STEPS = FULL_STEPS_3;

  pinMode(STEP_PIN_4,OUTPUT); digitalWrite(STEP_PIN_4,LOW);
  pinMode(DIR_PIN_4,OUTPUT);  digitalWrite(DIR_PIN_4,LOW);
  pinMode(IR_PIN_4,INPUT_PULLUP);
  M[3] = M[0]; M[3].FULL_STEPS = FULL_STEPS_4;

  pinMode(STEP_PIN_5,OUTPUT); digitalWrite(STEP_PIN_5,LOW);
  pinMode(DIR_PIN_5,OUTPUT);  digitalWrite(DIR_PIN_5,LOW);
  pinMode(IR_PIN_5,INPUT_PULLUP);
  M[4] = M[0]; M[4].FULL_STEPS = FULL_STEPS_5;

  pinMode(STEP_PIN_6,OUTPUT); digitalWrite(STEP_PIN_6,LOW);
  pinMode(DIR_PIN_6,OUTPUT);  digitalWrite(DIR_PIN_6,LOW);
  pinMode(IR_PIN_6,INPUT_PULLUP);
  M[5] = M[0]; M[5].FULL_STEPS = FULL_STEPS_6;

  // Load motor states from EEPROM (but don't apply yet)
  Serial.println("Loading motor states from EEPROM...");
  
  for (int i = 0; i < 6; i++) {
    loadMotorStateFromEEPROM(i);
  }
  
  // Start auto-scan for ALL motors (even if they have saved state)
  // They will restore state after sensor triggers during auto-scan
  Serial.println("Starting auto-scan for all motors...");
  for(int i=0;i<6;i++) {
    startMoveSteps(i, M[i].FULL_STEPS, true);
  }

  Serial.println("System ready — motors scanning until sensor trigger, then restoring state");
}


// ---------- LOOP ----------
void loop() {

  while(Serial.available()){
    char c = Serial.read();
    if(c=='\n'){ processUART(uartCmd); uartCmd=""; }
    else if(c!='\r') uartCmd+=c;
  }

  updateSteppers();

  for(int id=0; id<6; id++){

    if(M[id].homingInProgress)
      processHomePhases(id);

    if(M[id].swingActive &&
       M[id].stepsRemaining==0 &&
       !M[id].homingInProgress &&
       millis()>=M[id].swingWaitUntil)
    {
      int p1=0,p2=100;
      switch(M[id].swingMode){
        case 1:p1=0;p2=50;break;
        case 2:p1=50;p2=80;break;
        case 3:p1=50;p2=100;break;
        case 4:p1=0;p2=100;break;
      }

      if(M[id].swingMode!=0){
        int tgt = M[id].swingGoingForward ? p2 : p1;

        Serial.print("Motor ");
        Serial.print(id+1);
        Serial.print(" swing → target ");
        Serial.print(tgt);
        Serial.println("%");

        moveToPercentNonBlocking(id,tgt);
        M[id].swingGoingForward = !M[id].swingGoingForward;
      }
    }
  }
}


// ---------- START MOVE ----------
void startMoveSteps(int id,long steps,bool dir){

  if(steps<=0) return;

  M[id].moveDirection = dir;

  switch(id){
    case 0:digitalWrite(DIR_PIN_1,dir);break;
    case 1:digitalWrite(DIR_PIN_2,dir);break;
    case 2:digitalWrite(DIR_PIN_3,dir);break;
    case 3:digitalWrite(DIR_PIN_4,dir);break;
    case 4:digitalWrite(DIR_PIN_5,dir);break;
    case 5:digitalWrite(DIR_PIN_6,dir);break;
  }

  M[id].stepsRemaining = steps;
  M[id].stepPinState = false;

  switch(id){
    case 0:digitalWrite(STEP_PIN_1,LOW);break;
    case 1:digitalWrite(STEP_PIN_2,LOW);break;
    case 2:digitalWrite(STEP_PIN_3,LOW);break;
    case 3:digitalWrite(STEP_PIN_4,LOW);break;
    case 4:digitalWrite(STEP_PIN_5,LOW);break;
    case 5:digitalWrite(STEP_PIN_6,LOW);break;
  }

  M[id].lastToggleMicros = micros();
}


// ---------- STOP MOVE ----------
void stopMoveImmediate(int id){

  M[id].stepsRemaining = 0;
  M[id].stepPinState = false;

  Serial.print("Motor ");
  Serial.print(id+1);
  Serial.println(" STOP");

  switch(id){
    case 0:digitalWrite(STEP_PIN_1,LOW);break;
    case 1:digitalWrite(STEP_PIN_2,LOW);break;
    case 2:digitalWrite(STEP_PIN_3,LOW);break;
    case 3:digitalWrite(STEP_PIN_4,LOW);break;
    case 4:digitalWrite(STEP_PIN_5,LOW);break;
    case 5:digitalWrite(STEP_PIN_6,LOW);break;
  }
  
  // Save state when motor stops
  saveMotorState(id);
}


// ---------- UPDATE STEPPERS ----------
void updateSteppers(){

  unsigned long now = micros();

  for(int id=0; id<6; id++){

    if(M[id].stepsRemaining>0 &&
       (now - M[id].lastToggleMicros) >= M[id].stepHalfUs)
    {
      M[id].lastToggleMicros = now;
      M[id].stepPinState = !M[id].stepPinState;

      int pin=0;
      switch(id){
        case 0:pin=STEP_PIN_1;break;
        case 1:pin=STEP_PIN_2;break;
        case 2:pin=STEP_PIN_3;break;
        case 3:pin=STEP_PIN_4;break;
        case 4:pin=STEP_PIN_5;break;
        case 5:pin=STEP_PIN_6;break;
      }

      digitalWrite(pin,M[id].stepPinState);

      if(!M[id].stepPinState){

        M[id].stepsRemaining--;

        if(M[id].moveDirection){
          if(++M[id].currentPos > M[id].FULL_STEPS)
             M[id].currentPos = M[id].FULL_STEPS;
        } else {
          if(--M[id].currentPos < 0)
             M[id].currentPos = 0;
        }

        int ir=0;
        switch(id){
          case 0:ir=IR_PIN_1;break;
          case 1:ir=IR_PIN_2;break;
          case 2:ir=IR_PIN_3;break;
          case 3:ir=IR_PIN_4;break;
          case 4:ir=IR_PIN_5;break;
          case 5:ir=IR_PIN_6;break;
        }

        if(digitalRead(ir)==LOW){

          Serial.print("Motor ");
          Serial.print(id+1);
          Serial.println(" SENSOR TRIGGERED");

          M[id].sensorTriggered = true;
          M[id].autoScanning = false;
          M[id].stepsRemaining = 0;

          if(!M[id].homingInProgress){
            M[id].homingInProgress = true;
            M[id].homePhase = 1;
            startMoveSteps(id,20,true);
          }
        }

        if(M[id].stepsRemaining==0 &&
           M[id].autoScanning &&
           !M[id].sensorTriggered)
        {
          startMoveSteps(id,
            M[id].FULL_STEPS,
            !M[id].moveDirection);
        }
        
        // Save state when movement completes (only if not auto-scanning or homing)
        if(M[id].stepsRemaining == 0 && 
           !M[id].autoScanning && 
           !M[id].homingInProgress &&
           !M[id].waitingForSensorToRestore) {
          saveMotorState(id);
        }
      }
    }
  }
}


// ---------- HOME FSM ----------
void processHomePhases(int id){
  if(M[id].homePhase==0) return;
  if(M[id].stepsRemaining>0) return;

  if(M[id].homePhase==1){
    startMoveSteps(id,20,false);
    M[id].homePhase=2;
    return;
  }

  if(M[id].homePhase==2){
    M[id].currentPos=0;
    M[id].homeWaitUntil=millis()+200;
    M[id].homePhase=3;
    return;
  }

  if(M[id].homePhase==3){
    if(millis()>=M[id].homeWaitUntil){
      startMoveSteps(id,800,true);
      M[id].homePhase=4;
    }
    return;
  }

  if(M[id].homePhase==4){
    if(M[id].stepsRemaining==0){
      M[id].homeWaitUntil=millis()+200;
      M[id].homePhase=5;
    }
    return;
  }

  if(M[id].homePhase==5){
    if(millis()>=M[id].homeWaitUntil){
      startMoveSteps(id,800,false);
      M[id].homePhase=6;
    }
    return;
  }

  if(M[id].homePhase==6){
    if(M[id].stepsRemaining==0){
      M[id].homingInProgress=false;
      M[id].homePhase=0;
      M[id].sensorTriggered=false;
      M[id].autoScanning=false;
      M[id].currentPos=0;

      Serial.print("Motor ");
      Serial.print(id+1);
      Serial.println(" HOME COMPLETE");
      
      // Check if this motor was waiting to restore saved state
      if(M[id].waitingForSensorToRestore && M[id].hasSavedState) {
        // Restore the saved state now that homing is complete
        restoreSavedStateAfterHoming(id);
      } else {
        // No saved state, just save current state
        saveMotorState(id);
      }
    }
  }
}


// ---------- MOVE TO % ----------
void moveToPercentNonBlocking(int id,int percent){

  if(percent<0) percent=0;
  if(percent>100) percent=100;

  int target = (M[id].FULL_STEPS * percent) / 100;
  long diff = target - M[id].currentPos;
  if(diff==0) return;

  Serial.print("Motor ");
  Serial.print(id+1);
  Serial.print(" → Move to ");
  Serial.print(percent);
  Serial.print("%  (Target=");
  Serial.print(target);
  Serial.print("  Current=");
  Serial.print(M[id].currentPos);
  Serial.println(")");

  startMoveSteps(id,abs(diff),(diff>0));
}


// ---------- UART (WITH ALL-MOTOR COMMANDS + LOGGING) ----------
void processUART(String cmd){

  cmd.trim();
  cmd.toUpperCase();
  if(cmd.length()==0) return;

  Serial.print("CMD: ");
  Serial.println(cmd);


  // ===== GLOBAL ALL-MOTOR COMMANDS =====
  if(cmd.startsWith("ALL")){

    String rest = cmd.substring(3);
    rest.trim();

    if(rest=="STOP"){
      Serial.println("ALL MOTORS STOP");
      for(int i=0;i<6;i++) stopMoveImmediate(i);
      return;
    }

    if(rest=="HOME"){
      Serial.println("ALL MOTORS HOME START");
      for(int i=0;i<6;i++){
        M[i].sensorTriggered=false;
        M[i].autoScanning=true;
        M[i].waitingForSensorToRestore=false;  // Clear restoration flag on manual home
        startMoveSteps(i,M[i].FULL_STEPS,true);
      }
      return;
    }

    if(rest.startsWith("G")){
      int pct = rest.substring(1).toInt();
      Serial.print("ALL → G");
      Serial.print(pct);
      Serial.println("%");

      for(int i=0;i<6;i++)
        moveToPercentNonBlocking(i,pct);
      return;
    }

    if(rest.startsWith("SW")){
      int mode = rest.substring(2).toInt();
      if(mode<0||mode>4) return;

      Serial.print("ALL SWING MODE ");
      Serial.println(mode);

      for(int i=0;i<6;i++){
        M[i].swingMode = mode;
        M[i].swingActive = (mode!=0);
        M[i].swingGoingForward=true;
        M[i].swingWaitUntil=millis()+10;
        saveMotorState(i);  // Save swing state
      }
      return;
    }

    return;
  }


  // ===== PER-MOTOR COMMANDS =====
  if(cmd[0] != 'M') return;

  int id = cmd.charAt(1)-'1';
  if(id<0 || id>5) return;

  String rest = cmd.substring(2);
  rest.trim();

  if(rest=="HOME"){
    Serial.print("Motor ");
    Serial.print(id+1);
    Serial.println(" HOME START");

    M[id].sensorTriggered=false;
    M[id].autoScanning=true;
    M[id].waitingForSensorToRestore=false;  // Clear restoration flag on manual home
    startMoveSteps(id,M[id].FULL_STEPS,true);
    return;
  }

  if(rest=="STOP"){
    stopMoveImmediate(id);
    M[id].homingInProgress=false;
    M[id].homePhase=0;
    return;
  }

  if(rest.startsWith("G")){
    int pct = rest.substring(1).toInt();
    moveToPercentNonBlocking(id,pct);
    return;
  }

  if(rest.startsWith("SW")){
    int mode = rest.substring(2).toInt();
    if(mode<0||mode>4) return;

    Serial.print("Motor ");
    Serial.print(id+1);
    Serial.print(" SWING MODE ");
    Serial.println(mode);

    M[id].swingMode=mode;
    M[id].swingActive=(mode!=0);
    M[id].swingGoingForward=true;
    M[id].swingWaitUntil=millis()+10;
    saveMotorState(id);  // Save swing state
  }
}