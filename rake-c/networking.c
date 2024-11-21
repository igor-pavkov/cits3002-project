// Deals with communication between client and rakeservers

#define _XOPEN_SOURCE 600

#include "rake-c.h"

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>


void connect_rakeserver(ALLACTIONS actions, char **hosts, int nhosts, char *default_port) {
    
    // Iterate through action sets
    for(int i = 0; i < actions.nactionsets; i++) {
        printf("\n---Sending actionset %i---\n\n",i+1);
        int status = send_actionset(actions.actionsetarr[i],hosts,nhosts,default_port);        
        if(status != 0) {
            printf("\n---%i failed action(s) in actionset %i---\n\n",status,i+1);
            exit(EXIT_FAILURE);
        }
        else {
            printf("\n---Actionset %i completed successfully---\n\n",i+1);
        }
    }

}

// Adapted from Beej's Guide to Network Programming (Section 5.1)
// Connect to the specified host and return a new socket descriptor
int new_connection(char *host, char *default_port) {

    char *addr = NULL;
    char port[6] = "";
    
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;  

    memset(&hints, 0, sizeof hints); 
    hints.ai_family = AF_UNSPEC;     
    hints.ai_socktype = SOCK_STREAM; 

    
    if(strcmp(host,"localhost") == 0) { // Local host
        addr = strdup("localhost");
        strcpy(port,default_port);
    }
    else { // Remote host
        addr = strdup(host);
        char *colon = strchr(addr,':');
        strcpy(port, (colon+1));
        *colon = '\0';
    }
    
    printf("Trying to connect to host: %s, port: %s\n",addr, port);
    
    status = getaddrinfo(addr, port, &hints, &servinfo);
    
    free(addr);
    
    if(status != 0) {
        perror("getaddrinfo() failed");
        return -1;
    }

    int sd;
    sd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    
    int c;
    c = connect(sd, servinfo->ai_addr, servinfo->ai_addrlen);
    if(c < 0) {
        fprintf(stderr,"Could not connect to host: %s\n",host);
        perror("connect() failed");
        return -1;
    }
    
    freeaddrinfo(servinfo);

    printf("Connected to %s! (sd = %i)\n",host,sd);
    
    return sd;
}

// Send actionset commands to rakeserver(s) and receive the command output.
// Returns the number of actions that were failed
int send_actionset(ACTIONSET *set, char **hosts, int nhosts, char *default_port) {
    
    
    // The number of actions left (decremented as actions return)
    int actionsleft = set->nactions; 
    int exitstat = 0; // The exit status of the current action
    int failed = 0; // The number of actions that returned non-zero exit statuses
    
    fd_set readset;
    FD_ZERO(&readset);
    
    
    // Array containing all of the sockets to send each action to
    int nds = set->nactions;
    int ds[nds];
    
    // For every action, find the socket descriptor that returned the best quote and start executing
    for(int i = 0; i < set->nactions; i++) {
        if(set->actionarr[i]->remote) { 
            int best = get_lowest_quote(hosts, nhosts, set->actionarr[i]->command);
            if(best == -1) { // Couldn't connect to any server
                ds[i] = -1;
            }
            else {
                ds[i] = new_connection(hosts[best], default_port);                
            }
        }
        else {
            ds[i] = new_connection("localhost", default_port);
        }
        if(ds[i] == -1) { // If all remote servers are down for remote action or local server is down for local action
            printf("No available host for action %i (%s)\n",i+1,set->actionarr[i]->command);
            failed++; // Count the action as failed
            actionsleft--;
            continue; // Skip to next action
        }
        int send_status;
        send_status = action_send(ds[i], set->actionarr[i]);
        
        if(send_status != 0) {
            failed++; // Fail the action if files could not be sent (stat() or send() issue)
            actionsleft--;
        }
    }
    

    while(actionsleft > 0) {    
        
       // Populate readset with the still-connected socket descriptors
        int maxd = -1;
                
        for(int d=0; d<nds; d++) {
            if(ds[d] >= 0) {
                FD_SET(ds[d], &readset);
            }
            if(maxd < ds[d]) {
                maxd = ds[d];
            }
        }
        
        if(maxd == -1) {
            break;
        }

        // Block until any of the socket descriptors have something to read
        int nready = select(maxd+1, &readset, NULL, NULL, NULL);
        
        if(nready == 0) {
            break;
        }
        
        // Determine which socket descriptor(s) have something to read/write
        for(int d = 0; d < nds; d++) {
            
            // There is something to read
            if(ds[d] >= 0 && FD_ISSET(ds[d], &readset)) {
                
                char cmdtype[MAX_CMDTYPE_SIZE + 1] = "";
                char msglen[MAX_LENGTH_SIZE + 1] = "";
                char *msg = receive_packet(ds[d],cmdtype,msglen);
                
                exitstat = handle_command(ds[d], msg, cmdtype, msglen);
                if(exitstat != 0) {
                    failed++;
                }
                
                if(atoi(cmdtype) == CMD_END) {
                    shutdown(ds[d], SHUT_RDWR);
                    close(ds[d]);
                    ds[d] = -1;
                    --actionsleft;
                }
                
            }
                
               
            
        }

    }

    return failed; // Failure if one or more actions in the set failed
    
}


