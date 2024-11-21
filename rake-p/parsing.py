# Deals with parsing and storage of Rakefile data

def parsefile(filename):
    
    try:
        file = open(filename,"r")
    except FileNotFoundError as e:
        print(e.message)
        sys.exit(1)
    except:
        print(f"The file: {filename} could not be opened.")
        sys.exit(1)
    
    actions = []
    
    for line in file:
        line = line.strip("\r\n") # Strip carriage return/newline characters
        
        if (len(line) == 0) or (line[0] == "#"): # Skip empty and comment lines
            continue
        
        if line[0] == "\t": # Line starts with a tab
            
            if line[1] == "\t": # Line starts with two tabs (required files)
                line = line.strip("\t")
                actions[-1][-1]["requirements"].extend(line.split()[1:])
        
            else: # Line starts with one tab (actions)
                line = line.strip("\t")
                
                if line[0:6] == "remote":
                    remote = True
                    line = line[7:] # Remove 'remote-' from command
                else:
                    remote = False
                    
                action = {
                    "command": line,
                    "remote": remote,
                    "requirements": []
                }
                
                actions[-1].append(action) # Append the action to the last set
                
        
        else: # Line starts with no tab
            if line[0:4] == "PORT":
                default_port = line.split()[2] # Assign the port number
            
            
            if line[0:5] == "HOSTS":
                hosts = line.split()[2:] # List of strings assigned to hosts
                for i in range(len(hosts)):
                    if ":" not in hosts[i]: # Use the default port
                        hosts[i] = hosts[i] + ":" + default_port # Concatenate port number
                
            
            if line[0:9] == "actionset":
                actions.append([]) # Append an empty actionset
                
    
    file.close()
    return actions, hosts, default_port

def read_datastructure(actions):
    print("\nReading Datastructure...\n")
    nactionsets = len(actions)
    
    print(f"\nNumber of actionsets: {nactionsets}")
    
    for i in range(nactionsets):
        nactions = len(actions[i])
        
        print(f"Actionset {i}: \t(nactions: {nactions})\n")
        
        for j in range(nactions):
            command = actions[i][j]["command"]
            nfiles = len(actions[i][j]["requirements"])
            remote = actions[i][j]["remote"]
            
            print(f"\t{command}\t\t(remote = {remote})\t(nfiles: {nfiles})")
            
            requirements = actions[i][j]["requirements"]
            if len(requirements) != 0:
                print(f"\t\t{requirements}")

