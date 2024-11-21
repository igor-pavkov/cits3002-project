import os
import subprocess
import glob

# Runs the specified command in a subprocess
def run_action(action):
    
    action = action.split()
    
    files_before = file_list = glob.glob("*") # List of every file in temp directory
    
    status = subprocess.run(action,capture_output=True, text=True) # text=True for plaintext
    
    files_after = file_list = glob.glob("*") # List of every file in temp directory
    
    if files_before == files_after: # No file produced
        fileproduced = None
    else:
        fileproduced = max(file_list, key=os.path.getctime) # latest file
    
    
    returncode = str(status.returncode)
    stdout = status.stdout
    stderr = status.stderr
    
    return returncode,stdout,stderr,fileproduced



    
