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
#   VISA_ADDRESS = 'GPIB0::1::INSTR'  #2306

#=========================================================================
class BatterySimulator(object):
    
    def __init__(self, GPIB_ADRESS, Channel):

        self.channel = Channel
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
        set voltage at channel 1 or channel 2
        """
        self.session.write(":SOUR%d:VOLT %f" % (self.channel, Voltage))


    #=====================================================================
    def setCurrent (self, expectedCurrent):
        """
        set current limit at channel 1 or channel 2
        current is in amps
        """
        if( expectedCurrent <= 1):
            self.session.write("SOUR%d:CURR 1" % (self.channel)) #1 for lower current range
        else:
            self.session.write("SOUR%d:CURR 5" % (self.channel)) #5 for higher current range

    #=====================================================================
    def setCurrentRange (self, expectedCurrent):
        """
        set current limit at channel 1 or channel 2
        current is in amps
        """
        if( expectedCurrent <= 1):
            self.session.write("SENS:CURR:RANG:AUTO 1") #1 for lower current range
        else:
            self.session.write("SENS:CURR:RANG MIN") #5 for higher current range

    #=====================================================================
    def setImpedance (self, Impedance):
        """
        set current limit at channel 1 or channel 2
        impedance in ohm => <1
        """
        self.session.write(":OUTP%d:IMP %f" % (self.channel, Impedance))   


    #=====================================================================
    def getVoltage (self):
        """
        return measured voltage at channel 1 or channel 2
        default channel is battery channel (#1), charger channel is channel (#2)
        """
        self.session.write(":MEAS%d:VOLT?" % (self.channel))
        ##self.session.write("++read eoi") # Read until EOI asserted by instrument
        return round(float(self.session.read()),3) # 3 digits after the komma


    #=====================================================================  
    def getCurrent (self):
        """
        return measured current at channel 1 or channel 2
        """
        self.session.write(":MEAS%d:CURR?" % (self.channel))
        ##self.session.write("++read eoi") # Read until EOI asserted by instrument
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
        self.session.write(":OUTP%d %s" % (self.channel, MODE))

        
    #=====================================================================
    def closeConnection (self):

        # Reset the device first
        self._outPutOff()
        
        # Close the connection to the instrument
        self.session.close()
        self.resourceManager.close()

 

    #=====================================================================
    def _outPutOff (self):
        self.setVoltage( 0 ) #voltage to 0
        self.setCurrent( 5)  #currentlimit to 5
        self.setImpedance( 0)#impedance to 0
        self.session.write('*RST')
        


#=========================================================================
#=========================================================================

if __name__ == "__main__":

    defaultChannel = 1
    GPIB_ADDRESS   = 'GPIB0::1::INSTR'

    #Create simulator object
    Simulator = BatterySimulator(GPIB_ADDRESS, defaultChannel)

    #input('Press enter to continu 1')
    Simulator.setVoltage( 3.7 ) #volts
    #input('Press enter to continu 2')
    Simulator.setCurrent( 5) #amps 1 or 5
    #input('Press enter to continu 3')
    Simulator.setImpedance( 0.1)
    #input('Press enter to continu 4')

    Simulator.setCurrentRange (1)

    Simulator.Output( True ) #set output on

    #input('Press enter to get voltage')
    #print ('Voltage = %f' % Simulator.getVoltage())
    #input('Press enter to get current')
    #print ('Current = %f' % Simulator.getCurrent())

    #input('Press enter to set output off')
    #Simulator.Output( False ) #set output off
    
    #input('Press enter to exit...')
    Simulator.closeConnection()

