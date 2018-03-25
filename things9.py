# read a data string from Arduino gateway, parse and upload to Thingsboard
# Use it with OccupancyGateway2.ino
# A packet from Aruduino must end in new line.
# use the delimiters '[' and ']' on the two ends of the line.  
# discard partially received lines.
# packet structure: [G9,N9,S9,T99,H999]\n
# gateway id, sensor node id, occupied status, temperature and humidity
# Tools:
# python -m serial.tools.list_ports
# python -m serial.tools.miniterm  COM1

import requests
import serial
import json
import time
import sys
  
#-------------------------------------------------------------------------------------
#                                 Serial comm helper
#-------------------------------------------------------------------------------------
def read_serial():
    global gateway,node,occupied,temperature,humidity,packet_count  
    try:
        dstring = ser.readline()
        if (dstring is None or len(dstring) < 10):  # discard
            return (False)
        #print (type(dstring))
        dstring = dstring.strip()  # remove the new line
        dstring = str(dstring)        
        #print (dstring) 
        
        # The string is of the form 'b[G1,N2,S0,T99,H99]\n'   including the single quotes !     
        if (dstring[2] != '[' or dstring[-2] != ']'):
            print ('OK')
            return (False)
        dstring = dstring[3:-2]  # remove delimiters etc
        #print (dstring)
        
        data = dstring.split(',')
        #print (data) 
        if (len(data) < 5):
            print ('Data error')
            return (False)
            
        #print (data[0], data[1], data[2], data[3], data[4])
        gateway     = int(data[0][1:])
        node        = int(data[1][1:])
        status      = int(data[2][1:])
        temperature = int(data[3][1:])
        humidity    = int(data[4][1:])
        print (gateway, node, status, temperature, humidity)
        packet_count[node] = packet_count[node]+1
        
        return (True)
    except serial.serialutil.SerialException:
        return (False)
    except Exception as e:
        print (e)
        return (False)

#-------------------------------------------------------------------------------------
#                                       Globals
#-------------------------------------------------------------------------------------
 
url = 'http://demo.thingsboard.io:8080/api/v1/rciZ3I8gXy55h5EbWxyO/telemetry'  
jheader = {"content-type":"application/json"}
jdata = {"temperature" : 0, "humidity" : 0}
 
ser = None  
port = 'COM9'
 
gateway = 0
node = 0
occupied = 0
temperature = 0
humidity = 0
packet_count = [0,0,0,0,0,0,0,0]

#-------------------------------------------------------------------------------------
#                                       Main
#-------------------------------------------------------------------------------------

if (len(sys.argv) > 1):
    port = sys.argv[1]
print ("Serial port: " +port)

try:
    ser = serial.Serial(port, 9600, timeout=10)
    #ser = serial.Serial('/dev/ttyACM0', 9600, timeout=10)
except serial.serialutil.SerialException:
    print('* Cannot open serial port *')
    sys.exit()
    
print ("Press ^C to exit")
try:
    while (True):
        data = read_serial()
        if (data is None):
            continue
        jdata['temperature'] = data[0]
        jdata['humidity'] = data[1]
        print jdata
        response = requests.post(url, json=jdata, headers=jheader)
        #print ('Response: ', response.status_code)
        if (response.status_code != 200):
            print ("HTTP error.")  
            break
except KeyboardInterrupt: 
    print ("^C pressed.")   
     
ser.close()            
print ("Bye !")
 