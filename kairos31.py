# Kairos with speech; Prompter class and speak from cache added
# GPIO class added: button with edge detection and LED indicator  

'''
TODO:
switch off preview

read things from config file 
remote config through bluetooth buttons
try wlan0 with your mobile as the wifi hot spot 
move chek_network and self healing code into the Network class
check_network to connect to jsontest.com and get_ip_address
get_local_ip and get_public_ip methods in Network class

Make bluetooth status work !
convert all print to G.trace, trace2 for bluetooth
try ethernet, then wlan and then usb automatically
blue tooth: select camera, exit, reset network, select eth/wlan/usb
For pytts: How to enable audio caching?
'''
#-------------------------------------------------------------------

class G:

    def __init__(self, debug=True, serial=None):
        self.debug = debug
        self.serial = serial
        print self.serial

    def trace (self, msg):
        if self.debug:
 	      print (msg)
   
    def trace2 (self, msg):
        print (msg)
        #if(self.serial is not None): # TODO
	#   self.serial.write('%s\n'.format(msg).encode())

#---------------------------------------------------------

import pyttsx
from gtts import gTTS
import os
import platform

class TTS :
    ENGINE_PYTTS = 1
    ENGINE_GOOGLE = 2
    
    def __init__(self, ttsEngine=ENGINE_PYTTS):
        self.engine_type = ttsEngine
        if (self.engine_type == self.ENGINE_PYTTS):
            self.engine = pyttsx.init()
        self.os = platform.system().lower()
        print 'Platform OS: ', self.os    
        
    def speak (self, textInput, cacheFileName="tts/text2speech.mp3"):
        if (self.engine_type==self.ENGINE_PYTTS): # TODO: pytts does not have a disk file
            self.engine.say(textInput)
            self.engine.runAndWait()
        else:
            voice = gTTS(text=textInput, lang='en')
            voice.save(cacheFileName)
            if self.os =='windows':
                os.system('start ' +cacheFileName)   # Windows
            else:    
                #os.system('mpg321 ' +cacheFileName)  # Linux
                os.system('omxplayer -o local ' +cacheFileName)  # RPi
    
    def speakFromCache (self, cacheFileName):
        if self.os =='windows':
            os.system('start ' +cacheFileName)   # Windows
        else:    
            #os.system('mpg321 ' +cacheFileName)  # Linux
            os.system('omxplayer -o local ' +cacheFileName)  # RPi
           
#---------------------------------------------------------------------------------

from random import randint
import os.path

