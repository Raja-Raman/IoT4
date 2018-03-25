# python -m serial.tools.list_ports
# python -m serial.tools.miniterm  COM1

# read a data string from Arduino gateway
# Use it with OccupancyGateway2.ino
# A packet from Aruduino must end in new line.
# use the delimiters '[' and ']' on the two ends of the line.  
# discard partially received lines.
# packet structure: [G9,N9,S9,T99,H999]\n
# gateway id, sensor node id, occupied status, temperature and humidity

from datetime import datetime
import serial
import time
import sys

#--------------------------------------------------------------------------------
def open_log():
    global log_file
    close_log()
    fname = datetime.now().strftime("Log_%Y-%m-%d_%H-%M-%S.txt");
    print ('Log file: '+ fname)
    log_file = open(fname,  "wt+")
#--------------------------------------------------------------------------------
    
def close_log():
    global log_file
    if (log_file is None):
        return
    print ("\n\nPacket counts: ")
    print (packet_count)
    str_count = ', '.join([str(x) for x in packet_count])  # list comprehension
    #str_count = ', '.join(str(x) for x in list)    # generator expression
    log_file.write("Packet counts: ")
    log_file.write(str_count)     
    log_file.flush();
    log_file.close();
    log_file = None
#--------------------------------------------------------------------------------

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
    
log_file = None
open_log()
    
MAX_RECORDS = 5000
dstring = None
data = []
gateway = 0
node = 0
occupied = 0
temperature = 0
humidity = 0
packet_count = [0,0,0,0,0]
record_count = 0;

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
        # Note: this is applicable for Python 3; But Python 2 continues to be without the 'b  
        if (dstring[2] != '[' or dstring[-2] != ']'):
            print ('OK')
            continue
        dstring = dstring[3:-2]  # remove delimiters etc
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
        strin = ','.join((str(gateway), str(node), str(status), 
                         str(temperature), str(humidity)))
        print (strin)
        ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S    ")
        log_file.write(ts)
        log_file.write(strin +'\n')
        packet_count[node] = packet_count[node]+1
        record_count = record_count+1
        if (record_count >= MAX_RECORDS):
            open_log()
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
        print(e)
        #break
      
ser.close()
close_log()
print ('Bye!')
time.sleep(1)

	  
  
     