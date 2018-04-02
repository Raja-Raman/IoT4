import pyttsx
from gtts import gTTS
import os
import platform
import time

class TTS :
    ENGINE_PYTTS = 1
    ENGINE_GOOGLE = 2
    
    def __init__(self, ttsEngine=ENGINE_PYTTS):
        self.engine_type = ttsEngine
        if (self.engine_type == self.ENGINE_PYTTS):
            self.engine = pyttsx.init()
        self.os = platform.system().lower()
        print 'Platform OS: ', self.os    
        self.audioFolder = "audio/"
        if (not os.path.isdir(self.audioFolder)):
            os.mkdir(self.audioFolder)
    
    def speak (self, textInput, cacheFileName=None):
        if cacheFileName is None:
            cacheFileName=self.audioFolder +"ttstemp.mp3"
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

#-----------------------------------------------------------------------------------------

tt = TTS(TTS.ENGINE_GOOGLE)   # TTS.ENGINE_PYTTS  
tt.speak("Hello, Rajaraman. Very Good morning to you")
time.sleep(3)
tt.speak("How are you today ?")


