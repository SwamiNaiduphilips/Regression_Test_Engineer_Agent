"""
Basic usage of PPK2 Python API.
The basic ampere mode sequence is:
1. read modifiers
2. set ampere mode
3. read stream of data
"""
import time
from ppk2_api import PPK2_API

class uCurrentMeter(object):

    #=========================================================================
    #  __init__
    #=========================================================================
    def __init__(self):
        self.ppk2_test = None


    #=========================================================================
    #  initialize
    #=========================================================================
    def initialize(self):
        ppk2s_connected = PPK2_API.list_devices()
        initDone = "NOK"

        if(len(ppk2s_connected) == 1):
            ppk2_port = ppk2s_connected[0]
            print(f'Found PPK2 at {ppk2_port}')

            self.ppk2_test = PPK2_API(ppk2_port)
            self.ppk2_test.get_modifiers()
            self.ppk2_test.use_ampere_meter()  # set ampere meter mode

            self.ppk2_test.toggle_DUT_power("ON")  # Enable DUT power

            initDone = "OK"
        else:
            print(f'Too many connected PPK2\'s: {ppk2s_connected}')

        return initDone

    
    #=========================================================================
    #  measureCurrent
    #=========================================================================
    def measureCurrent(self, timeToMeasure_sec):
        runTime = int(round(time.time())) + int(timeToMeasure_sec)
        samples = []
        
        self.ppk2_test.start_measuring()
        startTime = int(round(time.time()))
        
        while ( int(round(time.time())) < runTime ):
            read_data = self.ppk2_test.get_data()
            if read_data != b'':
                samples += self.ppk2_test.get_samples(read_data)
            time.sleep(0.01)
        
        avgCurrent = sum(samples)/len(samples)
        print(f"Average of {len(samples)} samples is: {avgCurrent}uA")

        self.ppk2_test.stop_measuring()

        return (avgCurrent/1000000)
    

    #=========================================================================
    #  close_uCurrentMeter
    #=========================================================================
    def close_uCurrentMeter(self):
        if None != self.ppk2_test:   
            self.ppk2_test.stop_measuring()
            self.ppk2_test.toggle_DUT_power("OFF")  # Disable DUT power


if __name__ == "__main__":
    
    powerMeter = uCurrentMeter()
    powerMeter.initialize()
    
    powerMeter.measureCurrent(5)
    powerMeter.measureCurrent(5)
    powerMeter.measureCurrent(5)

    powerMeter.close_uCurrentMeter()

