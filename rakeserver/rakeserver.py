import socket
import sys
import os
import random
import uuid
import shutil

sys.path.append('../common') # Allows imports from ../common

from packet import *
import execution

def main():
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
    else:
        port = 12345 # Default port if no arguments are provided

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    host = "0.0.0.0"
    s.bind((host,port))

    # Move into temporary directory
    if not os.path.exists("tmp"): 
        os.mkdir("tmp")
    os.chdir("tmp")

    print(f"Listening on port {port}...")
    s.listen(5)
    
    while True:
        
        client,clientaddr = s.accept();
        
        try:
            pid = os.fork()

        except OSError:
            print("fork() failed", file=sys.stderr)
            client.close()
            s.close()
            exit(1)
    
        if pid == 0: # Child process
            print(f"Server: Connected to {clientaddr}!")
            done = False
            while not done:
                done = handle_request(client)
            
            print(f"Closing Connection to {clientaddr}")

            client.close()
            exit(0)
                
        else: # Parent process
            client.close()
            continue; # wait for next connection


# Receives the next packet and determines what to do with it.
# Returns true if communication with the client is over, and false otherwise
def handle_request(client):
    # Receive packet
    msg, cmdtype, length = receive_packet(client)
    
    # Handle request
    if(cmdtype == CMDTYPE.QUOTE_REQUEST.value):
        command = msg 
        
        print(f"Generating quote for command: {command}")
        
        quote = generate_quote(command)
        
        send_packet(client,quote,CMDTYPE.QUOTE_REPLY.value)
        
    
    if(cmdtype == CMDTYPE.SEND_FILE.value): # Required files for the next action
        nfiles = msg
        print(f"About to receive {nfiles} file(s)")

        create_unique_folder()

        # Receive required files
        for i in range(int(nfiles)):
            filename, cmdtype, length = receive_packet(client)
            receive_file(client,filename)
        
        reply = nfiles + " files received"
        send_packet(client,reply,CMDTYPE.SEND_FILE.value) # Send acknowledgement so execution can begin
        return False # More to be received

    if(cmdtype == CMDTYPE.EXECUTE.value):
        # Create a unique folder for commands with no required files
        if in_folder("tmp"):
            create_unique_folder()
        
        action = msg
        returncode,stdout,stderr,fileproduced = execution.run_action(action)
        
        send_packet(client,stdout,CMDTYPE.RETURN_STDOUT.value)
        send_packet(client,stderr,CMDTYPE.RETURN_STDERR.value)
        send_packet(client,returncode,CMDTYPE.RETURN_STATUS.value)
        
        if fileproduced != None:
            print(f"Returning file: {fileproduced}")
            send_file(client,fileproduced)
        

        send_packet(client, "End", CMDTYPE.END.value) # Signal end of communication
        
        # Remove the temporary directory created for the action
        temp_dir = os.getcwd()
        os.chdir("..")
        print(f"Removing {temp_dir}...")
        shutil.rmtree(temp_dir)

    return True # Close the connection    
        
        

# Generates a random quote weighted by command length
def generate_quote(command):
    num = random.randint(0,100) + len(command)
    return str(num)

# Create a unique folder to store the files for each action and change into the folder
def create_unique_folder():
    dirname = str(uuid.uuid1())
    os.mkdir(dirname)
    os.chdir(dirname)

# Returns true if the current directory has the specified name, false otherwise
def in_folder(name):
    path = os.getcwd()
    return name == path.split("/")[-1]



    
    
    

main()


