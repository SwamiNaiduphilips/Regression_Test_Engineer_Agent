#include "serialCom.h"

/***********************************************************
 * Variables
 **********************************************************/
String inputString;      // a string to hold incoming data
boolean stringComplete;  // whether the string is complete
boolean DEBUG;      // whether to print debug text or not

/**********************************************************************************
 * setupSerialComm()
 *********************************************************************************/
void setupSerialComm(void)
{
  Serial.begin(BAUDRATE);  

  DEBUG = false;
  inputString.reserve(20);
  inputString = "";
  stringComplete = false;
}

/**********************************************************************************
 * serialEvent()
 *********************************************************************************/
void serialEvent() 
{
  while (Serial.available()) 
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    
    // add it to the inputString:
    inputString += inChar;
    
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '|') 
    {
      stringComplete = true;
    }
  }
}

/**********************************************************************************
 * clearInputs()
 *********************************************************************************/
void clearInputs( void )
{
  // clear the string:
  inputString = "";
  stringComplete = false;
}

/**********************************************************************************
 * getInputComplete()
 *********************************************************************************/
boolean getInputComplete()
{
  return stringComplete;
}

/**********************************************************************************
 * getInputCommand()
 *********************************************************************************/
String getInputCommand()
{
  return inputString;
}

/**********************************************************************************
 * setDebug()
 *********************************************************************************/
void setDebug()
{
  if(DEBUG)
  {
    DEBUG = false;
    Serial.print("Debug disabled");
  }
  else
  {
    DEBUG = true;
    Serial.print("Debug enabled");
  }
}

/**********************************************************************************
 * printDebug()
 *********************************************************************************/
void printDebug( String debugText )
{
  if(true == DEBUG)
  {
    Serial.println(debugText);
  }
}
