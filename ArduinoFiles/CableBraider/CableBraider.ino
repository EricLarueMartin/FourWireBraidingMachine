/* Example sketch to control a stepper motor with L298N motor driver, Arduino UNO and AccelStepper.h library. Acceleration and deceleration. More info: https://www.makerguides.com */
// Include the AccelStepper library:
#include <AccelStepper.h>
#include <MultiStepper.h>
// Define the AccelStepper interface type:
#define MotorInterfaceType 4
// Create a new instance of the AccelStepper class:
AccelStepper braider = AccelStepper(MotorInterfaceType, 8, 9, 10, 11); // braiding section stepper motor
AccelStepper winder = AccelStepper(MotorInterfaceType, 4, 5, 6, 7); // winding section stepper motor

MultiStepper stepper;

// length unit is left off in names of variables, as it doesn't really matter to the program, but this was set up for centimeters on the winder circumference
// time is in seconds

// physical values used to calculate stepper speeds
const int HornGearTeeth = 26;
const int DriveGearTeeth = 6;
const int BraiderStepsPerRevolution = 200;
const int WinderStepsPerRevolution = 200;
const float RevolutionsPerBraid = 2.0; // revolutions of horn gear to make a complete braid cycle
const float WinderCircumference = 4.5 * 3.14159265; // in centimeters 

// constants that depend on other constants

// ratio of braiding stepper motor to winding gear to get one braid per centimeter
const float stepRatio = RevolutionsPerBraid*HornGearTeeth/DriveGearTeeth*WinderCircumference*BraiderStepsPerRevolution/WinderStepsPerRevolution; 

const float stepsPer = BraiderStepsPerRevolution/WinderCircumference; // steps per distance unit (cm) on winder

// user adjusted settings

float windingSpeed = 1.2; // one distance unit (cm) per second
float braidsPer = 0.1; // per distance unit (cm)
float braiderAcceleration = 25; // acceleration of braider in steps per second squared
int braidDirection = 1; // braiding motor turning direction

// values calculated based on user adjuted settings

// speed stepper motors run at in steps per second
float winderStepSpeed; // = windingSpeed/WinderCircumference*WinderStepsPerRevolution; 
float braiderStepSpeed; // = winderStepSpeed * stepRatio * braidsPer;

// run control
bool bRunning = false;
float cableLength; // target cable length

void braidCable(float cableLength) {
  // Set target position:
  winder.enableOutputs();
  braider.enableOutputs();
  long absolute[2];
  absolute[0] = cableLength*stepsPer; // set target winder position
  absolute[1] = braidDirection*cableLength*stepsPer*stepRatio; // braider position required to maintain braid ratio
  // use the current position as the starting position
  winder.setCurrentPosition(0);
  braider.setCurrentPosition(0);
  // set steppers to run for desired cable length
  stepper.moveTo(absolute);
  // Run to position with set speed and acceleration:
  Serial.print("Stepping to position ");
  Serial.print(cableLength);
  Serial.println(" cm.");
}

bool braidRunCheck() {
  if (stepper.run()){  
    Serial.print("Position ");
    Serial.print(winder.currentPosition()/stepsPer);
    Serial.println(" cm.");
    return true;
  } else return false;
}

void haltSteppers()
{
  Serial.println("Stopping steppers.");
  winder.stop();
  braider.stop();
  bRunning = false;
  displayPosition();
  winder.disableOutputs();
  braider.disableOutputs();
}

// recalculate the stepping speeds after parameter change
void recalc() {
  if (bRunning) haltSteppers();
  winderStepSpeed = windingSpeed/WinderCircumference*WinderStepsPerRevolution; 
  braiderStepSpeed = winderStepSpeed * stepRatio * braidsPer;
  winder.setMaxSpeed(winderStepSpeed);
  winder.setAcceleration(braiderAcceleration/stepRatio);
  braider.setMaxSpeed(braiderStepSpeed);
  braider.setAcceleration(braiderAcceleration);
}

void displaySettings(){
  Serial.print("stepsPer = ");
  Serial.println(stepsPer);
  Serial.print("braidsPer = ");
  Serial.println(braidsPer);
  Serial.print("windingSpeed = ");
  Serial.println(windingSpeed);
  Serial.print("winderStepSpeed = ");
  Serial.println(winderStepSpeed);
  Serial.print("braiderStepSpeed = ");
  Serial.println(braiderStepSpeed);
  Serial.print("braiderAcceleration = ");
  Serial.println(braiderAcceleration);
  Serial.print("braidDirection = ");
  Serial.println(braidDirection);
}

void displayPosition() {
  Serial.print("Position ");
  Serial.print(winder.currentPosition()/stepsPer);
  Serial.print(" cm. ( ");
  Serial.print(winder.currentPosition());
  Serial.print(" / ");
  Serial.print(braider.currentPosition());
  Serial.println(" )");      
}

void setup() {
// start serial port at 9600 bps
  Serial.begin(9600);
  
  stepper.addStepper(winder);
  stepper.addStepper(braider);

  recalc();
//  winder.setMaxSpeed(winderStepSpeed);
//  winder.setAcceleration(braiderAcceleration/stepRatio);
//  braider.setMaxSpeed(braiderStepSpeed);
//  braider.setAcceleration(braiderAcceleration);

// wait for serial port to connect. Needed for native USB port only
  while (!Serial) {;}
  displaySettings();
  Serial.println("A          : set acceleration of braiding motor in steps pre second squared.");
  Serial.println("B          : set braid density in braids per centimeters.");
  Serial.println("S          : set winding speed in centimeters per second.");
  Serial.println("D          : change braiding motor direction.");
  Serial.println("?          : display settings.");
  Serial.println("P          : report position in centimeters.");
  Serial.println("H          : halt motors.");
  Serial.println("R <length> : run to braid length of cable in centimeters.");
}

void loop() {
  while (!Serial.available()) {
    if (bRunning) // was running
      if (!stepper.run()) { // did it stop?     
        displayPosition();
        winder.disableOutputs();
        braider.disableOutputs();
        bRunning = false;
      }
  } 
  char arg = Serial.read();
  switch (toUpperCase(arg)) {
    case 'R' :
      cableLength = Serial.parseFloat();
      braidCable(cableLength);
      bRunning = true;
      break;
    case 'B' :
      braidsPer = Serial.parseFloat();
      Serial.print("Braid density set to ");
      Serial.print(braidsPer);
      Serial.println(" braids per cm.");  
      recalc();
      displaySettings();
      break;
    case 'S' :
      windingSpeed = Serial.parseFloat();
      Serial.print("Winding speed set to ");
      Serial.print(windingSpeed);
      Serial.println(" cm/s.");  
      recalc();
      displaySettings();
      break;
    case 'A' :
      braiderAcceleration = Serial.parseFloat();
      Serial.print("Braider acceleration set to ");
      Serial.print(braiderAcceleration);
      Serial.println(" steps/s^2.");  
      recalc();
      displaySettings();
      break;
    case '?' :
      displaySettings();
      break;
    case 'P' :
      displayPosition();
      break;
    case 'H' :
      haltSteppers();
      break;      
    case 'D' :
      braidDirection*=-1;
      displaySettings();
      break;
  }
}
