import sys
import time
import os
from datetime import datetime
from matplotlib import pyplot as plt

try:
    import visa
except:
    print ("")
    print ("Oh boy, didn't you install https://pypi.org/project/pyvisa ?")
    print ("")
    sys.exit(0)

#=========================================================================
#logging = True # ONLY FOR DEBUGGING
logging = False # ONLY FOR DEBUGGING
#=========================================================================

#=========================================================================
#  Amp meter Class
#   VISA_ADDRESS = 'GPIB0::18::INSTR'  #DMM6500
#=========================================================================
class AmpMeter(object):

    #===========================================================================
    # __init__
    #===========================================================================
    def __init__(self, GPIB_ADRESS, debugProduct, testStand=False):

        try:
           # Create a connection (session) to the instrument
           self.resourceManager = visa.ResourceManager()
           self.session = self.resourceManager.open_resource(GPIB_ADRESS)
           self.samples_per_second = 100000
           self.session.write('*RST')
        except visa.Error as ex:
           print('Couldn\'t connect to \'%s\', exiting now...' % GPIB_ADRESS)
        
        self.debugProduct = debugProduct
        self.testStand = testStand

        if True == logging :
            self.logfile = "D:\\Testsystem_reports\\"+self.debugProduct+"\\logging.txt"
            self.logger = open(self.logfile, "w")
    # END

            
    #===========================================================================
    # _logline
    #===========================================================================
    def _logline (self, text ):
        if logging == True:
            self.logger.write("%s: %s\n" % ( datetime.now().strftime('%H_%M_%S'), text) )
    # END
    

    #===========================================================================
    # doCurrentMeasurement
    #===========================================================================
    def doCurrentMeasurement (self, buffer_name, measureTime_sec, expectedCurrent ):
        self._logline("doCurrentMeasurement")

        # Calculate buffer size and create buffer 
        buffer_size = measureTime_sec * self.samples_per_second
        self.createBuffer ( buffer_name, buffer_size )
        
        # Set samplerate and count
        self.setSampleRateAndCount ( buffer_size )

        # Set the correct current range
        self.setCurrentRange ( expectedCurrent )

        # Show active buffer info
        self.session.write( ':DISP:BUFF:ACT "%s"' % buffer_name )

        # Trigger the measurement
        input('Press enter to continu...')
        self.triggerMeasurement ( buffer_name )
        time.sleep( measureTime_sec+2 )

        # Create list for measured samples
        measurementRangeSamples = []

        # Get all the samples and convert from string to numbers
        measurementRangeSamples = self.getMeasaurementSamples( buffer_name, buffer_size )
        measurementRangeSamples = list(map(float, measurementRangeSamples))

        # Debugging and logging purposes only
        if True == logging :
            self._writeLoggingToFile(measurementRangeSamples, "doCurrentMeasurement"  )
        
        if False == self.testStand :
            plt.plot(measurementRangeSamples)
            plt.show()

        # Calculate the average current of the measurement
        average_current = sum(measurementRangeSamples) / len(measurementRangeSamples)
        
        #delete the buffer again
        self.deleteBuffer( buffer_name )

        #Reset current range to 3A voor "motor" reasons
        self.setCurrentRange ( 3 )
        
        return average_current
    # END


    #===========================================================================
    # getMeasaurementSamples
    #===========================================================================
    def getMeasaurementSamples (self, buffer_name, buffer_size ):
        self._logline("getMeasaurementSamples")
        
        data = []
        nrOfSample = 500

        for x in range(0, int(buffer_size),nrOfSample):
            text = self.session.query( ':TRAC:DATA? %d,%d, "%s"' % ( x+1, x+nrOfSample, buffer_name ))
            data = data + text.split(',')
            if False == self.testStand :
                print ('Reading... %d' % len(data))
       
        return data
    # END

    
    #===========================================================================
    # triggerMeasurement
    #===========================================================================
    def triggerMeasurement (self, buffer_name ):
        # do a digitize current measurement with the buffer with 'buffer_name'
        self._logline("triggerMeasurement")
        
        self.session.write( ':MEASure:DIGitize:CURRent? "%s"' % ( buffer_name ) )
        self.session.write( '*TRG' )
    # END

    
    #===========================================================================
    # createBuffer
    #===========================================================================
    def createBuffer (self, buffer_name, buffer_size ):
        # creates the buffer with 'buffer_name' and size of 'buffer_size'
        self._logline("createBuffer")
        
        self.session.write( 'TRACe:MAKE "%s", %d, FULL' % ( buffer_name, buffer_size ) )
        self.session.write( 'TRACe:FILL:MODE ONCE, "%s"' % ( buffer_name ))
        self.session.write( ':DISP:BUFF:ACT "%s" ' % ( buffer_name ))
        self.session.write( ':DISPlay:SCReen SWIPE_STATistics')
    # END


    #===========================================================================
    # deleteBuffer
    #===========================================================================
    def deleteBuffer (self, buffer_name ):
        # deletes the buffer with 'buffer_name'
        self._logline("deleteBuffer")
        
        self.session.write( 'TRACe:DELete "%s"' % ( buffer_name ) )
    

    #===========================================================================
    # clearBuffer
    #===========================================================================
    def clearBuffer (self, buffer_name ):
        # cleares the buffer with 'buffer_name'
        self._logline("clearBuffer")
        
        self.session.write( 'TRACe:CLEar "%s"' % ( buffer_name ) )
    # END
    

    #===========================================================================
    # setCurrentRange
    #===========================================================================
    def setCurrentRange (self, expectedCurrent ):
        self._logline("setCurrentRange")

        currentRange = self.calculateCurrentRange ( expectedCurrent )
        
        self.session.write( ':DIG:CURR:RANG %s' % (currentRange))
    # END
    

    #===========================================================================
    # calculateCurrentRange
    #===========================================================================
    def calculateCurrentRange (self, expectedCurrent ):
        self._logline("calculateCurrentRange")

        # Default currentrange is set to 3A
        currentRange = "3"
        
        if ( expectedCurrent > 0 and expectedCurrent <= 1e-4 ):
            currentRange = "1e-4" # 100uA (0.0001)

        if ( expectedCurrent > 1e-4 and expectedCurrent <= 1e-3):
            currentRange = "1e-3" # 1mA (0.001)

        if ( expectedCurrent > 1e-3 and expectedCurrent <= 1e-2):
            currentRange = "1e-2" # 10mA (0.01)

        if ( expectedCurrent > 1e-2 and expectedCurrent <= 1e-1):
            currentRange = "1e-1" # 100mA (0.1)

        if ( expectedCurrent > 1e-1 ):
            currentRange = "3"    # 3A (3.0)

        return currentRange
    # END
    

    #===========================================================================
    # setSampleRateAndCount
    #===========================================================================
    def setSampleRateAndCount (self, buffer_size ):
        self._logline("setSampleAndCount")
        
        self.session.write( 'DIG:FUNC "CURR"')
        self.session.write( 'DIG:CURR:SRATE %d' % (self.samples_per_second))
        self.session.write( 'DIG:CURR:APER AUTO')
        self.session.write( 'DIG:COUN %d' % (buffer_size))
    # END

        
    #===========================================================================
    # closeConnection
    #===========================================================================
    def closeConnection (self):
        # Close the connection to the instrument
        self._logline("closeConnection")
        
        self.session.close()
        self.resourceManager.close()

        if logging == True:
            self.logger.close()
    # END

    #===========================================================================
    # _writeLoggingToFile
    #===========================================================================
    def _writeLoggingToFile(self, buffer_to_write, name ):
        #write the buffer to the debug_file
        self._logline("_writeLoggingToFile")
        logdir_p1 = "D:\\Testsystem_reports\\"
        logdir_p2 = "\\TMP_current_logging\\"

        if False == os.path.isdir(logdir_p1+self.debugProduct+logdir_p2):
            os.mkdir(logdir_p1+self.debugProduct+logdir_p2)

        debugFileName = logdir_p1+self.debugProduct+logdir_p2+self.debugProduct+""+name+"_"+datetime.now().strftime('%H_%M_%S')+".txt"
        file = open(debugFileName, "w")
        for item in buffer_to_write:
            file.write("%s\n" % item)
        file.close()
    # END

'''
    #===========================================================================
    # triggerOnLimit
    #===========================================================================
    def triggerOnLimit(self, buffer_name ):
        #
        self._logline("triggerOnLimit")
        
        self.session.write( ':MEASure:DIGitize:CURRent? "%s"' % ( buffer_name ) )
        self.session.write( ':DIG:CURR:ATR:MODE EDGE')
        self.session.write( ':DIG:CURR:ATR:EDGE:LEV 30e-3')
        self.session.write( ':TRIGger:LOAD "LoopUntilEvent", ATRigger, 0, ENTer, 0, "%s"' % ( buffer_name ) )
        #self.session.write( ':DISPlay:SCReen GRAPh')
        input('Press enter to exit...')
        self.session.write( ':INIT')
'''

            
#===============================================================================
#===============================================================================

if __name__ == "__main__":

    buff_name = 'freakbuffer'
    seconds = 5
    expectedCurrent = 1e-2
    
    Amp = AmpMeter( 'GPIB0::18::INSTR', 'Better_M13', False )
    print(Amp.doCurrentMeasurement ( buff_name, seconds, expectedCurrent  ))
           
    #input('Press enter to exit...')
    Amp.closeConnection()

