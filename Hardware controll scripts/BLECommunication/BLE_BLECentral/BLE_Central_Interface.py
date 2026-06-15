import socket
import sys
import time
import subprocess
import os
import math
import binascii

from time import gmtime, strftime
from textwrap import wrap
from binascii import hexlify

DEBUG = False

dataPointTypes = { 
    'RAW' : '0',
    'BOOL' : '1',
    'VALUE' : '2',
    'INT' : '2',
    'STRING' : '3',
    'ENUM' : '4',
    'BITMAP' : '5',          # Currently not supported
    'CHAR' : '7',            # Currently not supported
    'UCHAR' : '8',           # Currently not supported
    'SHORT' : '9',           # Currently not supported
    'USHORT' : '10',         # Currently not supported
    'LMT' : '10'
}

#=========================================================================
#  TuyaInterface class
#=========================================================================
class BLE_CentralInterface(object):

    #=========================================================================
    #  __init__
    #=========================================================================
    def __init__(self, isLESC):
        self.bat_path = '"C:\\Projects\\ETP\\trunk\\Software\\LabETP\\03-LabVIEW\\02-Python\\Hardware controll scripts\\BLECommunication\\BLE_BLECentral\\StartBLECommunication.bat"'

        self.BLE_CENTRALADDRES = 'COM8'
        self.BLE_CENTRALADDRES_LESC = 'COM6'

        self.tuyaApplication = 'ConsoleApplication.exe'
        self.LESC_Application = 'ConsoleApplication_LESC.exe'

        # set port for no LESC
        self.port = 11000

        if True == isLESC:
            self.BLE_CENTRALADDRES = self.BLE_CENTRALADDRES_LESC
            self.tuyaApplication = self.LESC_Application
            self.bat_path = self.bat_path.replace('.bat', '_LESC.bat')
            # set port for yes LESC
            self.port = 51236
        
        # Preventive kill the process blocking the port

        # Start the BLELib application
        if False == DEBUG:
            subprocess.Popen(self.bat_path)
            time.sleep(5)

        # Create a TCP/IP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Connect the socket to the port where the server is listening
        self.server_address = ('localhost', self.port)
        print('connecting to {} port {}'.format(*self.server_address))
        self.sock.connect(self.server_address)

        self.BLE_uuids = {}
        self.unProcessedUUID_str = ''


    #=========================================================================
    #  initBLEDevice
    #=========================================================================
    def initBLEDevice(self, tuyaPid, tuyaDid, tuyaAuthKey, shaverMacAddr, isLESC):

        init_data = [ 'command', 'init', 'tuyaPid', tuyaPid, 'tuyaDid', tuyaDid, 'tuyaAuthKey', tuyaAuthKey, 'shaverMacAddr', shaverMacAddr.replace(':', ''), 'authType', 'bond', 'comPort', self.BLE_CENTRALADDRES, 'isLESC', isLESC]
        init_data = self._createSendData( init_data )

        response = self._sendData( init_data )
        return self._reformatResponse (response)


    #=========================================================================
    #  connectDevice
    #=========================================================================
    def connectDevice(self, hasMITM):
        connect_data = [ 'command', 'connect', 'data', '-', 'mitm', hasMITM ]
        connect_data = self._createSendData( connect_data )

        response = self._sendData( connect_data )
        
        if 'OK' == response.decode():
            self.getListOfUuids()

        return self._reformatResponse (response)
    

    #=========================================================================
    #  startServiceDiscovery
    #=========================================================================
    def startServiceDiscovery(self):
        serviceDiscovery_data = [ 'command', 'startServiceDiscovery', 'data', '-' ]
        serviceDiscovery_data = self._createSendData( serviceDiscovery_data )

        response = self._sendData( serviceDiscovery_data )

        if 'OK' == response.decode():
            self.getListOfUuids()
            
        return self._reformatResponse (response)


    #=========================================================================
    #  connectDeviceNoBind (this is only used in the original BLE imoplementation)
    #=========================================================================
    def connectDeviceNoBind(self, deviceType):
        connectNoBind_data = [ 'command', 'connectNoBind', 'data', '-', 'deviceType', deviceType ]
        connectNoBind_data = self._createSendData( connectNoBind_data )

        response = self._sendData( connectNoBind_data )

        if 'OK' == response.decode():
            self.getListOfUuids()

        return self._reformatResponse (response)


    #=========================================================================
    #  Bind
    #=========================================================================
    def Bind(self):
        bind_data = [ 'command', 'bind', 'data', '-' ]
        bind_data = self._createSendData( bind_data )

        response = self._sendData( bind_data )
        return self._reformatResponse (response)


    #=========================================================================
    #  disconnectTuyaDevice
    #=========================================================================
    def disconnectDevice(self):
        disconnect_data = [ 'command', 'disconnect', 'data', '-' ]
        disconnect_data = self._createSendData( disconnect_data )

        response = self._sendData( disconnect_data )
        return self._reformatResponse (response)

    
    #=========================================================================
    #  unBind
    #=========================================================================
    def unBind(self):
        unbind_data = [ 'command', 'unbind', 'data', '-' ]
        unbind_data = self._createSendData( unbind_data )

        response = self._sendData( unbind_data )
        return self._reformatResponse (response)

    
    #=========================================================================
    #  removeBonds
    #=========================================================================
    def removeBonds(self):
        unbond_data = [ 'command', 'removebonds', 'data', '-' ]
        unbond_data = self._createSendData( unbond_data )

        response = self._sendData( unbond_data )
        return self._reformatResponse (response)


    #=========================================================================
    #  getTuyaDataPoint
    #=========================================================================
    def getTuyaDataPoint(self, dataPointID):
        id_hex = hex(int(dataPointID.split("|")[0])).replace('0x', '').upper()

        if 1 == len(id_hex) :
            id_hex = '0'+id_hex
        
        get_dataPoint_data = [ 'command', 'getdatapoint', 'datapoint', id_hex, 'typeRWN', 'R' ]
        get_dataPoint_data = self._createSendData( get_dataPoint_data )

        response = ''
        tryCounter = 0
        while 'retry' is not response and tryCounter < 2:
            response = self._sendData( get_dataPoint_data )  
            tryCounter += 1

            if 'retry' == response.decode():
                time.sleep(2)
            else:
                break

        return self._reformatResponse (response)

    
    #=========================================================================
    #  getTuyaDataPoint
    #=========================================================================
    def readUUID(self, UUID):

        formattedUUID = UUID.replace('-', '').upper()
        
        if formattedUUID in self.BLE_uuids :
            get_uuid_data = [ 'command', 'readUUID', 'uuid', formattedUUID ]
            get_uuid_data = self._createSendData( get_uuid_data )

            response = ''
            tryCounter = 0
            while 'retry' is not response and tryCounter < 2:
                response = self._sendData( get_uuid_data )  
                tryCounter += 1

                if 'retry' == response.decode():
                    time.sleep(2)
                else:
                    break
            
            return self._reformatResponse (response)
        else:
            return "Unknown UUID"
        
    
    #=========================================================================
    #  writeUUID
    #=========================================================================
    def writeUUID(self, UUID, dataToSend):

        formattedUUID = UUID.replace('-', '').upper()
        
        if formattedUUID in self.BLE_uuids :

            write_uuid_data = [ 'command', 'writeUUID', 'uuid', formattedUUID, 'data', dataToSend ]
            write_uuid_data = self._createSendData( write_uuid_data )

            response = self._sendData( write_uuid_data )
            return self._reformatResponse (response)
        else:
            return "Unknown UUID"
    

    #=========================================================================
    #  enableNotifications
    #=========================================================================
    def enableNotifications(self, UUID, doEnable):
        
        formattedUUID = UUID.replace('-', '').upper()
        
        if formattedUUID in self.BLE_uuids :

            write_notification_data = [ 'command', 'enablenotifications', 'uuid', UUID.replace('-', ''), 'data', doEnable ]
            write_notification_data = self._createSendData( write_notification_data )

            response = self._sendData( write_notification_data )
            return self._reformatResponse (response)
        else:
            return "Unknown UUID"


    #=========================================================================
    #  listenForNotifications
    #=========================================================================
    def listenForNotifications ( self, listenTime_sec, dataPointID ):
        listenTime = int(listenTime_sec)
        id_hex = ""

        if( "" != dataPointID):
            id_hex = hex(int(dataPointID.split("|")[0])).replace('0x', '').upper()
            if 1 == len(id_hex) :
                id_hex = '0'+id_hex

        notification_data = [ 'command', 'listenfornotifications', 'datapoint', id_hex, 'listentime', str(listenTime) ]
        notification_data = self._createSendData( notification_data )
        self.sock.sendall( notification_data.encode() )
        currentTimeOut = self.sock.gettimeout()

        notification_data_response = ''
        try:
            # Look for the response
            amount_expected = 10000            
            self.sock.settimeout( (listenTime*1.2 ))
            notification_data_response = self.sock.recv(amount_expected)
        except:
            pass 

        self.sock.settimeout( currentTimeOut )

        notification_data_response = notification_data_response.decode()

        if notification_data_response.startswith('C'):
            notification_data_returndata = notification_data_response[1:]

        else:
            notification_data_response = notification_data_response.split('*')

            notification_data_returndata = ""
            for item in notification_data_response:
                try:
                    notification_data_returndata += item.split('|')[4] + '|'
                except:
                    pass

        return notification_data_returndata 


    #=========================================================================
    #  setTuyaDataPoint
    #=========================================================================
    def setTuyaDataPoint(self, dataPointID, dataToSend):
        if 'NONE' != dataPointID.split("|")[1]:
            id_hex = hex(int(dataPointID.split("|")[0])).replace('0x', '').upper()

            if 1 == len(id_hex) :
                id_hex = '0'+id_hex

            set_dataPoint_data = [ 'command', 'setdatapoint', 'datapoint', id_hex, 'data', dataToSend, 'type', dataPointTypes.get( dataPointID.split("|")[1] ) ]
            set_dataPoint_data = self._createSendData( set_dataPoint_data )

            response = self._sendData( set_dataPoint_data )
            return self._reformatResponse (response)
        else:
            return "Unknown datapoint ID"


    #=========================================================================
    #  clearDataPoint
    #=========================================================================
    def clearDataPoint(self, dataPointID):
        id_hex = hex(int(dataPointID.split("|")[0])).replace('0x', '').upper()
        if 1 == len(id_hex) :
                id_hex = '0'+id_hex

        clear_dataPoint_data = [ 'command', 'clearDP', 'datapoint', id_hex ]
        clear_dataPoint_data = self._createSendData( clear_dataPoint_data )

        response = self._sendData( clear_dataPoint_data )
        return self._reformatResponse (response)


    #=========================================================================
    #  clearDataPoint
    #=========================================================================
    def getDataPointList(self):
        getDataPointList_data = [ 'command', 'getDPList' ]
        getDataPointList_data = self._createSendData( getDataPointList_data )

        return self._sendData( getDataPointList_data ).decode().replace('*', "")[:-1]


    #=========================================================================
    #  otauTuyaDevice
    #=========================================================================
    def otauDevice(self, product, file, o_type):
        otau_data_upload = [ 'command', 'otauupload', 'product', product, 'file', file, 'waittime_min', '5', 'type', o_type]
        otau_data_upload = self._createSendData( otau_data_upload )

        response = self._sendData( otau_data_upload )
        return self._reformatResponse (response)
        

    #=========================================================================
    #  otauTuyaUpgradeDevice
    #=========================================================================
    def otauUpgradeDevice(self):
        otau_data_upgrade = [ 'command', 'otauupgrade']
        otau_data_upgrade = self._createSendData( otau_data_upgrade )

        response = self._sendData( otau_data_upgrade )
        return self._reformatResponse (response)
    

    #=========================================================================
    #  getDiagnosticsOverBLE
    #=========================================================================  
    def getDiagnosticsOverBLE(self, DIAG_MAX_SIZE_UUID, DIAG_COUNTER_UUID, DIAG_DATA_STREAM_UUID, savePath, isTuya ):
        CHUNCK_SIZE = 20

        numBytes = 0

        if True == isTuya:
            # Tuya device
            numBytes = int(self.getTuyaDataPoint(DIAG_MAX_SIZE_UUID.split('|')[0]),16)
        else:
            # Condor device
            data = self.readUUID(DIAG_MAX_SIZE_UUID)
            if "retry" == data:
                return "Could not read out Diag with these UUID after all the retry's"
            numBytes = int(self.reverseData(self.readUUID(DIAG_MAX_SIZE_UUID)),16)
        
        num_packets = math.ceil((numBytes/CHUNCK_SIZE))

        restBytes = numBytes
        diagData = ''
        for pacNum in range(num_packets):
            packNum_hex = "{0:#0{1}x}".format(pacNum,10).split('x')[1]

            if True == isTuya:
                # Tuya device
                self.setTuyaDataPoint(DIAG_COUNTER_UUID, packNum_hex)
            else:
                # Condor device
                self.writeUUID(DIAG_COUNTER_UUID, self.reverseData(packNum_hex))

            time.sleep(3)

            if restBytes > CHUNCK_SIZE:
                if True == isTuya:
                    # Tuya device
                    diagData += self.getTuyaDataPoint(DIAG_DATA_STREAM_UUID.split('|')[0])[:(CHUNCK_SIZE*2)]
                else:
                    # Condor device
                    diagData += self.readUUID(DIAG_DATA_STREAM_UUID)[:(CHUNCK_SIZE*2)]
            else:                
                if True == isTuya:
                    # Tuya device                    
                    diagData += self.  getTuyaDataPoint(DIAG_DATA_STREAM_UUID.split('|')[0])[:(restBytes*2)]
                else:
                    # Condor device
                    diagData += self.readUUID(DIAG_DATA_STREAM_UUID)[:(restBytes*2)]

            
            restBytes -= CHUNCK_SIZE
            print('Number of bytes remaining: '+str(restBytes))

            
        
        timeStamp = strftime("%d_%m_%Y_%H_%M_%S", gmtime())
        f = open(savePath+"BLEdiagData_"+timeStamp+".bin", "wb")
        f.write(binascii.unhexlify(diagData))
        f.close()

        return diagData.upper()


    #=========================================================================
    #  getListOfUuids
    #=========================================================================
    def getListOfUuids(self):
        uuid_list_data = [ 'command', 'getuuidlist']
        uuid_list_data = self._createSendData( uuid_list_data )

        response = self._sendData( uuid_list_data ).decode()
        self.unProcessedUUID_str = response

        if 'NOK' not in response:
            self.BLE_uuids = {}
            unformatted_uuids = response.split('|')
            for uuid in unformatted_uuids:
                if ('' != uuid) and (';' in uuid):
                    self.BLE_uuids[uuid.split(';')[0].upper()] = uuid.split(';')[1].upper()
        
        return response


    #=========================================================================
    #  getUnProcessedUUID_str
    #=========================================================================
    def getUnProcessedUUID_str(self):
        if '' == self.unProcessedUUID_str:
            return 'empty'
        else:
            return self.unProcessedUUID_str[:-1]


    #=========================================================================
    #  reverseData
    #========================================================================= 
    def reverseData ( self, dataToReverse ):
        reversedData = ''

        if '' is not dataToReverse:
            listToReverse = wrap(dataToReverse, 2)
            listToReverse.reverse()
            reversedData = ''.join(listToReverse)

        return reversedData


    #=========================================================================
    #  closeConnection
    #=========================================================================
    def closeConnection(self):
        print('closing socket')

        exit_data = [ 'command', 'DIE', 'data', 'STOP' ]
        exit_data = self._createSendData( exit_data )

        self._sendData( exit_data )
        self.sock.close()

        # Give the application time to die
        time.sleep(5)

        # Might it happen the application does not close..... now it will
        if True == self._process_exists( self.tuyaApplication ):
            os.system('taskkill /f /im '+self.tuyaApplication)


    #=========================================================================
    #  sendData
    #=========================================================================
    def _sendData(self, dataToSend):
        data = ''
        try:
            # Send data
            #dataToSend = json.dumps(dataToSend)
            print (dataToSend)
            self.sock.sendall( dataToSend.encode() )

            # Look for the response
            amount_expected = 5000

            data = self.sock.recv(amount_expected)
            print('received {!r}'.format(data))

        except:
            pass

        return data


    #=========================================================================
    #  _createSendData
    #=========================================================================
    def _createSendData(self, listToProcess):
        returnString = ""

        for item in listToProcess:
            returnString += item+"|"

        returnString += '*'

        return returnString


    #=========================================================================
    #  _createSendData
    #=========================================================================
    def _reformatResponse(self, response):
        return ( response.decode().split('|') )[-1].replace('*', "")
        

    #=========================================================================
    #  _process_exists
    #=========================================================================
    def _process_exists(self, process_name):
        call = 'TASKLIST', '/FI', 'imagename eq %s' % process_name
        # use buildin check_output right away
        output = subprocess.check_output(call).decode()
        # check in last line for process name
        last_line = output.strip().split('\r\n')[-1]
        # because Fail message could be translated
        return last_line.lower().startswith(process_name.lower())


