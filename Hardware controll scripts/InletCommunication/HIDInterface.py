import time
import pywinusb.hid as hid


# ******************************************************************
#
#   This class is for communication with the inlet communication
#   dongle via its HID interface. With this interface special
#   functions of the dongle can be accessed, like charging via the
#   dongle and changing the speed of the communication.
#
# ******************************************************************

class HID_Interface( object ):

    # Commands as defined by Renger Ypenburg
    DICT_commands = {
        'Blink':            ( 1, 0xA0 ),
        'ChargeEnable':     ( 1, 0x33, 1 ),
        'ChargeDisable':    ( 1, 0x33, 0 ),
        'RandomPulses':     ( 1, 0x20 ),
        'StopComm':         ( 1, 0x21 ),
        'SetSpeed':         ( 1, 0x10 ),
        'AutoDetect':       ( 1, 0x32 ),
        'AutoDetectOff':    ( 1, 0x00 ),
        'Reset2Default':    ( 1, 0x02 ),
        'Reset2Bootloader': ( 1, 0xFF ),
        'CommOld':          ( 1, 0x22, 0 ),
        'CommUART':         ( 1, 0x22, 1 )
    }


    # ******************************************************************
    #
    #   Init function for creating an HID_Interface class instance
    #
    # ******************************************************************
    def __init__( self, devicename, isteststand = False ):
        self.devicename = devicename

        # This is used to make sure some functions cannot be accessed from TestStand
        self.IsTestStand = isteststand

        # Variables for retreiving data from the dongle
        self.shaverconnected = False
        self.chargerconnected = False
        self.inlet_voltage = 0


    # ******************************************************************
    #
    #   Callback function for reading data from the dongle
    #       Data that can be read is:
    #       - Battery voltage
    #       - Charger connected (actually >= 10V measured at inlet of dongle)
    #       - Shaver connected to dongle
    #
    # ******************************************************************
    def read_data( self, data ):
        if data[1] == 1:
            h_voltage = data[2] << 8
            l_voltage = data[3]
            self.inlet_voltage = h_voltage + l_voltage
            print( self.inlet_voltage )

        elif data[1] == 3:
            if data[3] & 0x01 > 0:
                self.chargerconnected = True
            else:
                self.chargerconnected = False

            if data[3] & 0x02 > 0:
                self.shaverconnected = True
            else:
                self.shaverconnected = False

            print( self.chargerconnected, self.shaverconnected )


    # ******************************************************************
    #
    #   Open the inlet communication dongle
    #       Data that can be read is:
    #       - Battery voltage
    #       - Charger connected (actually >= 10V measured at inlet of dongle)
    #       - Shaver connected to dongle
    #
    # ******************************************************************
    def open_Philips_dongle( self ):
        # Data set to store the number of bytes for each output communication channel
        self.output_reportsize = {}

        # Determine the vendor_id and product_id of the ShaverAnalyser
        all_hids = hid.find_all_hid_devices()

        # Find the dongle in the list of HID devices
        for hid_device in all_hids:
            if self.devicename in hid_device.product_name:
                self.device=hid_device

        # Try to open the dongle and store the length of each output report
        try:
            # Open the ShaverAnalyzer
            self.device.open()

            # Set the data handler for reading data from the dongle via the HID interface
            self.device.set_raw_data_handler( self.read_data )

            # Find the input reports
            self.output_reports = self.device.find_output_reports()
            self.output_reportsize[0] = self.output_reports[0]._HidReport__raw_report_size
            self.output_reportsize[1] = self.output_reports[1]._HidReport__raw_report_size

            #default disable autodetect
            bfr = list(self.DICT_commands['AutoDetectOff'])
            self._send_message( 'AutoDetectOff', bfr )

            # If this succeeded return True
            return True

        except:
            return False


    # ******************************************************************
    #
    #   Close the dongle (only if it was opened)
    #   Also: return the dongle to its default settings
    #
    # ******************************************************************
    def close_Philips_dongle( self ):
        if self.device.is_opened():
            self.device.close()


    # ******************************************************************
    # ******************************************************************
    #
    #   General function to send a message to the dongle
    #
    # ******************************************************************
    # ******************************************************************
    def _send_message( self, command, commandbuffer ):
        # Create a buffer of the correct length
        buffer = [0x00] * self.output_reportsize[self.DICT_commands[command][0]]

        # Fill the buffer with the data from the commandbuffer
        cnt = 0
        for x in commandbuffer:
            buffer[cnt] = x
            cnt += 1

        # If there is a valid HID connection set the data and send it
        if self.device.is_opened() == True:
            self.output_reports[0].set_raw_data(buffer)
            self.output_reports[0].send()
            #time.sleep(0.01)


    # ******************************************************************
    #
    #   Blink the led on the dongle for a few times
    #   (For checking the HID communication with the donlge)
    #
    # ******************************************************************
    def blink( self, nr_of_blinks ):
        bfr = self.DICT_commands['Blink']

        for x in range( nr_of_blinks ):
            self._send_message( 'Blink', bfr )
            time.sleep(0.5)


    # ******************************************************************
    #
    #   Start charging
    #   (By default the adapter is used for charging, in this interface
    #   we keep it that way)
    #
    # ******************************************************************
    def charge( self, state ):
        # If state is True: start charging
        if state == True:
            bfr = self.DICT_commands['ChargeEnable']
            self._send_message( 'ChargeEnable', bfr )
        # If state is False: stop charging
        else:
            bfr = self.DICT_commands['ChargeDisable']
            self._send_message( 'ChargeDisable', bfr )

        
    # ******************************************************************
    #
    #   Send random pulses for msec miliseconds
    #
    # ******************************************************************
    def send_random_pulses( self, msec ):
        bfr = list(self.DICT_commands['RandomPulses'])

        bfr.append( int(msec) >> 8 )
        bfr.append( int(msec) & 0xFF )

        self._send_message( 'RandomPulses', bfr )


    # ******************************************************************
    #
    #   Set the communication speed
    #   Speed is set to default * mul / dif
    #
    # ******************************************************************
    def set_speed( self, mul, dif ):
        bfr = list(self.DICT_commands['SetSpeed'])

        bfr.append( mul )
        bfr.append( diff )

        self._send_message( 'SetSpeed', bfr )


    # ******************************************************************
    #
    #   Stop the communication of the next communication attempt 
    #   after x edges
    #   
    # ******************************************************************
    def stop_communication_after_x_edges( self, x ):
        bfr = list(self.DICT_commands['StopComm'])

        bfr.append( int(x) >> 8 )
        bfr.append( int(x) & 0xFF )

        self._send_message( 'StopComm', bfr )


    # ******************************************************************
    #
    #   Activate detection of connection and disconnection of the shaver
    #
    # ******************************************************************
    def set_auto_detect( self, connect_detect, remove_detect ):
        bfr = list(self.DICT_commands['AutoDetect'])

        if True == connect_detect: 
            bfr.append( 0x01 ) 
        else: 
            bfr.append( 0x00 )

        if True == remove_detect: 
            bfr.append( 0x01 ) 
        else: 
            bfr.append( 0x00 )

        self._send_message( 'AutoDetect', bfr )


    # ******************************************************************
    #
    #   Set inlet communication mode
    #    (By default the communication is UART, 
    #    for platform <19 it can be set to Serial)
    #
    # ******************************************************************
    def set_communication_mode( self, mode ):
        if 'UART' in mode:
            bfr = self.DICT_commands['CommUART']
            self._send_message( 'CommUART', bfr )
        else:
            bfr = self.DICT_commands['CommOld']
            self._send_message( 'CommOld', bfr )
               
        
    # ******************************************************************
    #
    #   Reset the dongle to the default settings
    #
    # ******************************************************************
    def reset_to_default( self ):
        bfr = self.DICT_commands['Reset2Default']
        self._send_message( 'Reset2Default', bfr )


    # ******************************************************************
    #
    #   Set the dongle in bootloader mode, to flash the firmware
    #   DO NOT USE THIS FUNCTION FROM TESTSTAND!
    #
    # ******************************************************************
    def reset_to_bootloader( self ):
        if self.IsTestStand == False:
            bfr = self.DICT_commands['Reset2Bootloader']
            self._send_message( 'Reset2Bootloader', bfr )


    # ******************************************************************
    #
    #   GET the state of the shaver connection
    #       True = Shaver is connected
    #       False = Shaver is not connected
    #
    # ******************************************************************
    def GET_shaverconnected( self ):
        return self.shaverconnected


    # ******************************************************************
    #
    #   GET the state of the charger
    #       True = Charger is connected to dongle
    #       False = Charger is not connected to dongle
    #
    # ******************************************************************
    def GET_chargerconnected( self ):
        return self.chargerconnected


    # ******************************************************************
    #
    #   Get the inlet voltage of the dongle in mV
    #
    # ******************************************************************
    def GET_inlet_voltage( self ):
        return self.inlet_voltage


# **********************************************************************
# **********************************************************************
#
#   Mainly for debugging...
#
# **********************************************************************
# **********************************************************************

if __name__ == "__main__":
    HIDevice = HID_Interface( 'ShaverAnalyser' )
#    HIDevice = HID_Interface( 'KitBridge' )

    HID_connect_result = HIDevice.open_Philips_dongle()

    if True == HID_connect_result:
        #HIDevice.set_communication_mode( 'UART' )

        HIDevice.blink(5)
        HIDevice.reset_to_default()
        HIDevice.blink(5)

        #HIDevice.send_random_pulses( 1000 )

    #    HIDevice.set_auto_detect( True, True )
    #    time.sleep(10)
    #   HIDevice.set_auto_detect( False, False )
    #    time.sleep(5)
        
    #    HIDevice.blink( 5 )
    #    HIDevice.reset_to_bootloader()

        HIDevice.close_Philips_dongle()




#    HID_connect_result = HIDevice.open_Philips_dongle()

#    HIDevice.set_auto_detect( True, True )

#    time.sleep(10)

#    HIDevice.set_auto_detect( False, False )

#    HIDevice.close_Philips_dongle()
