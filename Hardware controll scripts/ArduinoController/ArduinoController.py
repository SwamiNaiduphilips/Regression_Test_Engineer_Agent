import time
import sys

from thermistor_utils import SH_converter

try:
    import serial
except:
    print ("")
    print ("Oh, my gosh, didn't you install https://pypi.org/project/pyserial ???")
    print ("")
    sys.exit(0)

#=========================================================================
#  Arduino communication class
#=========================================================================
class ArduinoComm(object):

    #=========================================================================
    #  __init__
    #=========================================================================
    def __init__(self, portNumber):

        # initialize temperature table
        readings = (
            (0, 357011),
            (25, 100000),
            (50, 33194),
        )
        self.conv = SH_converter.from_points(readings)

        try:
            self.serialPort = serial.Serial(port=portNumber,baudrate=9600, timeout=1)
            time.sleep(2) #! Kind of incredible stupid wait...but necessary.
            self.initSuccesfull = True
        except Exception as e:
            self.initSuccesfull = False
            print (e)

    #=========================================================================
    #  getInitStatus
    #=========================================================================       
    def getInitStatus(self):
        isInited = False

        if True is self.initSuccesfull:
            self._send( '1' )

            for i in range(5): #5x retries
                response = self._read()
                if 'ArduinoInitialized' == response:
                    isInited = True
                    break

        return isInited
            
    #=========================================================================
    #  closeInletCommm
    #=========================================================================       
    def closeArduinoCommm(self):
        self.serialPort.close()

        
    #=========================================================================
    #  sendArduinoCommand
    #=========================================================================
    def sendArduinoCommand (self, command ):
        if "" != command:
            #send the command
            self._send( command )
    

    #=========================================================================
    #  sendArduinoCommandWithResponse
    #=========================================================================
    def sendArduinoCommandWithResponse (self, command ):
        if "" != command:
            #send the command
            self._send( command )
            return self._read()


    #=========================================================================
    #  setARMToPosition
    #=========================================================================
    def setARMToPosition (self, position ):

        # set steps to position
        self._send( '4;'+position )

        result = ""
        timeout = 60
        starting_time = time.time()
        while result != 'ready' and time.time() < (starting_time + timeout) :    
            result = self._read()
    
    
    #=========================================================================
    #  setARMToPositionWithSpeedSetting
    #=========================================================================
    def setARMToPositionWithSpeedSetting (self, position, speed ):
        localSpeed = speed
        if 2 > speed:
            localSpeed = 2
        
        # set steps to position with specific speed
        self._send( '8;'+position+';'+str(localSpeed) )

        result = ""
        timeout = 60
        starting_time = time.time()
        while result != 'ready' and time.time() < (starting_time + timeout) :    
            result = self._read()

    
    #=========================================================================
    #  calibrateArm
    #=========================================================================
    def calibrateArm (self):

        # send calibrate command
        self._send( '3' )

        result = ""
        timeout = 60
        starting_time = time.time()
        while result != 'ready' and time.time() < (starting_time + timeout) :    
            result = self._read()


    #=========================================================================
    #  getArmPosition
    #=========================================================================
    def getArmPosition (self):

        # set steps to position
        self._send( '5' )
        return self._read()


    #=========================================================================
    #  powerDown
    #=========================================================================
    def powerDown (self):

        # set steps to position
        self._send( '6' )


    #=========================================================================
    #  setNTCtemperature
    #=========================================================================
    def setNTCtemperature (self, channel, temperature):
        # send calibrate command
        #step = -(( 8*( self.conv.resistance(temperature) - 100125 ) ) / 3125)
        
        localTemperature = 0
        # channel 1 Battery NTC minimum temp = -2c
        if 0 == channel:
            if temperature < -2:
                localTemperature = -2
            else:
                localTemperature = temperature
        # channel 2 motor NTC minimum temp = 25c
        else:
            if temperature < 25:
                localTemperature = 25
            else:
                localTemperature = temperature


        KOhm = round(self.conv.resistance(localTemperature)/1000)

        print ('Temp to set -> '+str(temperature)+'  KOhm -> '+str(KOhm))
        self._send( '7'+';'+str(channel)+';'+str( KOhm ) )


    #=========================================================================
    #  _send
    #=========================================================================
    def _send (self, data ):
        # set steps to position
        self.serialPort.write( (data+'|').encode() )


    #=========================================================================
    #  _read
    #=========================================================================
    def _read (self):
        return self.serialPort.read(20).decode()


#=========================================================================
#=========================================================================


if __name__ == "__main__":

    communication = ArduinoComm( "COM4" )

    #for x in range(0,250,5):
    #    print ("Loop nr = "+str(x) )
    #for temp in range(0,100,5):
    #    communication.setNTCtemperature ( 0, temp)
    #communication.setNTCtemperature ( 0, 25)
    #communication.setNTCtemperature ( 0, 30)
    #communication.setNTCtemperature ( 0, -2)

    communication.setNTCtemperature ( 1, 35)


    #for x in range(-2, 100):
    #    communication.setNTCtemperature ( 0, x)
    




    '''
    print ( communication.getInitStatus() )
    
    # Enable debug info
    #communication.sendArduinoCommand("2")

    # Calibrare
    print(time.time())
    communication.calibrateArm( 10 )
    print(time.time())

    # set steps to position
    communication.setARMToPosition("50")

    # get the current position
    print ( communication.getArmPosition() )

    # powerDown
    communication.powerDown()
    '''

   
    communication.closeArduinoCommm()

