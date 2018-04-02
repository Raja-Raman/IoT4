# Load the dictionary back from the pickle file.
import pickle

kairos_credentials = pickle.load(open( "kairos.p", "rb" ))
print kairos_credentials
print kairos_credentials['app_id']
print kairos_credentials['app_key']
print kairos_credentials['gallery']