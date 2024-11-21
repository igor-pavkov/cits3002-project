#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "strsplit.h"
#include "protocol.h"

#define CHECK_ALLOC(p) if(p == NULL) { perror(__func__); exit(EXIT_FAILURE); }

#if     defined(__linux__)
extern  char    *strdup(const char *string);
#endif

typedef struct _action {
    char *command;
    bool remote;
    int nfiles;
    char **requirements; // Array of strings
} ACTION;

typedef struct _actionset{ // List of actions
    int nactions;
    ACTION **actionarr; // Array of action pointers
} ACTIONSET;

typedef struct _allactions { // Contains all actionsets
    int nactionsets;
    ACTIONSET **actionsetarr; // Array of actionset pointers
} ALLACTIONS;


// Defined in parsing.c
extern ACTION *action_new(char *command);

extern ACTIONSET *actionset_new();

extern void parsefile(char *filename, char ***hosts, int *nhosts, char *default_port, ALLACTIONS *actions);

extern void print_datastructure(ALLACTIONS *actions);

extern void free_datastructure(ALLACTIONS *actions);


// Defined in networking.c
extern void connect_rakeserver(ALLACTIONS actions, char **hosts, int nhosts, char *default_port);

extern int new_connection(char *host, char *default_port);

extern int send_actionset(ACTIONSET *set, char **hosts, int nhosts, char *default_port);

extern int get_lowest_quote(char **hosts, int nhosts, char *msg);

extern int action_send(int sd, ACTION *action);

extern int handle_command(int sd, char *msg, char *cmdtype, char *msglen);


// defined in packet.c
extern int send_packet(int sd, char *msg, int msglen, CMDTYPE type);

extern int send_file(int sd, char *filename, int filesize);

extern void receive_header(int sd, char *cmdtype, char *msglen);

extern char *receive_packet(int sd, char *cmdtype, char* msglen);

extern int receive_file(int sd, char *filename);