/***********************************************************
 * Includes
 **********************************************************/
#include "Arduino.h"
#include "serialCom.h"

/***********************************************************
 * Defines
 **********************************************************/
#define BUTTONPIN 2

#define REL_100_K 6
#define REL_200_K 7

#define MAX_RESISTANCE_BATT_NTC 400
#define MAX_RESISTANCE_MOTOR_NTC 100
#define DEFAULT_RESISTANCE_NTC 100

#define CHANNEL_1 0
#define CHANNEL_2 0

#define REL_CLOSED HIGH
#define REL_OPEN LOW

#define STEP_SIZE 1
#define MIN_STEP_POS -10
#define MAX_STEP_POS 110
#define MOTORSPEED 254
#define SPEEDDEVIDER 2
#define CS_PIN 10 //for NTC

enum {  INIT=1,
        SWDEBUG=2,
        CALIBRATE=3,
        SETTOPOSITION=4,
        GETCURRENTPOSITION=5,
        POWERDOWN=6,
        SETNTCTEMPERATURE=7,
        SETTOPOSITIONWITHSPEED=8,
        VERSION=9
        };

/***********************************************************
 * Write Functions
 **********************************************************/
String Calibrate          ( void );
String MoveToPosition     ( int posToSet, int speedSetting );

void SetNTCTemperature    ( int channel, int KOhm );


/***********************************************************
 * Read Functions
 **********************************************************/
String GetCurrentStepPosition( void );

/***********************************************************
 * Other Functions
 **********************************************************/
void setupCommands( void );
void wait( int msec );
void powerDown ( void );
void sendDataToNTC( int channel, int stepToSet );
