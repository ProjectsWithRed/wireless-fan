#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <Stepper.h>
#include <Preferences.h>


#define motorInterfaceType 1


const char auth[] = "your-blynk-app-authentication-code";
const char ssid[] = "wifi-name";
const char pass[] = "wifi-password";


const int FAN_PWM_PIN = 25;

// Initial fan speed and rotation speed.
const int INIT_FAN_SPEED = 1200;
const int INIT_ROT_SPEED = 100;


const int SERVO_PIN = 33;


// For the fan horizontal rotation.
const int STEPPER_STEP_PIN = 16;
const int STEPPER_DIR_PIN = 17;


const int SERVO_SPEED = 10;  // Above and below 90 (for both spin directions).

const int STEPPER_STEPS = 10;  // Stepper steps movement at a time.

// The speed when the stepper is controlled manually using the buttons.
const int STEPPER_CONTROL_SPEED = 250;


Servo servo;
Stepper stepper(200, STEPPER_DIR_PIN, STEPPER_STEP_PIN);

Preferences preferences;


int prevStepperSpeed = INIT_ROT_SPEED;  // Will hold previous stepper speed.

// The current position/step of the stepper motor, where 0 is facing the front of the fan.
int stepperCurrStep = 0;

int autoModeDir = -1;  // The direction of the fan rotation when in auto mode, -1=left, 1=right.


// The start and end positions of the fan rotation when in auto mode. This is relative to when the fan is facing the front.
int stepperStartSteps = -1000;
int stepperEndSteps = 1000;


// To hold state of the buttons from the Blynk app.
int tiltUp, tiltDown, autoMode, turnLeft, turnRight, setPos1, setPos2;

// To hold the value of the sliders from the Blynk app.
int fanSpeed;
int rotSpeed;


void setup() {
    preferences.begin("wireless-fan", false);  // False for read/write.

    servo.attach(SERVO_PIN);

    // PWM setup for the fan speed.
    ledcSetup(3, 25000, 12);  // Channel 3, 25000Hz frequency, 12-bits res.
    ledcAttachPin(FAN_PWM_PIN, 3);
    handleFanSpeed();

    loadSavedData();

    Blynk.begin(auth, ssid, pass);
}



// When any of the widgets in Blynk are changed, the corresponding variable will be updated.
BLYNK_WRITE(V0) { tiltUp = param.asInt(); }
BLYNK_WRITE(V1) { tiltDown = param.asInt(); }
BLYNK_WRITE(V2) { fanSpeed = param.asInt(); preferences.putInt("fanSpeed", fanSpeed); }
BLYNK_WRITE(V3) { autoMode = param.asInt(); }
BLYNK_WRITE(V4) { rotSpeed = param.asInt(); preferences.putInt("rotSpeed", rotSpeed); }
BLYNK_WRITE(V5) { turnLeft = param.asInt(); }
BLYNK_WRITE(V6) { turnRight = param.asInt(); }
BLYNK_WRITE(V7) { setPos1 = param.asInt(); }
BLYNK_WRITE(V8) { setPos2 = param.asInt(); }


// When the Blynk app and the ESP32 are connected, set the default/init widget values.
BLYNK_CONNECTED() {
    Blynk.virtualWrite(V2, fanSpeed);
    Blynk.virtualWrite(V4, rotSpeed);
}


void loadSavedData() {
    // Load the preference data, fan speed and rotation speed.
    
    fanSpeed = preferences.getInt("fanSpeed", INIT_FAN_SPEED);
    rotSpeed = preferences.getInt("rotSpeed", INIT_ROT_SPEED);
}


void handleServo() {
    // Control the servo motor (fan tilt).
        
    if(tiltUp) {
        servo.write(90 + SERVO_SPEED);
    } else if (tiltDown) {
        servo.write(90 - SERVO_SPEED);
    } else {
        servo.write(90);
    }
}


void handleStepper() {
    // Control the stepper motor (the fan rotation).

    if(autoMode) {
        autoRotMode();
    } else {
        manualRotMode();

        // Updates the start and end positions of the fan rotation.
        if(setPos1) {
            stepperStartSteps = stepperCurrStep;
        }
        if(setPos2) {
            stepperEndSteps = stepperCurrStep;
        }
    }
}


void autoRotMode() {
    // Rotate the fan back and forward, between the start and end stepper steps.

    updateStepperSpeed(rotSpeed);

    // Step towards the provided stepper start steps location.
    if(stepperCurrStep >= stepperStartSteps && autoModeDir == -1) {
        stepper.step(-STEPPER_STEPS);
        stepperCurrStep -= STEPPER_STEPS;
    } else {
        autoModeDir = 1;
    }

    // Step towards the other direction, the provided stepper end steps location.
    if(stepperCurrStep <= stepperEndSteps && autoModeDir == 1) {
        stepper.step(STEPPER_STEPS);
        stepperCurrStep += STEPPER_STEPS;
    } else {
        autoModeDir = -1;
    }
}


void manualRotMode() {
    // Control the stepper motor manually using left and right buttons.

    updateStepperSpeed(STEPPER_CONTROL_SPEED);  // Setting speed to default control speed.

    if(turnLeft) {
        stepper.step(-STEPPER_STEPS);
        stepperCurrStep -= STEPPER_STEPS;
    } else if (turnRight) {
        stepper.step(STEPPER_STEPS);
        stepperCurrStep += STEPPER_STEPS;
    } else {
        stepper.step(0);
    }
}


void updateStepperSpeed(int newStepperSpeed) {
    // Update stepper speed only if it has changed.
    
    if(newStepperSpeed != prevStepperSpeed) {
        stepper.setSpeed(newStepperSpeed);
        prevStepperSpeed = newStepperSpeed;
    }
}


void handleFanSpeed() {
    ledcWrite(3, fanSpeed);
}


void loop() {
    Blynk.run();
    
    handleServo();
    
    handleStepper();

    handleFanSpeed();
}
