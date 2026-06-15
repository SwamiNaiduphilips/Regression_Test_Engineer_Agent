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
#  Frequentie Generator Class
#   VISA_ADDRESS = 'GPIB0::14::INSTR'  #33120A
#   VISA_ADDRESS = 'GPIB0::10::INSTR'  #33512B
#=========================================================================
class FreqGenerator(object):
    
    def __init__(self, GPIB_ADRESS, DeviceModelNr):

        try:
           # Create a connection (session) to the instrument
           self.resourceManager = visa.ResourceManager()
           self.session = self.resourceManager.open_resource(GPIB_ADRESS)
        except visa.Error as ex:
           print('Couldn\'t connect to \'%s\', exiting now...' % GPIB_ADRESS)

        self.deviceNr = DeviceModelNr
            
    #=====================================================================
    def closeConnection (self):

        # Reset the device first
        self._outPutOff()
        
        # Close the connection to the instrument
        self.session.close()
        self.resourceManager.close()

    #=====================================================================
    def setWaveForm (self, frequentie=1000, voltage=1, dutyCycle=50 ):

        # Reset the device first
        #self._outPutOff()
        
        if "33120A" == self.deviceNr :
            self.session.write('FUNC:SHAP SQU')
            self.session.write('FREQ %d'         % frequentie)
            self.session.write('VOLT %f'         % (voltage/2))
            self.session.write('VOLT:OFFS %f'    % (voltage/4))
            self.session.write('PULSe:DCYCle %d' % dutyCycle)

        if "33512B" == self.deviceNr :
            self.session.write('++auto 0')
            self.session.write('FUNC SQU')
            self.session.write('FUNC:SQU:DCYC %d'  % dutyCycle)
            self.session.write('FREQ %d'           % frequentie)
            self.session.write('VOLT:HIGH %f'      % voltage)
            self.session.write('VOLT:LOW 0.0')
            self.session.write('OUTP 1')


    #=====================================================================
    def _outPutOff (self):
        
        if "33120A" == self.deviceNr :
            self.session.write('APPLy:DC DEFault, DEFault, 0')
        else:
            self.session.write('*RST')
        


#=========================================================================
#=========================================================================

if __name__ == "__main__":

    fGen = FreqGenerator('GPIB0::14::INSTR', "33120A")

    fGen.setWaveForm ( 2000, 3, 60 )

    
            
    input('Press enter to exit...')
    
    fGen.closeConnection()

