import time

try:
    import serial
except:
    print ("")
    print ("Oh, my gosh, didn't you install https://pypi.org/project/pyserial ???")
    print ("")
    sys.exit(0)

#=========================================================================
#  Inlet communication class
#=========================================================================
class InletComm(object):

    #=========================================================================
    #  __init__
    #=========================================================================
    def __init__(self, portNumber):

        try:
            self.serialPort = serial.Serial(port=portNumber,baudrate=38400, timeout=1)
            time.sleep(2) #! Kind of incredible stupid wait...but necessary.
        except Exception as e:
            print (e)

            
    #=========================================================================
    #  closeInletCommm
    #=========================================================================       
    def closeInletCommm(self):
        self.serialPort.close()

        
    #=========================================================================
    #  sendInletCommand
    #=========================================================================
    def sendInletCommand (self, command ):
        if "" != command:
            #send the command
            command_crc = command+self._calculateChecksum(command).upper()
            self.serialPort.write( bytes.fromhex(command_crc))
            return self._readInletResponse()

        
    #=========================================================================
    #  _readInletResponse
    #=========================================================================
    def _readInletResponse(self):
        return self.serialPort.read(10).hex().upper()[:-2]

    
    #=========================================================================
    #  _calculateChecksum
    #=========================================================================
    def _calculateChecksum (self, command ):
        input_list = []

        indx = 0
        for b in range(int(len(command)/2)):
            input_list.append( int("0x"+command[indx]+command[indx+1],16) )
            indx += 2
        
        res = input_list[0]
        
        for n in range(len(input_list)-1):
            res = res^input_list[n+1]

        crc = hex( (~res + (1 << 8)) % (1 << 8) )[2:]
        if len(crc) == 1:
            crc = '0' + crc

        return crc
    

#=========================================================================
#=========================================================================


if __name__ == "__main__":

    communication = InletComm( "COM20" )
    
    '''
    userInput = ""

    while userInput != "exit":
        userInput = input("Enter command : ")
        
        if userInput != "exit":
            response = communication.sendInletCommand( userInput )
            print (response)
    '''
    communication.closeInletCommm()

