#include "rake-c.h"

int main(int argc, char* argv[]) 
{
    if(argc > 2) { 
        fprintf(stderr,"Maximum of 1 argument to specify non-standard Rakefile path");
        exit(EXIT_FAILURE);
    }
    
    char **hosts = NULL;
    int nhosts;
    char default_port[6]; // Maximum port = 2^16 - 1 = 65535 (5 chars + '\0')
	
    ALLACTIONS actions = {.nactionsets = 0, .actionsetarr = NULL}; // Initialise main datastructure
    
    // Read and store the contents of the Rakefile
    if(argc == 1) {
        parsefile("Rakefile", &hosts, &nhosts, default_port, &actions);
    }
    else {
        parsefile(argv[1], &hosts, &nhosts, default_port, &actions);
    }
    
    // Print out the Rakefile contents
    // print_datastructure(&actions);
    
    // printf("Hosts:\n");
    // for(int i = 0; i < nhosts; i++) { 
        // printf("\t%s\n", hosts[i]);
    // }
    
	// printf("Default port: %s\n",default_port);
    
    
    // Handle communication with rakeserver(s)
    connect_rakeserver(actions, hosts, nhosts, default_port);

    // Free all dynamically allocated memory
    free_datastructure(&actions);
    
    exit(EXIT_SUCCESS);
}
