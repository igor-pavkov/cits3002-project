# Deals with the sending and receiving of data using the developed protocol

import socket
from protocol import MAX_HEADER_SIZE
from protocol import CMDTYPE

# Send a message to a specified socket preceded by the header (cmdtype and length)
def send_packet(sock,msg,cmdtype):
    header = str(cmdtype) + "/" + str(len(msg)) + "/"
    packet = header + msg
    print(f"Sending packet: {packet}")
    sock.send(bytes(packet,"utf-8"))

# Send the filename and contents of the file in two consecutive packets
def send_file(sock,filename):
    file = open(filename, "rb")
    contents = file.read()
    print(f"Sending file... (size: {len(contents)}) bytes")
    
    send_packet(sock,filename,CMDTYPE.RETURN_FILE.value)
    
    fileheader = str(CMDTYPE.RETURN_FILE.value) + "/" + str(len(contents)) + "/"
    
    sock.send(bytes(fileheader,"utf-8"))
    sock.send(contents)


def receive_header(sock):
    in_header = sock.recv(MAX_HEADER_SIZE, socket.MSG_PEEK).decode() # Peek the message to get the header
    cmdtype,length = in_header.split("/")[:2] 
    header_length = len(cmdtype) + len(length) + 2
    message_length = int(length)
    sock.recv(header_length) # Remove header from the buffer
    return int(cmdtype), message_length

def receive_packet(sock):
    cmdtype, length = receive_header(sock)
    msg = sock.recv(length).decode()
    return msg, cmdtype, length

def receive_file(sock, filename):
    cmdtype, filesize = receive_header(sock)
    contents = sock.recv(filesize)

    print(f"File Received (size: {filesize} bytes)")
    f = open(filename,"wb")
    f.write(contents)

