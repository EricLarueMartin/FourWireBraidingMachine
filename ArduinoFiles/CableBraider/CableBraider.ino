// length unit is left off in names of variables, as it doesn't really matter to the program, but the constants were set for centimeters and seconds

// Include the AccelStepper library:
#include <AccelStepper.h>
#include <MultiStepper.h>
// Define the AccelStepper interface type:
#define MotorInterfaceType 4

// physical values used to calculate stepper speeds
const int HornGearTeeth = 26;
const int DriveGearTeeth = 6;
const int BraiderStepsPerRevolution = 200;
const int WinderStepsPerRevolution = 200;
const float RevolutionsPerBraid = 2.0; // revolutions of horn gear to make a complete braid cycle
const float WinderCircumference = 4.5 * 3.14159265; // in centimeters 

// constants that depend on other constants

const float StepsPerDistance = WinderStepsPerRevolution/WinderCircumference; // steps per distance unit (cm) on winder
const float StepsPerBraid = BraiderStepsPerRevolution*HornGearTeeth/DriveGearTeeth*RevolutionsPerBraid; // steps to complete a braiding cycle

// ratio of braiding stepper motor to winding gear to get one braid per centimeter
const float StepRatio =  StepsPerBraid/StepsPerDistance;


class cableBraider
{
 public: 

  AccelStepper *braiderStepper,*winderStepper;
  MultiStepper *steppers;

  // user adjusted settings and defaults
  float windingSpeed = 0.1; // one distance unit (cm) per second
  float braidsPer = 1.0; // per distance unit (cm)
  float braiderAcceleration = 25.0; // acceleration of braider in steps per second squared
  int braidDirection = 1; // braiding motor turning direction

  int braidPin = 13; // default is all braiders run if pin 13 is grounded
  
  // values calculated based on user adjuted settings
  
  // speed stepper motors run at in steps per second
  float winderStepSpeed; // = windingSpeed/WinderCircumference*WinderStepsPerRevolution; 
  float braiderStepSpeed; // = winderStepSpeed * stepRatio * braidsPer;
  
  // run control
  bool bRunning = false;
  bool outputsEnabled = false;
  float cableLength; // target cable length

  cableBraider(int pin0, int pin1, int pin2, int pin3, int pin4, int pin5, int pin6, int pin7); // constructor sets defaults
  void braidCable();
  bool cableBraider::runCheck();
  void cableBraider::haltSteppers();
  void cableBraider::recalc();
  void cableBraider::displaySettings();
  void cableBraider::displayPosition();

};

/* pins for MotorInterfaceType = 4 (FULL4WIRE)
0-3 - winder L298N pins 1-4
4-8 - braider L298N pins 1-4   
*/
cableBraider::cableBraider(int pin0, int pin1, int pin2, int pin3, int pin4, int pin5, int pin6, int pin7) // constructor sets defaults
{
  winderStepper = new AccelStepper(MotorInterfaceType,pin0,pin1,pin2,pin3);
  braiderStepper = new AccelStepper(MotorInterfaceType,pin4,pin5,pin6,pin7);

  steppers = new MultiStepper;

  steppers->addStepper(*winderStepper);
  steppers->addStepper(*braiderStepper);

  // this part should be redundant, but just in case
  windingSpeed = 0.1;
  braidsPer = 1.0;
  braiderAcceleration = 25.0;
  braidDirection = 1;
  braidPin = 13;
  bRunning = false;
  cableLength = 0.0;
  outputsEnabled = false;
}

void cableBraider::braidCable() {
  // Set target position:
  winderStepper->enableOutputs();
  braiderStepper->enableOutputs();
  outputsEnabled = true;
  long absolute[2];
  absolute[0] = cableLength*StepsPerDistance; // set target winder position
  absolute[1] = braidDirection*cableLength*StepsPerDistance*braiderStepSpeed/winderStepSpeed; // braider position required to maintain braid ratio
  // use the current position as the starting position
  winderStepper->setCurrentPosition(0);
  braiderStepper->setCurrentPosition(0);
  // Run to position with set speed and acceleration:
  Serial.print("Stepping to position ");
  Serial.print(cableLength);
  Serial.print(" cm. ( ");
  Serial.print(absolute[0]);
  Serial.print(" / ");
  Serial.print(absolute[1]);
  Serial.println(" )");      
  // set steppers to run for desired cable length
//  delay(100);
  steppers->moveTo(absolute);
}

bool cableBraider::runCheck() {
  if (steppers->run()){  
    Serial.print("Position ");
    Serial.print(winderStepper->currentPosition()/StepsPerDistance);
    Serial.println(" cm.");
    return true;
  } else return false;
}

