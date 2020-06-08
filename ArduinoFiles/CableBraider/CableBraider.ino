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
  bool freeRunning = false;
  float cableLength; // target cable length

  char machineName[40] = "";

  cableBraider(int pins[8], char setName[40]); // constructor sets defaults
  void braidCable();
  void cableBraider::haltSteppers();
  void cableBraider::recalc();
  void cableBraider::displaySettings();
  void cableBraider::displayPosition();

};

/* pins for MotorInterfaceType = 4 (FULL4WIRE)
0-3 - winder L298N pins 1-4
4-8 - braider L298N pins 1-4   
*/
cableBraider::cableBraider(int pins[8], char setName[40] = "") // constructor sets defaults
{
  winderStepper = new AccelStepper(MotorInterfaceType,pins[0],pins[1],pins[2],pins[3]);
  braiderStepper = new AccelStepper(MotorInterfaceType,pins[4],pins[5],pins[6],pins[7]);

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
  memcpy(machineName,setName,40);
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
  Serial.print(machineName);
  Serial.print(" stepping to position ");
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

void cableBraider::haltSteppers()
{
  Serial.print("Stopping steppers on ");
  Serial.print(machineName);
  Serial.println(".");
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
  Serial.println(machineName);
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
  Serial.print(machineName);
  Serial.print("'s position is ");
  Serial.print(winderStepper->currentPosition()/StepsPerDistance);
  Serial.print(" cm. ( ");
  Serial.print(winderStepper->currentPosition());
  Serial.print(" / ");
  Serial.print(braiderStepper->currentPosition());
  Serial.println(" )");      
}

const int NumBraiders = 1;
const int BraiderPins[NumBraiders][8] = {
  {4,5,6,7,8,9,10,11} // pins for first braider
  };
const char MachineNames[NumBraiders][40] = {
  "Prototype B (0)"
};
  
cableBraider *braider[NumBraiders];

void setup() {
// start serial port at 9600 bps
  Serial.begin(9600);

// wait for serial port to connect. Needed for native USB port only
  for (int ctDelay = 10000; !Serial && ctDelay > 0; --ctDelay) { delay(1); }
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

  for (int ct = 0; ct < NumBraiders; ++ct) 
  {
    braider[ct] = new cableBraider(BraiderPins[ct],MachineNames[ct]);
    braider[ct]->recalc();
    braider[ct]->displaySettings();
  }
}

void loop() {
  while (!Serial.available())
    for (int ctBraider = 0; ctBraider < NumBraiders; ++ctBraider)
    {
      if (braider[ctBraider]->bRunning) // was running
      {
        braider[ctBraider]->bRunning = braider[ctBraider]->steppers->run();
        if (!braider[ctBraider]->bRunning && digitalRead(braider[ctBraider]->braidPin)) { // did it stop and braidPin is open?     
          braider[ctBraider]->displayPosition();
          braider[ctBraider]->winderStepper->disableOutputs();
          braider[ctBraider]->braiderStepper->disableOutputs();
        }          
      }
      else if (!digitalRead(braider[ctBraider]->braidPin)) // run for 1 cm if braidPin grounded
      {
        braider[ctBraider]->cableLength = 1.0;
        braider[ctBraider]->braidCable();
        braider[ctBraider]->bRunning = true;
      }
  } 
  static int nBraider = 0;
  char arg = Serial.read();
  switch (toUpperCase(arg)) {
    case 'M' :
      nBraider = Serial.parseInt();
      if (nBraider >= NumBraiders)
        nBraider = 0;
      Serial.print("Braiding machine ");
      Serial.print(nBraider);
      Serial.println(" selected.");  
      braider[nBraider]->displaySettings();
      break;
    case 'R' :
      braider[nBraider]->cableLength = Serial.parseFloat();
      Serial.print("Braiding for ");
      Serial.print(braider[nBraider]->cableLength);
      Serial.println(" cm.");  
      braider[nBraider]->braidCable();
      braider[nBraider]->bRunning = true;
      break;
    case 'B' :
      braider[nBraider]->braidsPer = Serial.parseFloat();
      Serial.print("Braid density set to ");
      Serial.print(braider[nBraider]->braidsPer);
      Serial.println(" braids per cm.");  
      braider[nBraider]->recalc();
      braider[nBraider]->displaySettings();
      break;
    case 'S' :
      braider[nBraider]->windingSpeed = Serial.parseFloat();
      Serial.print("Winding speed set to ");
      Serial.print(braider[nBraider]->windingSpeed);
      Serial.println(" cm/s.");  
      braider[nBraider]->recalc();
      braider[nBraider]->displaySettings();
      break;
    case 'A' :
      braider[nBraider]->braiderAcceleration = Serial.parseFloat();
      Serial.print("Braider acceleration set to ");
      Serial.print(braider[nBraider]->braiderAcceleration);
      Serial.println(" steps/s^2.");  
      braider[nBraider]->recalc();
      braider[nBraider]->displaySettings();
      break;
    case '?' :
      braider[nBraider]->displaySettings();
      break;
    case 'P' :
      braider[nBraider]->displayPosition();
      break;
    case 'H' :
      braider[nBraider]->haltSteppers();
      break;      
    case 'D' :
      braider[nBraider]->braidDirection*=-1;
      braider[nBraider]->displaySettings();
      break;
  }
}
