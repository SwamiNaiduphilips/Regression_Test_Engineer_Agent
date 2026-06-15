import sys
import time

try:
    import visa
except:
    print ("")
    print ("Oh boy, didn't you install https://pypi.org/project/pyvisa ?")
    print ("")
    sys.exit(0)

#=========================================================================
#  BatterySimulator Class
#   VISA_ADDRESS = 'GPIB0::5::INSTR'  #E3634A

#=========================================================================
class PowerSupply(object):
    
    def __init__(self, GPIB_ADRESS):

        try:
           # Create a connection (session) to the instrument
           self.resourceManager = visa.ResourceManager()
           self.session = self.resourceManager.open_resource(GPIB_ADRESS)
           self.session.write('*RST')
        except visa.Error as ex:
           print('Couldn\'t connect to \'%s\', exiting now...' % GPIB_ADRESS)


    #=====================================================================
    def setVoltage (self, Voltage):
        """
        set voltage
        voltage is in volt
        """
        self.session.write(":VOLT %f" % (Voltage))


    #=====================================================================
    def setCurrent (self, Current):
        """
        set current limit
        current is in amps
        """
        self.session.write(":CURR %f" % (Current))


    #=====================================================================
    def getVoltage (self):
        """
        return measured voltage at channel 1 or channel 2
        default channel is battery channel (#1), charger channel is channel (#2)
        """
        self.session.write(":VOLT?")
        return float(self.session.read())


    #=====================================================================  
    def getCurrent (self):
        """
        return measured current at channel 1 or channel 2
        """
        self.session.write(":CURR?")
        return float(self.session.read())

    
    #=====================================================================
    def Output(self, On):
        """
        set the OUTPUT off (False) or on (True) for channel 1 or 2
        """
        if On == True: #case insensitive comparison
            MODE = "ON"
        else: 
            MODE = "OFF"
        self.session.write(":OUTP %s" % ( MODE ))

        
    #=====================================================================
    def closeConnection (self):

        # Reset the device first
        self._outPutOff()
        
        # Close the connection to the instrument
        self.session.close()
        self.resourceManager.close()

 

    #=====================================================================
    def _outPutOff (self):
        
        self.session.write('*RST')
        


#=========================================================================
#=========================================================================

if __name__ == "__main__":

    GPIB_ADDRESS   = 'GPIB0::5::INSTR'

    #Create simulator object
    PowerSupply = PowerSupply(GPIB_ADDRESS)

    #input('Press enter to set voltage')
    PowerSupply.setVoltage( 15.5 )

    #input('Press enter to set currentlimit')
    PowerSupply.setCurrent( 0.4 )

    #input('Press enter to enable power supply')
    PowerSupply.Output( True ) #On
       
    #input('Press enter to disable power supply')
    PowerSupply.Output( False ) #Off
        
    input('Press enter to exit...')
    PowerSupply.closeConnection()