void cableBraider::haltSteppers()
{
  Serial.println("Stopping steppers.");
  winderStepper->stop();
  braiderStepper->stop();
  bRunning = false;
  displayPosition();
  winderStepper->disableOutputs();
  braiderStepper->disableOutputs();
  outputsEnabled = false;
}

// recalculate the stepping speeds after parameter change
void cableBraider::recalc() 
{
  if (bRunning) haltSteppers();
  winderStepSpeed = windingSpeed*StepsPerDistance; 
  braiderStepSpeed = winderStepSpeed * StepRatio * braidsPer;
  winderStepper->setMaxSpeed(winderStepSpeed);
  winderStepper->setSpeed(winderStepSpeed);
  winderStepper->setAcceleration(braiderAcceleration/(StepRatio*braidsPer));
  braiderStepper->setMaxSpeed(braiderStepSpeed);
  braiderStepper->setSpeed(braiderStepSpeed);
  braiderStepper->setAcceleration(braiderAcceleration);
}

void cableBraider::displaySettings()
{
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

void cableBraider::displayPosition() 
{
  Serial.print("Position ");
  Serial.print(winderStepper->currentPosition()/StepsPerDistance);
  Serial.print(" cm. ( ");
  Serial.print(winderStepper->currentPosition());
  Serial.print(" / ");
  Serial.print(braiderStepper->currentPosition());
  Serial.println(" )");      
}

cableBraider braider(4,5,6,7,8,9,10,11);

void setup() {
// start serial port at 9600 bps
  Serial.begin(9600);

// wait for serial port to connect. Needed for native USB port only
  int ctDelay = 10000; // milliseconds to wait on serial
  while (!Serial) {
    delay(1);
    --ctDelay;
  };
  if (Serial)
  {
    Serial.print("StepsPerDistance = ");
    Serial.println(StepsPerDistance);
    Serial.print("StepsPerBraid = ");
    Serial.println(StepsPerBraid);
    Serial.print("Ratio for 1 braid per cm = ");
    Serial.println(StepRatio);
    Serial.println("A          : set acceleration of braiding motor in steps pre second squared.");
    Serial.println("B          : set braid density in braids per centimeters.");
    Serial.println("S          : set winding speed in centimeters per second.");
    Serial.println("D          : change braiding motor direction.");
    Serial.println("?          : display settings.");
    Serial.println("P          : report position in centimeters.");
    Serial.println("H          : halt motors.");
    Serial.println("R <length> : run to braid length of cable in centimeters.");
  }
  pinMode(13,INPUT_PULLUP);
  
  braider.recalc();
  braider.displaySettings();

}

void loop() {
  while (!Serial.available()) {
    if (braider.bRunning) // was running
    {
      if (!braider.steppers->run()) { // did it stop?     
        braider.displayPosition();
        braider.winderStepper->disableOutputs();
        braider.braiderStepper->disableOutputs();
        braider.bRunning = false;
      }
    }
    else if (!digitalRead(braider.braidPin)) // run pin is grounded
    {
        if (Serial &&  millis()%10000 == 0)
          Serial.println("Free running.");
        if (!braider.outputsEnabled)
        { // if outputs aren't enabled then run to braid a short amount of cable just to get settings going
          braider.cableLength = 1.0;
          braider.braidCable();
        }
        braider.winderStepper->runSpeed();
        braider.braiderStepper->runSpeed();
    }
  } 
  char arg = Serial.read();
  switch (toUpperCase(arg)) {
    case 'R' :
      braider.cableLength = Serial.parseFloat();
      Serial.print("Braiding for ");
      Serial.print(braider.cableLength);
      Serial.println(" cm.");  
      braider.braidCable();
      braider.bRunning = true;
      break;
    case 'B' :
      braider.braidsPer = Serial.parseFloat();
      Serial.print("Braid density set to ");
      Serial.print(braider.braidsPer);
      Serial.println(" braids per cm.");  
      braider.recalc();
      braider.displaySettings();
      break;
    case 'S' :
      braider.windingSpeed = Serial.parseFloat();
      Serial.print("Winding speed set to ");
      Serial.print(braider.windingSpeed);
      Serial.println(" cm/s.");  
      braider.recalc();
      braider.displaySettings();
      break;
    case 'A' :
      braider.braiderAcceleration = Serial.parseFloat();
      Serial.print("Braider acceleration set to ");
      Serial.print(braider.braiderAcceleration);
      Serial.println(" steps/s^2.");  
      braider.recalc();
      braider.displaySettings();
      break;
    case '?' :
      braider.displaySettings();
      break;
    case 'P' :
      braider.displayPosition();
      break;
    case 'H' :
      braider.haltSteppers();
      break;      
    case 'D' :
      braider.braidDirection*=-1;
      braider.displaySettings();
      break;
  }
}