class Prompter:
    person = {
        "unknown" : ["Hello, new friend ! I think we have never met before... This is Lisa, from service desk.",
            "Welcome to Verizon face identification system.. To begin an exciting employee journey, please register yourself first.",
            "Hello, good morning.. Please download the V Zee bot app, and register yourself for the service."],
        "rajaraman" : "Hello, Raja Raman.. Good morning. Your work station number is W K 2 34. Thank you.",
        "nikhilesh" : ["Hi, dude ! IIT Nikhileysh.",
            "Hi, Nikhileysh. I am told you bought a pair of golden socks."],
        "mythily" : ["Hi, Doctor my thili. Are you still moody melancholy?",
            "Hello, My thili. you have put on some weight recently."],
        "Xamma" : "That is our mother. But Ammaa does not always mean Jaya Lalitha.",
        "vaidy" : "Hello, vye the kal yana soondaram. Your work station number is W K 7 O 8. Thank you.",
        "dinesh" : "Hello, Dhineysh Raaj.. Workstation number W K 4 37 has been reserved for you. Thank you.",
        "richard" : "Good morning, Richard. Please proceed to work station number 5 7 8. Thank you.",
        "anil" : "Good morning, Anil Kumaar. Welcome back to India.. Cabin number CN 2 2 5 has been reserved for you. \
        If you need assistance, please contact the smart office... Have a nice day."                         
        }
        
    status = {
        "take_snap" : "Welcome to Verizon face identification system. I will take a picture of your face. \
	  at the count of three... One... Two... Three..",
        "shutdown" : "Your bot is shutting down..Good bye!",
        "kairos_started" : "Kairos is starting.",
	"recognizer_inited" : "Face identifier created.",
	"net_inited" : "Network created.",
	"try_network" : "trying to connect.",
	"net_success" : "Network connected successfully.",
	"camera_OK" : "My camera is ready..",
	"main_loop" : "Entering main loop",
        "reset_network" : "Resetting the network..",
	"cache_all" : "I am caching all fixed audio snippets. Please wait"
        }
        
    error = {
        "not_identified" : "The person's face is not in my memory",
        "no_face" : "I think there is an error.. There is no human face in the picture",
        "no_gallery" : "Could not find that gallery. Please check the gallery name",
        "net_error" : "Could not connect to the network.",
        "cloud_error" : "I am unable to contact the cloud server.",
        "speech_engine_error" : "My speech engine has stopped working.",
        "speech_not_enabled" : "I am not speech enabled.",
        "audio_file_error" : "The sound clip is not there",
        "bluetooth_error" : "There is a problem with blue tooth connectivity.",
        "bluetooth_not_on" : "Please switch on my blue tooth.",
        "camera_error" : "Attention. My camera is not working!",
        "init_failed" : "Initial setup failed. I am giving up.."    
     }
    
    def __init__(self, speechEnabled=True, ttsEngine=TTS.ENGINE_GOOGLE, audioFolder="."):
        self.speechEnabled = speechEnabled
        if speechEnabled:
            self.tts = TTS(ttsEngine)         
            if not audioFolder.endswith('/'):
                audioFolder = audioFolder+'/'
            self.audio_folder = audioFolder
            self.audio_extn = '.mp3'
            self.scratch_pad_file =  self.audio_folder +'scratch_file' +self.audio_extn
            print 'Audio folder: ' +self.audio_folder
  
    def notifyStatus (self, statusName):
        print self.status[statusName]  
        if not self.speechEnabled: 
            return
        fname = self.audio_folder  +'status_' +statusName +self.audio_extn
        if os.path.isfile(fname): 
            self.tts.speakFromCache(fname)
        else :
            self.tts.speak(self.status[statusName], fname)
      
    def notifyError (self, errorName):
        print self.error[errorName]  
        if not self.speechEnabled: 
            return
        fname = self.audio_folder  +'error_' +errorName +self.audio_extn
        if os.path.isfile(fname): 
            self.tts.speakFromCache(fname)
        else :
            self.tts.speak(self.error[errorName], fname)      
            
    def announceName (self, name, confidence):
        if (confidence==0):
            if (name==FaceRecognizer.UNKNOWN_FACE):
                self.greet('unknown')
            else:
                self.notifyError('no_face')
        else:    
            self.greet(name)
         
    def greet (self, personName, selectRandom=True):
        personName = personName.lower()
        if (personName not in self.person): # TODO: avoid editing this program every time!
            greeting = 'Hello, ' +personName +'... How are you today ?'
            self.tts.speak (greeting, self.scratch_pad_file)
            print greeting
            return
        
        if (isinstance(self.person[personName], str)):  # it is a string, not a list
            fname = self.audio_folder  +'id_' +personName +self.audio_extn
            self.tts.speak (self.person[personName], fname)
            print (self.person[personName])
            return
        if (not selectRandom):
            fname = self.audio_folder  +'id_' +personName +'_0' +self.audio_extn
            self.tts.speak (self.person[personName][0], fname)
            print (self.person[personName][0])
        else:
            r = randint(0, len(self.person[personName])-1)
            fname = self.audio_folder  +'id_' +personName +'_' +str(r) +self.audio_extn
            self.tts.speak (self.person[personName][r], fname)
            print (self.person[personName][r])
            
    def playSound (self, audioFileName): 
        audioFileName = self.audio_folder +audioFileName
        if os.path.isfile(audioFileName): 
            self.tts.speakFromCache(audioFileName)
        else:
            self.notifyError('audio_file_err')

    def cacheAll (self):
	self.notifyStatus("cache_all")
	for stat in self.status.keys():
	    self.notifyStatus(stat)
	for err in self.error.keys():
	    self.notifyError(err)
