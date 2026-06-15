import serial, serial.tools.list_ports
import time
import json
import statistics


class MotorLoadSimulator (object):
    """A class for communicating with the Motor Load Simulator
    """

    def __init__(self, ComPortName):
        self.InstrumentConnect(ComPortName)
        #self.Present = False
        self.Imotor = 0
        
    def close(self):
        if self.Present:
            self.ctrl.close()
        

    #
    #   Get functions

    #   getIDN : return the device identification string
    def getIDN(self): 
        if self.Present:
            self.write('IDN?')
            return self.read()
        else:
            return 'Demo mode'
    
    #   getRPM : return the measured RPM (rpm)
    def getRPM(self):
        if self.Present:
            RPM = int(self.query('RPM?'))
        else:
            RPM = self.datagen.next()
        return RPM

    #   getRPM : return the measured RPM (rpm)
    def getRPM_timed(self, time_sec):
        RPM_array = []
        if self.Present:
            starttime = time.monotonic()
            while time.monotonic() - starttime < time_sec:
                RPM_array.append(int(self.query('RPM?')))
            RPM = round(statistics.mean(RPM_array))
        else:
            RPM = self.datagen.next()
        return RPM


    def getMotorActive(self, lowerLimit):
        if self.getRPM() > lowerLimit:
            return True
        else:
            return False
        
    #   getCurr : get the actual motor load current (A)
    def getCurr(self):
        if self.Present:
            self.write('CURR?')
            return float(self.read())
        else:
            return self.Imotor


    #   getVolt : return the load voltage (V)
    def getVolt(self):
        if self.Present:
            return float(self.query('VOLT?'))
        else:
            return -1

    #   getTemp : return the board temperature (C)
    def getTemp(self):
        if self.Present:
            return float(self.query('TEMP?'))
        else:
            return -1

    #   getmAh : return the dissipated energy (mAh)
    def getmAh(self):
        if self.Present:
            return float(self.query('MAH?'))
        else:
            return -1

    #   getRMot : get the motor resistance (mOhm)
    def getRMot(self):
        if self.Present:
            return float(self.query('RMOT?'))
        else:
            return -1

    #   getKFac : get the motor k-factor (uV/rad/s)
    def getKFac(self):
        if self.Present:
            return float(self.query('KFAC?'))
        else:
            return -1

    #   getFVCO : get the VCO frequency (Hz)
    def getFVCO(self):
        if self.Present:
            return float(self.query('FVCO?'))
        else:
            return -1

    #   getTIME : get the time since the motor voltage was detected and the current setting was higher than zero (s)
    def getTIME(self):
        if self.Present:
            return float(self.query('TIME?'))
        else:
            return -1

    #   getFREQ : get the motor frequency (kHz)
    def getFREQ(self):
        if self.Present:
            return float(self.query('FREQ?'))
        else:
            return -1

    #
    #   Set functions

    #   setMotorParameters
    #       Input is a json string with a k-factor and an r-motor
    #       This function also checks if the values were written correctly by reading the values from the motor dummy and comparing them
    #       If this check fails the device will return False, if it passes it will return True
    def setMotorParameters(self, parametersjson):
        parameters = json.loads(parametersjson)

        kfac_w = int(float(parameters['kfactor'])*1000000)
        rmot_w = int(float(parameters['rmotor'])*1000)

        self.write('KFAC {}'.format(kfac_w))
        reply = self.read()        
        self.write('RMOT {}'.format(rmot_w))
        reply = self.read() 

        #   Check if the motor dummy has stored the values correctly
        returnvalue = {}
        self.write('KFAC?')
        returnvalue['kfac_r'] = self.read()        
        self.write('RMOT?')
        returnvalue['rmot_r'] = self.read() 

        returnvalue['result'] = ( (int(returnvalue['kfac_r']) == kfac_w) and (int(returnvalue['rmot_r']) == rmot_w) )

        return json.dumps(returnvalue)


    #   setCurr : set the motor load current, input current in [A] and convert to mA
    def setCurr(self, current):
        self.Imotor = current # remember the setting
        if self.Present:
            self.write('CURR {}'.format(int(current*1000)))
            reply = self.read()
        else:
            reply = 'NOK'

        return reply


    def increaseCurr(self, stepsize):
        try:
            if self.getMotorActive():
                currcurr = self.getCurr()

                if currcurr <= 3 - stepsize:
                    newcurr = currcurr + stepsize
                    self.setCurr(newcurr)
            return newcurr
        except:
            return 0

    def decreaseCurr(self, stepsize):
        try:
            if self.getMotorActive():
                currcurr = self.getCurr()
                if currcurr >= stepsize:
                    newcurr = currcurr - stepsize
                    self.setCurr(newcurr)
            return newcurr
        except:
            return 0

    #   setCmot : set the motor inertia "capacitance" in mF
    def setCmot(self, Cmotor):
        if self.Present:
            self.write('CMOT {}'.format(int(Cmotor*1000)))
            reply = self.read()
        else:
            reply = 'NOK'

        return reply

    #   enablePLLmode: set the motor dummy in PLL mode (or not)
    def enablePLLmode(self, pllmode=False):
        if True == pllmode:
            command = 'VCO 100'
        else:
            command = 'VCO 0'
            
        if self.Present:
            self.write(command)
            reply = self.read()
        else:
            reply = 'NOK'

        return reply
        


    #
    #   Debug commands

    #   getState
    def getState(self):
        if self.Present:
            self.write('STATE?')
            reply = self.read()
        else:
            reply = 'Not present'

        return reply

            
            
    def InstrumentConnect(self, ComPortName):
        comports = [comport[0] for comport in serial.tools.list_ports.comports()]
        if ComPortName in comports:
            self.ctrl = serial.Serial(ComPortName, 115200, timeout=0.1)
            # print('comport connected')
            time.sleep(2) # wait for startup of the motor load simulator 
            self.write('IDN?')
            response = self.read()
            # print('Response from simulator:', response)
            if 'Motor Load Simulator' in response:
                self.Present = True
            else:
                print('Motor Load Simulator not found on {}'.format(ComPortName))
                self.close()
        else:
            print('comport {} not found!'.format(ComPortName))
                                                                    

    #
    #   Helper functions
    def write(self, command_string):
        """ send the supplied string to the load simulator, followed by a return character.
            encode the string.
        """
        self.ctrl.write('{}\r'.format(command_string).encode())
   
    def read(self):
        return self.ctrl.readline().decode('UTF-8').strip()
    
    def query(self, command_string):
        self.write(command_string)
        return self.read()
        

if __name__ == "__main__":
    #   Initialize the motor dummy on COM29
    MLS = MotorLoadSimulator("COM21")
    connected = MLS.Present

    #   If initialization passed, do the magic
    if connected:
        #   Set the motor parameters (K-factor and Rmotor)
        motpar = {
            'kfactor': '0.003361091',
            'rmotor': '0.711'
        }
        motparjson = json.dumps(motpar)
        parwriteresult = MLS.setMotorParameters(motparjson)
        print(parwriteresult)

        print('Motor active: ' + str(MLS.getMotorActive(1500)))

        #   Hold here to start operation
        print(MLS.getCurr(), MLS.getRPM(), MLS.getVolt())

        #   Set the current to draw from themotor        
        print(MLS.setCurr(0.02))

        print(MLS.increaseCurr(0.01))

        print(MLS.decreaseCurr(0.01))

        l = 0
        
        while MLS.getRPM() > 1000 and l < 2000:
            l += 10
            print(MLS.setCurr(l/1000))
            time.sleep(0.5)

        print('Motor blocked at: ' + str(l) + 'mA')

#        print(MLS.getState())

#        for x in range(10):
#            print(MLS.getCurr(), MLS.getRPM(), MLS.getVolt())

        MLS.close()
