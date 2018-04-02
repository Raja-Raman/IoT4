# Save your Kairos credentials into a pickle file.
import pickle

kairos_credentials = { 
    "app_id": "edb4a215", 
    "app_key": "3df74777b90859421ca9947444c6e7ea", 
    "gallery" : "MyGallery5" }

pickle.dump(kairos_credentials, open("kairos.p", "wb" ))

print "Credentials saved as 'kairos.p'"
 