#---------------------------------------------------------------------------------

import requests
import json

class FaceRecognizer :
    NETWORK_ERROR = 'network_error'
    NO_FACE = 'error'
    UNKNOWN_FACE = 'unknown'
    STATUS_OK = 200
    
    def __init__(self, gallery):
        self.gallery = gallery
        self.baseurl = "https://api.kairos.com/recognize"
        self.headers = {
            "content-type":"application/json", 
            "app_id":"edb4a215", 
            "app_key":"3df74777b90859421ca9947444c6e7ea"}
        print 'Base URL: ', self.baseurl
        #print 'Headers: ', self.headers
        
    # TODO: make this asynchronous    
    def identify (self, filename):
        imgfile = open (filename, "rb")  
        img64 = imgfile.read().encode("base64") 
        imgfile.close()
        body = {"image":img64, "gallery_name":self.gallery}
	try:
            response = requests.post(self.baseurl, json=body, headers=self.headers)
	except:
	    return (self.NETWORK_ERROR, 0)
        print 'Status: ', response.status_code
	if (response.status_code != self.STATUS_OK):
	    return (self.NETWORK_ERROR, 0)
        return (self.process(response.json()))

    def process (self, jobject):
        print jobject
        if ('Errors' in jobject):
            print "ERROR: Could not detect any face in the picture!"
            return (self.NO_FACE, 0)  # do not remove this return statement !
        print 'Face detected'
        res = jobject['images'][0]['transaction']['status']
        if res == 'success':
            name = jobject['images'][0]['transaction']['subject_id']
            confidence = jobject['images'][0]['transaction']['confidence']   
            print 'Person identified: ', name, ' (', confidence, ')'
            return (name, int(round(100*confidence)))
        else:
            print 'Failed to identify the person in the picture'     
            return (self.UNKNOWN_FACE, 0)

    def listGalleries(self):
	listurl = "https://api.kairos.com/gallery/list_all"
	try:
	    response = requests.post(listurl, json={}, headers=self.headers)
	except:
	    return (self.NETWORK_ERROR, 0)
	if (response.status_code != self.STATUS_OK):
	    return (self.NETWORK_ERROR, 0)
        jresponse = response.json()
	print jresponse
        return (tuple(jresponse['gallery_ids']), 1)
#---------------------------------------------------------------------------------

def take_snapshot():
    print 'Click!'
    #prompter.notifyStatus('take_snap')
    prompter.playSound('camera-shutter.mp3')
    fileName = cam.getFrame()
    (name, confidence) = recognizer.identify(fileName)
    prompter.announceName(name, confidence)


#-------------------------------------------------------------------

gallery = 'MyGallery3'
audioFolder = 'tts/audio_cache'
enableSpeech = True # False 
ttsEngine = TTS.ENGINE_GOOGLE # TTS.ENGINE_PYTTS  #

print ('Kairos is starting...')
warnings.filterwarnings("ignore")
#requests.packages.urllib3.disable_warnings() # SSL certificate warning

if (not os.path.isdir(audioFolder)):
    os.mkdir(audioFolder)
try:    
    prompter = Prompter(enableSpeech, ttsEngine, audioFolder)
    #prompter.notifyStatus("kairos_started")
except:
    g.trace2 ('Could not initialize speech module')
    raise SystemExit(1)

#prompter.cacheAll()
#prompter.notifyStatus("")

recognizer = FaceRecognizer(gallery)
#prompter.notifyStatus("recognizer_inited") 

#time.sleep(3)
g.trace2('Bye !')
