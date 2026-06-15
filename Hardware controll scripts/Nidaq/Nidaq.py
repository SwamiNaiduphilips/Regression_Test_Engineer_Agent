import time
import nidaqmx
from nidaqmx.constants import LineGrouping



class Nidaq(object):

    def __init__( self ):
        self.port_capacitor = 'Dev2/port2/line6'
        self.port_charging = 'Dev2/port0/line18'
        self.port_switchC = 'Dev2/port2/line1'
        self.port_switchD = 'Dev2/port2/line4'
        self.port_switchE = 'Dev2/port0/line27'
        self.port_switchF = 'Dev2/port2/line7'
        self.port_switchG = 'Dev2/port1/line7'
        self.port_menuturbo = 'Dev2/port0/line28'        
        self.port_inletcommunication = 'Dev2/port0/line29'
        self.port_startstop = 'Dev2/port0/line30'
        self.port_di = 'Dev2/port0/line0:'

    def close( self ):
        pass

    def enable_capacitor( self, state=False ):
        with nidaqmx.Task() as task:
            task.do_channels.add_do_chan( self.port_capacitor )
            task.write( state )

    def enable_inlet_communication( self, state=False ):
        with nidaqmx.Task() as task:
            task.do_channels.add_do_chan( self.port_inletcommunication )
            task.write( state )

    def enable_charging( self, state=False ):
        with nidaqmx.Task() as task:
            task.do_channels.add_do_chan( self.port_charging )
            task.write( state )

    def short_press_button( self, button, time_ms=100 ):
        with nidaqmx.Task() as task:
            task.do_channels.add_do_chan( button )
            task.write( True )
            time.sleep( time_ms/1000 )
            task.write( False )

    def press_on_off_button( self ):
        self.short_press_button( self.port_startstop )

    def press_turbo_menu_button( self ):
        self.short_press_button( self.port_menuturbo )


    def push_release_on_off_button( self, state=False ):
        with nidaqmx.Task() as task:
            task.do_channels.add_do_chan( self.port_startstop )
            task.write( state )

    def push_release_turbo_button( self, state=False ):
        with nidaqmx.Task() as task:
            task.do_channels.add_do_chan( self.port_menuturbo )
            task.write( state )

    # Toggle switch C
    def push_release_switchC( self, state=False ):
        with nidaqmx.Task() as task:
            task.do_channels.add_do_chan( self.port_switchC )
            task.write( state )

    # Toggle switch D
    def push_release_switchD( self, state=False ):
        with nidaqmx.Task() as task:
            task.do_channels.add_do_chan( self.port_switchD )
            task.write( state )

    # Toggle switch E
    def push_release_switchE( self, state=False ):
        with nidaqmx.Task() as task:
            task.do_channels.add_do_chan( self.port_switchE )
            task.write( state )

    # Toggle switch F
    def push_release_switchF( self, state=False ):
        with nidaqmx.Task() as task:
            task.do_channels.add_do_chan( self.port_switchF )
            task.write( state )

    # Toggle switch G
    def push_release_switchG( self, state=False ):
        with nidaqmx.Task() as task:
            task.do_channels.add_do_chan( self.port_switchG )
            task.write( state )

    def read_input_port( self, nr_of_leds_to_read, nr_of_samples ):
        di_state = 'no_info'
        
        with nidaqmx.Task() as task:
            portstring = self.port_di+str(nr_of_leds_to_read)
            task.di_channels.add_di_chan(portstring, line_grouping=LineGrouping.CHAN_PER_LINE)
            di_state = task.read(number_of_samples_per_channel=nr_of_samples)

        return di_state


    # Press the menu button for a fixed number of secconds
    # Was an idea, does not work!!!
    def press_menu_sec( self, seconds ):    
        press_button( self.port_menuturbo, seconds*1000 )


if __name__ == "__main__":
    ndaq = Nidaq()
