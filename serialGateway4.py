# python -m serial.tools.list_ports
# python -m serial.tools.miniterm  COM1

# read a data string from Arduino gateway; log in a file
# Use it with OccupancyGateway2.ino
# A packet from Aruduino must end in new line.
# use the delimiters '[' and ']' on the two ends of the line.  
# discard partially received lines.
# packet structure: [G9,N9,S9,T99,H999]\n
# gateway id, sensor node id, occupied status, temperature and humidity

from time import gmtime, strftime
from datetime import datetime
import serial
import time
import sys

port = 'COM4'
if (len(sys.argv) > 1):
    port = sys.argv[1]
print ("Serial port: " +port)

try:
    ser = serial.Serial(port, 9600, timeout=10)
    #ser = serial.Serial('/dev/ttyACM0', 9600, timeout=10)
except serial.serialutil.SerialException:
    print('* Cannot open serial port *')
    sys.exit()
    
dstring = None
data = []
gateway = 0
node = 0
occupied = 0
temperature = 0
humidity = 0
packet_count = [0,0,0,0,0,0,0,0]

print ("Press ^C to exit...")
while 1:
    try:
        dstring = ser.readline()
        if (dstring is None or len(dstring) < 10):  # discard
            continue
        #print (type(dstring))
        dstring = dstring.strip()  # remove the new line
        dstring = str(dstring)        
        #print (dstring) 
        # The string is of the form 'b[G1,N2,S0,T99,H99]\n'   including the single quotes !     
        if (dstring[0] != '[' or dstring[-1] != ']'):
            print ('OK')
            continue
        dstring = dstring[1:-1]  # remove delimiters etc
        #print (dstring)
        data = dstring.split(',')
        #print (data)
        if (len(data) < 5):
            print ('Data error')
            continue
        #print (data[0], data[1], data[2], data[3], data[4])
        gateway     = int(data[0][1:])
        node        = int(data[1][1:])
        status      = int(data[2][1:])
        temperature = int(data[3][1:])
        humidity    = int(data[4][1:])
        #print (gateway, node, status, temperature, humidity)
        ts = strftime("%Y-%m-%d %H:%M:%S", gmtime())
        occufree = "Occupied"
        if (status==0) : occufree = "Free"
        ts = time.time()
        ts = datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')        
        display_str = "Room {0} is {1},\tTemp:{2} C, Hum={3} %\t{4}"
        print (display_str.format (node, occufree, temperature, humidity, ts))
        packet_count[node] = packet_count[node]+1
    #except serial.SerialTimeoutException:
    except serial.serialutil.SerialException:
        print('* Cannot read serial port *')
        time.sleep(10)
        try :
            if (ser is not None):
                ser.close()
            ser = serial.Serial(port, 9600, timeout=10)
        except serial.serialutil.SerialException:
            print('* Cannot open serial port *')
            time.sleep(10)
    except KeyboardInterrupt:
        break
    except Exception as e:
        print (e)
        # break
      
ser.close()
print ("\nPacket counts: ")
print (packet_count)
print ('Bye!')
time.sleep(1)

	  
  
     