// Asks every connection for a quote and returns the position of the host in 'hosts' that returned the lowest quote. 
// Returns -1 if none of the hosts could be contacted
int get_lowest_quote(char **hosts, int nhosts, char *msg) {
    
    int ds[nhosts];
    int quotes[nhosts]; // Initialised to -1
    
    for(int i = 0; i < nhosts; i++) {
        ds[i] = new_connection(hosts[i],""); // Default port not needed - only remote hosts are contacted
        quotes[i] = -1;
    }
    
    
    
    for(int i = 0; i < nhosts; i++) {
        if(ds[i] != -1) { // Only get quotes from contactable hosts
            int msglen = strlen(msg) + 1;
            send_packet(ds[i], msg, msglen, CMD_QUOTE_REQUEST);
            
            char *reply = NULL;
            char cmdtype[MAX_CMDTYPE_SIZE + 1] = "";
            char replylen[MAX_LENGTH_SIZE + 1] = "";
            reply = receive_packet(ds[i], cmdtype, replylen);
            printf("Quote received: %s\n",reply);
            
            quotes[i] = atoi(reply);
        }
    }
    
    // Iterate through quotes[] to find the lowest quote
    int lowest = -1; // i.e., hosts[lowest] returned the lowest quote
    
    for(int i = 0; i < nhosts; i++) {
        if(quotes[i] != -1 && quotes[i] < quotes[lowest]) { // Update if lower quote is found
            lowest = i;
        }
        printf("Closing Connection to host: %i\n",i+1);
        close(ds[i]);
    }
    
    return lowest;
    
}

// Send an individual action as well as its required files - returns 0 on success, 1 on failure
int action_send(int sd, ACTION *action) {
  
    int bytes_sent = 0;
      
    // Send required files (if any)
    if(action->nfiles > 0) {
        char nfiles[16] = ""; 
        sprintf(nfiles,"%i",action->nfiles);
        
        // Inform the server how many files have been sent
        send_packet(sd,nfiles,strlen(nfiles),CMD_SEND_FILE);
     
     // Send all of the required files
        for(int i = 0; i < action->nfiles; i++) {
            char *filename = action->requirements[i];
            struct stat statinfo;
            int status;
            status = stat(filename,&statinfo);
            if(status != 0) {
                fprintf(stderr,"Issue with required file: %s\n",filename);
                perror("stat() failed");
                return -1;
            }
            
            bytes_sent += send_file(sd, filename, statinfo.st_size);
            if(bytes_sent == -1) {
                perror("Error sending file");
                return -1;
            }
            
        }
        
        // Receive acknowledgement that files have been received
        char cmdtype[MAX_CMDTYPE_SIZE + 1];
        char msglen[MAX_LENGTH_SIZE + 1];
        receive_packet(sd, cmdtype, msglen);

    }
    // send the command itself - at this point it is assumed that the server has required files
    bytes_sent += send_packet(sd, action->command, strlen(action->command),CMD_EXECUTE);

    if(bytes_sent <= 0) {
        perror("Issue sending file");
        return -1;
    }

    return 0;
}

// Receive the output of a command and decide what to do with it
int handle_command(int sd, char *msg, char *cmdtype, char *msglen) {
    
    CMDTYPE cmd = atoi(cmdtype);
    
    switch (cmd) {
        
        case CMD_RETURN_STATUS: {
        
            int returnstat = -1;
            returnstat = atoi(msg);
            printf("return status = %i\n",returnstat);
            if(returnstat != 0) {
                return returnstat; // The action failed
            }
            break;
        }
        
        case CMD_RETURN_STDOUT: {
            if(atoi(msglen) != 0) {                
                fprintf(stdout,"%s",msg);
            }
            break;
        }
            
        case CMD_RETURN_STDERR: {
            if(atoi(msglen) != 0) {                
                fprintf(stderr,"%s",msg);
            }
            break;
        }
            
        case CMD_RETURN_FILE: {     
            int recvfilestat = -1;
            recvfilestat = receive_file(sd, msg);
            if(recvfilestat != 0) {
                return recvfilestat;
            }
            break;
        }
        
        case CMD_END: // Handled in send_actionset()
            
            break;
            
        default:
        
            break;
    }
    
    return 0;
}
