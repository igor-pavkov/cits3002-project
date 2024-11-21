// Deals with sending and receiving of data using the developed protocol

#include "rake-c.h"

#include <sys/socket.h>


// Send a message to a specified socket descriptor preceded by the header (cmdtype and length)
int send_packet(int sd, char *msg, int msglen, CMDTYPE type) {
        
    char *buffer = NULL;
    buffer = strdup(msg);
    
    int cmd_send = type; 
    
    char out_header[MAX_HEADER_SIZE + 1] = ""; // Outgoing header
    sprintf(out_header,"%i/%i/", cmd_send,msglen);
    int outheaderlen = strlen(out_header);
    
    printf("Sending packet: %s%s (%i bytes)\n",out_header,buffer,msglen+outheaderlen);
    
    int bytes_sent = 0;
    
    bytes_sent += send(sd,out_header,outheaderlen,0);
    bytes_sent += send(sd,buffer,msglen,0);
    
    free(buffer);
    
    if(bytes_sent < 0) {
        perror("Sending error in client");
        exit(EXIT_FAILURE);
    }

    return bytes_sent;
}

// Send the filename and contents of the file in two consecutive packets
int send_file(int sd, char *filename, int filesize) {
    
    printf("Sending file '%s' (size=%i bytes) to sd %i\n",filename,filesize,sd);
    
    FILE *file;
    
    file = fopen(filename, "r");
    if(file == NULL) {
        perror("Could not open file");
        return -1;
    }
    
    char *buffer;
    buffer = calloc(filesize,sizeof(char));
    CHECK_ALLOC(buffer)
    
    int elements_read; 
    elements_read = fread(buffer,sizeof(char),filesize,file);
    
    if(elements_read < 0) {
        perror("Error reading file");
        return -1;
    }
    
    int bytes_sent = 0;
    
    // Send filename with header
    bytes_sent += send_packet(sd, filename, strlen(filename), CMD_SEND_FILE);
    
    // Send file with header 
    char out_header[MAX_HEADER_SIZE + 1] = ""; // Outgoing header
    sprintf(out_header,"%i/%i/", CMD_SEND_FILE,filesize);
    int outheaderlen = strlen(out_header);
    
    bytes_sent += send(sd,out_header,outheaderlen,0);
    
    bytes_sent += send(sd,buffer,filesize,0);
    
    
    fclose(file);
    free(buffer);
    
    if(bytes_sent < 0) {
        perror("Error sending file");
        return -1;
    }

    return bytes_sent;
}

// Receive an incoming header, update the parameter variables and remove the header from the receive queue
void receive_header(int sd, char *cmdtype, char *msglen) {
    
    char in_header[MAX_HEADER_SIZE + 1] = ""; // Incoming header
    recv(sd,in_header,MAX_HEADER_SIZE,MSG_PEEK); // Peek the incoming header
    
    char *token = strtok(in_header, "/");
    strcpy(cmdtype,token);
    token = strtok(NULL, "/");
    strcpy(msglen,token);

    // Remove the header from the receive queue
    strcpy(in_header,"");
    int headerlen = strlen(cmdtype) + strlen(msglen) + 2; // +2 for '/' dividers
    recv(sd,in_header,headerlen,0);
    
}

// Receive a packet from the specified socket descriptor, updating the passed cmdtype and msglen parameter variables and returning the message string
char *receive_packet(int sd, char *cmdtype, char* msglen) {

    receive_header(sd, cmdtype, msglen);
    
    char *msg;
    msg = calloc(atoi(msglen)+1,sizeof(char)); // +1 for null byte
    CHECK_ALLOC(msg);
    
    recv(sd,msg,atoi(msglen),0);

    return msg;

}

// Receive a file with the specified filename
int receive_file(int sd, char *filename) {
    
    char cmdtype[MAX_CMDTYPE_SIZE + 1] = ""; 
    char msglen[MAX_LENGTH_SIZE + 1] = "";

    receive_header(sd, cmdtype, msglen);

    int filesize = atoi(msglen);
    
    printf("Receiving file %s (size: %i bytes)",filename, filesize);
    
    FILE *file;
    
    file = fopen(filename, "wb");

    char *contents;    
    contents = calloc(filesize,sizeof(char));
    CHECK_ALLOC(contents);
    
    int elements_written = 0;
    
    int bytes_recv = 0;
    bytes_recv = recv(sd, contents, filesize, 0);
    
    if(bytes_recv < 0) {
        perror("Error receiving file");
        return -1;
    }
    
    elements_written = fwrite(contents, sizeof(char), filesize, file);

    if(elements_written < 0) {
        perror("Error reading file");
        return -1;
    }
    
    fclose(file);
    free(contents);
    
    return 0;
}