#=========================================================================
#=========================================================================

if __name__ == "__main__":

    #DEBUG = True

    tuyaPid = 'iabz3srw'
    tuyaDid = 'tuya1d9432c679ed'
    tuyaAuthKey = 'grgiU27DSFYueQO0kTHVEkOZPt6wsruY'
    

    upgFile = 'Apollo_3000_048_81631_Build_1539.upg'
    product = 'S9000_Apollo'

    DIAG_MAX_SIZE = "8D560602-3CB9-4387-A7E8-B79D826A7025"
    DIAG_COUNTER = "8D560603-3CB9-4387-A7E8-B79D826A7025"
    DIAG_DATA_STREAM = "8D560604-3CB9-4387-A7E8-B79D826A7025"

    BLEVersion = "8D5601FE-3CB9-4387-A7E8-B79D826A7025"
    Soc = "2A19"
    isLESC = "true"

    #shaverMacAddr = 'FB:7B:C2:2D:BE:CE'
    shaverMacAddr = 'DB:D3:97:95:E3:4F'
   
    
    for x in range(10):
        BCI = BLE_CentralInterface(isLESC=True)
        BCI.initBLEDevice( tuyaPid, tuyaDid, tuyaAuthKey, shaverMacAddr, isLESC )
        time.sleep(5)
        BCI.removeBonds()
        time.sleep(2)
        BCI.closeConnection()
        pass


    #print(BCI.connectDevice())
    #print(BCI.startServiceDiscovery())

    #print(BCI.readUUID(Soc))

    #print(BCI.getUnProcessedUUID_str())
    
    

    #BCI.getDiagnosticsOverBLE(DIAG_MAX_SIZE, DIAG_COUNTER, DIAG_DATA_STREAM,'C:\\Temp\\',False)

    #BCI.otauDevice('S9000_Apollo', 'Apollo_3000_048_81631_Build_1524.upg','chunck')
    #print(BCI.connectDevice())
    #BCI.otauDevice('S9000_Apollo', 'Apollo_3000_048_81631_Build_1539.upg','default')

    #BCI.otauUpgradeDevice()


    #reset dev manually 0x46

    #BCI.connectDevice()



    
    #BCI.closeConnection()
