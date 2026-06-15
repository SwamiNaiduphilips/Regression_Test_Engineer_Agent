// I2C interface to driver the WuL light driver
// by Freek Bosker

// Created 20 October 2021
#include "commands.h"
#include "serialCom.h"

void procesInput    ( void );
void processCommand ( int command );


/**********************************************************************************
 * Arduino SetUp function
 *********************************************************************************/
void setup() 
{
  setupCommands();
  setupSerialComm();
  pinMode(LED_BUILTIN, OUTPUT);
}

/**********************************************************************************
 * Arduino main loop function
 *********************************************************************************/
void loop() 
{
  // Check if a complete input is given
  if ( getInputComplete() )
  {
    // print input command
    printDebug( getInputCommand() );

    // process the input received from serial
    procesInput();

    // clear all received inputs
    clearInputs();
  }
}

/**********************************************************************************
*  INIT                   = 1   ( 1 )
*  SWDEBUG                = 2   ( 2 )
*  CALIBRATE              = 3   ( 3 )
*  SETANGLE               = 4   ( 4;<angle(int)> ) Side of limit switch is 0
*  GETCURRENTPOSITION     = 5 
*  POWERDOWN              = 6
*  SETNTCTEMPERATURE      = 7   ( 7;<channel(int)>;<value>(int) )
*  SETTOPOSITIONWITHSPEED = 8   ( 8;<position(int)>;<speedsetting(int)> ) Side of limit switch is 0
*  version                = 9   ( 9 )

 *********************************************************************************/
void procesInput()
{
     // Split the command in the correct values based on seperator
     int command_index = 0;
     String commandArray[4];
     String inputCommand = getInputCommand();
     
     for(int i=0; i< inputCommand.length(); i++)
     {
        if( inputCommand[i] != ';' )
        {
           commandArray[command_index] += inputCommand[i];
        }
        else
        {
          command_index++;
        }     
     }

    /* Based on command select action */
    switch(commandArray[0].toInt())
    {
      /***************************************************************************/
      case INIT:
        Serial.print("ArduinoInitialized");
        break;
            
      /***************************************************************************/  
      case SWDEBUG:
        setDebug();
        break;

     /***************************************************************************/
      case CALIBRATE:
        printDebug("CALIBRATE");
        Serial.print( Calibrate() );
        break;

      /***************************************************************************/  
      case SETTOPOSITION:
        printDebug("SETTOPOSITION");
        Serial.print( MoveToPosition( commandArray[1].toInt(), 0 ) ); // 0 for speedSetting because it is not used
        break;

      /***************************************************************************/  
      case SETTOPOSITIONWITHSPEED:
        printDebug("SETTOPOSITIONWITHSPEED");
        Serial.print( MoveToPosition( commandArray[1].toInt(), commandArray[2].toInt() ) );
        break;

      /***************************************************************************/  
      case GETCURRENTPOSITION:
        printDebug("GETCURRENTPOSITION");
        Serial.print( GetCurrentStepPosition() );
        break;

      /***************************************************************************/  
      case POWERDOWN:
        printDebug("POWERDOWN");
        powerDown();
        break;

     /***************************************************************************/  
      case SETNTCTEMPERATURE:
        printDebug("SETNTCTEMPERATURE");
        SetNTCTemperature( commandArray[1].toInt(), commandArray[2].toInt() );
        break; 

      /***************************************************************************/  
      case VERSION:
        printDebug("VERSION");
        Serial.print( "Current version 2.0" );
        break; 

      /***************************************************************************/ 
      default:
        Serial.println("Shit happend");
        break;
    }
}

/***********************************************************************************************************/
/***********************************************************************************************************/
