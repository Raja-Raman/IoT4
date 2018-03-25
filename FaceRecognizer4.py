# show faces in realtime streaming video; if the person is close enough, trigger Kairos.

import cv2
import time
import my_imutils
from videostream import VideoStream 
import argparse
import os.path
import requests
import json
import os
import platform
import pickle
#---------------------------------------------------------------------------------

class Kairos :
    NETWORK_ERROR = 'network_error'
    NO_FACE = 'error'
    UNKNOWN_FACE = 'unknown'
    STATUS_OK = 200
    
    def __init__(self, galleryName=None):
        kairos_credentials = pickle.load(open( "kairos.p", "rb" ))
        self.gallery = galleryName
        if (galleryName is None):
            self.gallery = kairos_credentials['gallery']
        self.baseurl = "https://api.kairos.com/recognize"
        self.headers = {
            "content-type":"application/json", 
            "app_id": kairos_credentials['app_id'], 
            "app_key":kairos_credentials['app_key']
            }
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
green = (0, 255, 0)
red = (0, 0, 255)
font = cv2.FONT_HERSHEY_SIMPLEX

gallery = None
kairos = Kairos(gallery)

source = 0  # camera

#Load a cascade file for detecting faces
#xmlfile = '../XML/haarcascades/haarcascade_frontalface_alt.xml'
xmlfile = "C:\\Prasad-IoT\\Code\\Python\\CV1-master\\XML\\haarcascades\\haarcascade_frontalface_alt.xml"
if not os.path.isfile(xmlfile):
   print "Could not find cascade training set"
   raise SystemExit(1)
face_cascade = cv2.CascadeClassifier(xmlfile)

fileName = "detected_face.jpg" 
stream = VideoStream(src=source)
stream.start()
time.sleep(2.0)
frame = stream.read()
if frame is None:
    print "No camera !"
    raise SystemExit(1)   

print "press any key to save file; ESC to quit.."          
while(True): 
    frame = stream.read()
    frame = my_imutils.resize(frame, 640)
    gray = cv2.cvtColor(frame,cv2.COLOR_BGR2GRAY)
    faces = face_cascade.detectMultiScale(gray, 1.1, 5)
    if (len(faces)==0): 
        cv2.imshow("Face", frame)
        if (cv2.waitKey(20) & 0xff)==27: break
        continue
    face_detected = False
    for (x,y,w,h) in faces:
        cv2.rectangle(frame, (x,y),(x+w,y+h),(0,0,255),2)  
        if w > 120:  # 260
            face_detected = True
            print 'Click!' 
            print "width: ",w, "  height: ",h
            cv2.imwrite(fileName, gray[y:y+h, x:x+h])
            print 'Picture saved as: ',fileName
            (name, confidence) = kairos.identify(fileName)
            print "Name: ", name, " confidence: ",confidence
            if confidence < 30:
                # image, text, location, font, scale, color, thickness, line type    
                cv2.putText(frame, "Unidentified",(x,y-10), font, 0.5, red, 2, cv2.LINE_AA)        
            else:
                # image, text, location, font, scale, color, thickness, line type    
                cv2.putText(frame, name,(x,y-10), font, 0.5, green, 2, cv2.LINE_AA)        
    cv2.imshow("Face", frame)
    if (cv2.waitKey(20) & 0xff)==27: break    
    if (face_detected):    
        time.sleep(5)  # to avoid retriggering immediately

stream.stop()
cv2.destroyAllWindows()
time.sleep(2.0)
print "Done !"