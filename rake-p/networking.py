# Deals with communication between client and rakeservers

import sys
import socket
import select

sys.path.append('../common') # Allows imports from ../common

from packet import *


def connect_rakeserver(actions, hosts, default_port):
    
    for i in range(len(actions)):
        print(f"\n---Sending actionset {i+1}---\n")
        status = send_actionset(actions[i],hosts,default_port)
        if status != 0:
            print(f"\n---{status} failed action(s) in actionset {i+1}\n")
            exit(1)
        else:
            print(f"\n---Actionset {i+1} completed successfully---\n")

# Connect to the specified host and return a new socket object
def new_connection(host, default_port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    if host == "localhost":
        new_host = "localhost"
        new_port = int(default_port)
    else:
        pair = host.split(":") # host,port pair
        new_host = pair[0]
        new_port = int(pair[1])

#     print(f"Trying to connect to host: {new_host}:{new_port}")
    try:
        s.connect((new_host,new_port))
    except:
        print(f"Could not connect to host: {new_host}:{new_port}")
        return -1

    print(f"Connected to {host}!")

    return s



def send_actionset(aset, hosts, default_port):
    # Number of actions left (decremented as actions return)
    actionsleft = len(aset)
    failed = 0
    
    readset = []
    
    ds = []
    
    # For every action, find the socket that returned the best quote and use it for execution
    for i in range(len(aset)):
        if aset[i]["remote"]:
            best = get_lowest_quote(hosts, aset[i]["command"])
            if best == -1: # Couldn't connect to any rakeserver
                ds.append(-1)
            else:
                ds.append(new_connection(hosts[best],default_port))
        else:
            ds.append(new_connection("localhost",default_port))
        
        if ds[i] == -1: # If all remote servers are down for remote action, or local server is down for local action
            print(f"No available host for action {i+1} ({aset[i]['command']})")
            failed += 1 # Count the action as failed
            actionsleft -= 1
            continue # Skip to the next action
        
        send_status = action_send(ds[i],aset[i])
        if send_status != 0:
            failed += 1 # Fail the action if files could not be sent
            actionsleft -= 1
            
    while actionsleft > 0:
        for i in range(len(ds)):
            if ds[i] != -1:
                readset.append(ds[i])

        # Block until any of the socket descriptors have something to read
        nready = select.select(readset,[],[])
        
        if(nready == 0):
            break
        
        for d in range(len(ds)):
            msg, cmdtype, msglen = receive_packet(ds[d])
            exitstat = handle_command(ds[d], msg, cmdtype)
            if exitstat != 0:
                failed += 1
            
            if cmdtype == CMDTYPE.END.value: # Sockets automatically become 0 after server closes
                actionsleft -= 1

    return failed
    
def get_lowest_quote(hosts, msg):
    ds = []
    quotes = []
    
    for i in range(len(hosts)):
        ds.append(new_connection(hosts[i],0)) # Remote, so default port not needed
    
    for d in ds:
        if d != -1:
            send_packet(d,msg,CMDTYPE.QUOTE_REQUEST.value)
            reply = receive_packet(d)[0] # Ignore cmdtype and length
            print(f"Quote Received: {reply}")
            
            quotes.append(int(reply))
        else:
            quotes.append(-1)
    
    if max(quotes) == -1: # No connections
        return -1

    lowest = min(i for i in quotes if i > 0) # Minimum positive quote
    best_ind = quotes.index(lowest)
    return best_ind

def action_send(sock, action):
    nfiles = len(action["requirements"])
    if nfiles > 0: # Send any required files
        send_packet(sock,str(nfiles),CMDTYPE.SEND_FILE.value)
        
        for filename in action["requirements"]:
            send_file(sock, filename)

        receive_packet(sock) # Acknowledgement that files have been received
    
    send_packet(sock,action["command"],CMDTYPE.EXECUTE.value) # Send the command itself
    
    return 0

def handle_command(sock, msg, cmdtype):
    
    if cmdtype == CMDTYPE.RETURN_STATUS.value:
        returnstat = int(msg)
        print(f"return status = {returnstat}")
        if returnstat != 0:
            return returnstat # The action failed

    if cmdtype == CMDTYPE.RETURN_STDOUT.value:
        if len(msg) > 0:
            print(msg) # Print output to stdout

    if cmdtype == CMDTYPE.RETURN_STDERR.value:
        if len(msg) > 0:
            print(msg, file = sys.stderr) # Print output to stderr
            
    if cmdtype == CMDTYPE.RETURN_FILE.value:
        receive_file(sock, msg)
        
    # CMDTYPE.END is handled in send_actionset()
        
    return 0