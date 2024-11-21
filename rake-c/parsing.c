// Deals with parsing and storage of Rakefile data

#include "rake-c.h"

#define LINESIZE    1024

// Initialise a new action
ACTION *action_new(char *command) 
{
    ACTION *new = calloc(1,sizeof(ACTION));
    CHECK_ALLOC(new);
    new->command = strdup(command);
    CHECK_ALLOC(new->command);
    new->remote = false;
    new->nfiles = 0;
    new->requirements = NULL;
    return new;
}

// Initialise a new actionset
ACTIONSET *actionset_new() 
{ 
    ACTIONSET *new = calloc(1,sizeof(ACTIONSET));
    CHECK_ALLOC(new);
    new->nactions = 0;
    new->actionarr = NULL;
    return new;
    
}

void parsefile(char* filename, char ***hosts, int *nhosts, char *default_port, ALLACTIONS *actions)
{
    FILE *file = fopen(filename, "r");
    
    if(file == NULL) 
    {
        perror("Unable to open Rakefile");
        exit(EXIT_FAILURE);
    }
    
    char *line;
    line = calloc(LINESIZE,sizeof(char));
    CHECK_ALLOC(line);

	int nwords;
	int nsets = 0;
	int nactions = 0;
	char **requiredfiles = NULL;
    
    // Pointers to the current set and current action for better readability
	ACTIONSET *currentset = NULL;
	ACTION *currentaction = NULL;
	
    while(fgets(line, LINESIZE, file) != NULL) {
        
        line[strcspn(line,"\r\n")] = '\0'; // Cut off any trailing '\n' or '\r'        
        
        if(line[0] == '\0' || line[0] == '#') { // Skip empty lines and comment lines
            continue;
        }
        

        if(line[0] == '\t') { 
        
            if(line[1] == '\t') { // The line begins with two tabs (required files)
                
                line = strrchr(line,'\t') + 1; // Skip tabs at the beginning of line
            
                nwords = 0;
                requiredfiles = strsplit(line,&nwords);
                
                // Allows for multiple reallocations if there is more than one line for requirements
                currentaction->requirements = realloc(currentaction->requirements, currentaction->nfiles*sizeof(char*) + (nwords-1)*sizeof(char*));
                CHECK_ALLOC(currentaction->requirements);

                // Ignore the first word ("requires")
                
				for(int i = 0; i < nwords - 1; i++) {
					currentaction->requirements[i] = calloc(strlen(requiredfiles[i+1]+1),sizeof(char)); 
                    CHECK_ALLOC(currentaction->requirements[i]);
                    strcpy(currentaction->requirements[i], requiredfiles[i + 1]);
				}
				currentaction->nfiles += (nwords - 1); // Update the number of files
                
            }
			else {	// The line begins with one tab (actions) 

                line = strrchr(line,'\t') + 1; // Skip tabs at the beginning of line
                
                nactions++;
                bool remote = false;
                
                if(strncmp(line,"remote",6) == 0) { // Remote "remote-" from remote actions
                    remote = true;
                    line = strchr(line,'-') + 1; // Start the line after the '-'
                }

                
                // Realloc actions actionarr to include one more pointer
                actions->actionsetarr[nsets-1]->actionarr = realloc(actions->actionsetarr[nsets-1]->actionarr,nactions*sizeof(ACTION *));
                CHECK_ALLOC(actions->actionsetarr[nsets-1]->actionarr);
                
                // Point to the current action
                actions->actionsetarr[nsets-1]->actionarr[nactions-1] = NULL;
                
                actions->actionsetarr[nsets-1]->actionarr[nactions-1] = realloc(actions->actionsetarr[nsets-1]->actionarr[nactions-1],sizeof(*currentaction));
                CHECK_ALLOC(actions->actionsetarr[nsets-1]->actionarr[nactions-1]);
                
                actions->actionsetarr[nsets-1]->actionarr[nactions-1] = action_new(line); //assumes the command doesn't go over multiple lines
                
                // Update currentaction
                currentaction = actions->actionsetarr[nsets-1]->actionarr[nactions-1]; 
                
                currentaction->remote = remote;
                
                // nfiles and requirements fields dealt with in 'two tabs' block
				
			}
            
        }
        else { // The line begins with no tab
        
			if(strncmp(line,"PORT",4) == 0) { // Initialise default port string
				nwords = 0;
                char **portstr = strsplit(line,&nwords);
                strcpy(default_port, portstr[2]);
            }
            else if(strncmp(line,"HOSTS",5) == 0) { // Store host strings - maybe default to localhost if no host specified
                nwords = 0;
                char **hoststr = strsplit(line,&nwords);
                *nhosts = nwords - 2;
                           
                char **temphosts = calloc(*nhosts,sizeof(char *));
                CHECK_ALLOC(temphosts);
                
                for(int i = 2; i < nwords; i++) {
                    temphosts[i-2] = calloc((strlen(hoststr[i])+1), sizeof(char));
                    CHECK_ALLOC(temphosts[i-2]);
                    strcpy(temphosts[i-2],hoststr[i]);
                    if(strchr(temphosts[i-2],':') == NULL) { // Append the default port if no port is specified
                        strcat(temphosts[i-2],":");
                        strcat(temphosts[i-2],default_port);
                    }
                   
                }
                
                *hosts = temphosts;
                
            }
            else if (strncmp(line, "actionset",9) == 0) {
                // Update number of actions in previous set
                if(currentset != NULL) {
                    currentset->nactions = nactions;                    
                }
                nactions = 0; // Reset the action counter
                nsets++;
                
                // Realloc actions actionsetarr to include one more pointer
                actions->actionsetarr = realloc(actions->actionsetarr,nsets*sizeof(ACTIONSET *));
                CHECK_ALLOC(actions->actionsetarr);
                
                actions->actionsetarr[nsets-1] = NULL;
                actions->actionsetarr[nsets-1] = realloc(actions->actionsetarr[nsets-1], sizeof(ACTIONSET));
                CHECK_ALLOC(actions->actionsetarr[nsets-1]);
                
                actions->actionsetarr[nsets-1] = actionset_new();
                currentset = actions->actionsetarr[nsets-1];
                
            }
                
           
        }
    }
	
    // Update number of actions in the last set
        if(currentset != NULL) {
            currentset->nactions = nactions;                    
        }
    
	// Update the number of actionsets
	actions->nactionsets = nsets;
    
    fclose(file);
}

