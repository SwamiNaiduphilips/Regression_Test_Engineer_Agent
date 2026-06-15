
#include "commands.h"
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"
#include <SPI.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #1 (M1 and M2)
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 1);

int currentPos = 0;

/**********************************************************************************
 * setupCommands()
 *********************************************************************************/
void setupCommands(void)
{
  // Setup for motion arm
  pinMode(BUTTONPIN, INPUT_PULLUP);
  AFMS.begin();  // create with the default frequency 1.6KHz

  // Setup for NTC
  pinMode(REL_100_K, OUTPUT); // set the REL_100_K as an output:
  pinMode(REL_200_K, OUTPUT); // set the REL_200_K as an output:
  pinMode(CS_PIN, OUTPUT);    // set the CS_PIN as an output:
  SPI.begin();     // initialize SPI:

  //default set 100KOhm (25degrees)
  SetNTCTemperature( CHANNEL_1, DEFAULT_RESISTANCE_NTC ); // CHANNEL_1, 100KOhm
  SetNTCTemperature( CHANNEL_2, DEFAULT_RESISTANCE_NTC ); // CHANNEL_1, 100KOhm
   
}

/**********************************************************************************

 *********************************************************************************/
String Calibrate ( void )
{
  int buttonState = 0;
  
  myMotor->setSpeed(MOTORSPEED / SPEEDDEVIDER); // 10 rpm

  buttonState = digitalRead(BUTTONPIN);
  printDebug(String(buttonState));
  while (buttonState == HIGH) 
  {
    buttonState = digitalRead(BUTTONPIN);
    printDebug(String(buttonState));
    printDebug("finding 0");
    myMotor->step(STEP_SIZE * 1, FORWARD, MICROSTEP);
    wait(200);
  }
  printDebug("found 0");

  // step 1 backward to level the arm
  myMotor->step(STEP_SIZE * 1, BACKWARD, MICROSTEP);
  currentPos = MIN_STEP_POS;

  // After calibrating set arm position to 0, second 0 for default speed
  MoveToPosition( 0, 0);
  
  return "ready";
}


/**********************************************************************************

 *********************************************************************************/
String MoveToPosition( int posToSet, int speedSetting )
{
    // change the speedsetting when it is required
    if ( speedSetting >= SPEEDDEVIDER)
    {
      myMotor->setSpeed(speedSetting / SPEEDDEVIDER);  
    }
    
    if ( posToSet >= MIN_STEP_POS && posToSet <= MAX_STEP_POS )
    {
      int stepsToMove = abs(posToSet-currentPos);
      if ( posToSet > currentPos)
      {
        myMotor->step(STEP_SIZE * stepsToMove, BACKWARD, MICROSTEP);
        currentPos = currentPos+stepsToMove;
      }
      else
      {
        myMotor->step(STEP_SIZE * stepsToMove, FORWARD, MICROSTEP);
        currentPos = currentPos-stepsToMove;
      }
    }  

   return "ready";
}

/**********************************************************************************

 *********************************************************************************/
String GetCurrentStepPosition( void )
{
  return String(currentPos);   
}

/**********************************************************************************

 *********************************************************************************/
void powerDown( void )
{
  myMotor->release();
}


/**********************************************************************************

 *********************************************************************************/
void SetNTCTemperature    ( int channel, int KOhm )
{
  int usedChannel = 0;
  int localKOhm = 0;

  /*************************************************/
  /* Channel = 0 -> Battery NTC
  /*************************************************/
  if ( CHANNEL_1 == channel )
  {
    // Set the channel adress to channel 1
    usedChannel = 0x11;

    // do a check if the input resistance is !> MAX_RESISTANCE
    if (KOhm > MAX_RESISTANCE_BATT_NTC)
    {
      // limit local to MAX
      localKOhm = MAX_RESISTANCE_BATT_NTC;
    }
    else
    {
      // no problemo, locals = input
      localKOhm = KOhm;
    }  

    // set to relaiy swithches in correct setting
    if ( (localKOhm - 300) >= 0 )
    {
      localKOhm -= 300;
      digitalWrite(REL_100_K, REL_OPEN); 
      digitalWrite(REL_200_K, REL_OPEN);
    }
    else if ( (localKOhm - 200) >= 0 )
    {
      localKOhm -= 200;
      digitalWrite(REL_100_K, REL_CLOSED); 
      digitalWrite(REL_200_K, REL_OPEN);
    }
    else if ( (localKOhm - 100) >= 0 )
    {
      localKOhm -= 100;
      digitalWrite(REL_100_K, REL_OPEN); 
      digitalWrite(REL_200_K, REL_CLOSED);
    }
    else
    {
      digitalWrite(REL_100_K, REL_CLOSED); 
      digitalWrite(REL_200_K, REL_CLOSED);
    }
  
    // set the remaining resistance with to digital pot meter
    if ( localKOhm >=0 && localKOhm <=100 )
    {
      // if all is good and the remaning resistance is between 0 and 100, set the remaing via the digital potmeter
      //int StepToSet = constrain(-(( 8*( (localKOhm*1000) - 100125 ) ) / 3125), 0, 255);
      int StepToSet = map(localKOhm, 100, 0, 0, 255);
      
      // set the corresponding temp
      sendDataToNTC( usedChannel, StepToSet ); 
    }
    else
    {
      // something went wrong with localKOhm, so just set the default 100K (25degrees)
      digitalWrite(REL_100_K, REL_OPEN); 
      digitalWrite(REL_200_K, REL_CLOSED);
    }
  }
  /*************************************************/
  /* Channel = 1 -> Motor NTC
  /*************************************************/
  else
  {
    // Set the channel adress to channel 2
    usedChannel = 0x12;

    // do a check if the input resistance is !> MAX_RESISTANCE
    if (KOhm > MAX_RESISTANCE_MOTOR_NTC)
    {
      // limit local to MAX
      localKOhm = MAX_RESISTANCE_MOTOR_NTC;
    }
    else
    {
      // no problemo, localKOhm = input
      localKOhm = KOhm;
    }  

    //int StepToSet = -(( 8*( (KOhm*1000) - 100125 ) ) / 3125);
    int StepToSet = map(localKOhm, 100, 0, 0, 255);
      
    // set the corresponding temp
    sendDataToNTC( usedChannel, StepToSet ); 
  }
}


/**********************************************************************************
  sendDataToNTC
 *********************************************************************************/
void sendDataToNTC( int channel, int stepToSet )
{
    // set the CS pin to low to select the chip:
    digitalWrite(CS_PIN, LOW);
    
    // send the command and value via SPI:
    SPI.transfer(channel);
    SPI.transfer(stepToSet);
    
    // Set the CS pin high to execute the command:
    digitalWrite(CS_PIN, HIGH); 
}


/**********************************************************************************

 *********************************************************************************/
void wait(int msec) 
{
  long long startTime = millis();
  while (startTime + msec >= millis()) {
    delay (10);
  }
}
