# pip insatll redis
# start redis server in a separate terminal, in 'Run as Admin' mode

import redis
from datetime import datetime
from time import sleep
    
r = redis.StrictRedis(host='localhost', port=6379, db=0)
r.set ('gid', 0)

print 'Writing to Redis...' 
for i in range(10):
    # NOTE: there is a colon in front of %Y
    ts = '{:%Y-%m-%d %H:%M:%S}'.format(datetime.now())
    #ev = {'timestamp': ts, 'camera' : 1, 'hotspot' : (i%3)+1 , 'url': 'captured.mp4'}
    # must be double quotes for json parser
    ev = {"timestamp": ts, "camera" : 1, "hotspot" : (i%3)+1 , "url": "captured.mp4"}
    current_index = r.incr('gid')
    ev['id'] = current_index
    print ev
    my_key = 'eventid:'+str(current_index)
    r.hset ('activity', my_key, ev)
    sleep(0.6);
 
print '\n-------------------------------'
print 'Reading from Redis...' 
for i in range(15): 
    print r.hget ('activity', 'eventid:'+str(i))
 
 