void print_datastructure(ALLACTIONS *actions) { 

    printf("\nReading Datatstructure...\n");
    
    printf("\nnactionsets: %i\n\n", actions->nactionsets);
    
    for(int i = 0; i < actions->nactionsets; i++) {
        
        printf("Actionset %i: \t(nactions: %i)\n", i+1, actions->actionsetarr[i]->nactions);
        
        for(int j = 0; j < actions->actionsetarr[i]->nactions; j++) {
            
            printf("\t%s\t\t(remote = %s)\t(nfiles: %i)\n",actions->actionsetarr[i]->actionarr[j]->command, actions->actionsetarr[i]->actionarr[j]->remote ? "true" : "false", actions->actionsetarr[i]->actionarr[j]->nfiles);
            
            for(int k = 0; k < actions->actionsetarr[i]->actionarr[j]->nfiles; k++) {
                
                printf("\t\t%.*s\n",(int) strlen(actions->actionsetarr[i]->actionarr[j]->requirements[k]),actions->actionsetarr[i]->actionarr[j]->requirements[k]);
                
            }
        }
    } 
}

void free_datastructure(ALLACTIONS *actions) { 

    for(int i = 0; i < actions->nactionsets; i++) {
        
        for(int j = 0; j < actions->actionsetarr[i]->nactions; j++) {

            free(actions->actionsetarr[i]->actionarr[j]->command);
            
            for(int k = 0; k < actions->actionsetarr[i]->actionarr[j]->nfiles; k++) {
                
                free(actions->actionsetarr[i]->actionarr[j]->requirements[k]);
                
            }
            
            free(actions->actionsetarr[i]->actionarr[j]->requirements);
            
        }
    }
}