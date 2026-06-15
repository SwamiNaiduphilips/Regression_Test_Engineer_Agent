import nidaqmx
import time
import math
from datetime import datetime
from ni_reference import new_Analysis

'''
import matplotlib.pyplot as plt
import numpy as np
'''

STATE_SWITCH_SAMPLES = 400000
PULSE_SAMPLES = 120
DEBUG_CODE = False
SHOW_GRAPH = False

class digitalSampler:

    #==================================================================================================
    #   __init__()
    #==================================================================================================
    def __init__(self, digitalLine, encoderPulseRate, TestStand ):

        self.encoderPulseRate = encoderPulseRate
        
        if False == DEBUG_CODE:
            if False == TestStand:
                self.port_reference_rpm = 'Dev2/AO2'
                with nidaqmx.Task() as task_ref:
                    task_ref.ao_channels.add_ao_voltage_chan( self.port_reference_rpm )
                    value = 2
                    task_ref.write( value )
            
            # create and start the task
            self.task = nidaqmx.Task()
            self.task.di_channels.add_di_chan( digitalLine )
            self.task.start()
            
            self.oneSampleTime = 0
            self.nrOfsamplesPerSec = self.calculateSamplesPer1sec()

        else:
            self.oneSampleTime = 0.00125
            self.nrOfsamplesPerSec = 800000

        self.completeSampleList = []
        
    #==================================================================================================
    #   getSamplesPer10sec()
    #==================================================================================================
    def calculateSamplesPer1sec( self ):

        # get all the samples
        startTime_ms = time.perf_counter()
        heelVeelSamples = self.task.read(number_of_samples_per_channel=500000) 
        endTime_ms = time.perf_counter()

        runTime = ( endTime_ms-startTime_ms ) * 1000

        self.oneSampleTime = runTime/len(heelVeelSamples)
        samples1Sec = ( 1000/self.oneSampleTime)

        return int(samples1Sec)


    #==================================================================================================
    #   getSamples()
    #==================================================================================================
    def getSamples( self, timeToSample_sec):
        if False == DEBUG_CODE:
            secondPerLoop = 10
            nrOfSamples = self.nrOfsamplesPerSec * timeToSample_sec
            nrLoops = math.ceil( timeToSample_sec/secondPerLoop )
            nrOfLoopSamples = int(nrOfSamples / nrLoops )

            self.completeSampleList.clear()

            # get all the samples
            heelVeelSamples = []
            print ('GO')
            for x in range(nrLoops):
                heelVeelSamples.append( self.task.read(number_of_samples_per_channel=nrOfLoopSamples, timeout=(secondPerLoop*1.1)) )

            for l in heelVeelSamples:
                self.completeSampleList = self.completeSampleList+l
            print ('Done')

            self.close()
        else:
            f = open("sampesList.txt", "r")
            self.completeSampleList = f.readlines()
            f.close()

        writeSamplesToFile ( self.completeSampleList )
        return new_Analysis(self.completeSampleList)

    #==================================================================================================
    #   getSamples()
    #==================================================================================================
    def doTheAnalasis( self ):
        stageList = []

        periodsList = []
        tmp_list = []
        
        firsSample = True
        previous_sample = False

        dateTimeObj = datetime.now()
        print(dateTimeObj.hour, ':', dateTimeObj.minute, ':', dateTimeObj.second)
        #----------------------------------------------------------------------------------------------------------------
        for sample in self.completeSampleList:
            if True == DEBUG_CODE:
                if 'True' in sample:
                    sample = True
                else:
                    sample = False

            if False == firsSample:
                #--------------------------------------------------------------------------------------------------------

                if previous_sample != sample:
                    if ( len(tmp_list) >= PULSE_SAMPLES) and ( sum(tmp_list) == len(tmp_list) ):
                        for x in range(len(tmp_list)):
                            tmp_list[x] = False
                    periodsList.append(tmp_list.copy())
                    tmp_list.clear()

                #--------------------------------------------------------------------------------------------------------
            else:
                firsSample = False
            
            # add the sample to the temp list
            tmp_list.append(sample)

            previous_sample = sample
        
        # also add the last pulse
        periodsList.append(tmp_list.copy())

        # make sure same pulses are in same period
        tmp_periodsList = []
        prev_period = []
        first_period = True
        for period in periodsList:
            if False == first_period:
                try:
                    if 0 == sum(prev_period) and 0 == sum(period):
                        tmp_periodsList[-1] = tmp_periodsList[-1]+period.copy()
                    else:
                        tmp_periodsList.append(period.copy())
                except:
                    pass
            else:
                first_period = False
            
            prev_period = period.copy()
        
        # copy the tmp list back to original
        periodsList.clear()
        periodsList = tmp_periodsList.copy()
        tmp_periodsList.clear()

        print ("All pulses found, start determining states")
        tmp_list.clear()
        addedToTmp = False
        for pulse_samples in periodsList:

            # determine if it is not a stopping pulse
            #if (len(pulse_samples) >= PULSE_SAMPLES) and (sum(pulse_samples) == len(pulse_samples)):
            #    for x in range(len(pulse_samples)):
            #        pulse_samples[x] = False
        
            if len(pulse_samples) < STATE_SWITCH_SAMPLES:
                # this is a single pulse
                tmp_list = tmp_list+pulse_samples
                addedToTmp = True
            else:
                #in this case pulse_samples > STATE_SWITCH_SAMPLES
                if False == addedToTmp:
                    # in this case nothing was added to the tmp_list
                    # so pulse_samples is a complete stage and is added to stageList
                    stageList.append(pulse_samples.copy())
                else:
                    # in this case items are added to the tmp_list an need to be added to
                    # the stageList
                    #showGraph( tmp_list )
                    stageList.append(tmp_list.copy())
                    # this must be reset after adding item to stageList
                    addedToTmp = False
                    # also add the current pulse_sample list to stageList as a stage
                    stageList.append(pulse_samples.copy())
                    #showGraph( pulse_samples )

                
                # all item are added to stageList so tmp_list can be cleared
                tmp_list.clear()
        
        if True == addedToTmp:
            stageList.append(tmp_list.copy())
        #----------------------------------------------------------------------------------------------------------------
        dateTimeObj = datetime.now()
        print(dateTimeObj.hour, ':', dateTimeObj.minute, ':', dateTimeObj.second)

        # do some cleanup
        self.completeSampleList.clear()
        periodsList.clear()
        tmp_list.clear()

        resultList = []
        # check if motor was running or not
        rpm = 0
        duration_sec = 0
        for stage in stageList:
            duration_sec = len(stage)*self.oneSampleTime
            rpm = 0

            # motor is off
            if (sum(stage) == 0) or sum(stage) == len(stage):
                duration_sec = len(stage)*self.oneSampleTime
            else:
            #motor is on
                firstSample = True
                previous_sample = False

                #----------------------------------------------------------------------------------------------------------------
                count_rising_edge = 0
                for sample in stage:
                    if False == firstSample:
                        if previous_sample == False and sample == True:
                            count_rising_edge = count_rising_edge + 1
                    else:
                        firstSample = False
                    
                    previous_sample = sample
                #----------------------------------------------------------------------------------------------------------------
                rpm = (((60*1000)/duration_sec) * count_rising_edge ) / self.encoderPulseRate

            resultList.append( [round(duration_sec), round(rpm)] )

        # remove first item from resultlist
        if len( resultList ) > 0:
            del resultList[0]

        returnString = ''
        for item in resultList:
            try:
                returnString = returnString+str(item[0]) +'|'+ str(item[1])+';'
            except:
                pass
        
        if len( returnString ) > 0:
            return returnString[:-1]
        else:
            return returnString


    #==================================================================================================
    #   getSamples()
    #==================================================================================================
    def close( self ):
        # stop and close the task
        self.task.stop
        self.task.close()

#==================================================================================================
#   showGraph()
#==================================================================================================
'''
def showGraph( listToShow):
    if True == DEBUG_CODE and True == SHOW_GRAPH:
        # X axis parameter:
        xaxis = np.array( listToShow )
        plt.plot(xaxis)
        plt.show()
        pass
'''

#==================================================================================================
#   writeSamplesToFile()
#==================================================================================================
def writeSamplesToFile ( sampleList):
    f = open("samplesssList.txt", "w")
    f.write( '\n'.join(map(str,sampleList) ) )
    f.close()


if __name__ == "__main__":
    digitalLine = 'Dev2/port1/line5' # is PFI5
    encoderPulseRate = 32

    sampler = digitalSampler( digitalLine, encoderPulseRate, False )
    sampler.getSamples( 10 )    

    pass