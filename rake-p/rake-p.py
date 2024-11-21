import sys

import parsing
import networking

def main(): # Error check arguments
    
    if len(sys.argv) > 2:
        print("Maximum of 1 argument to specify non-standard Rakefile path")
        exit(1)
    
    if len(sys.argv) == 1:
        actions, hosts = parsing.parsefile("Rakefile")
    
    else:
        filename = sys.argv[1]
        actions, hosts, default_port = parsing.parsefile(filename)
    
#     parsing.read_datastructure(actions)
#     print(f"Hosts: {hosts}")
#     print(f"Default port: {default_port}")
    
    networking.connect_rakeserver(actions,hosts,default_port)
    
    exit(0)
    
main()
    