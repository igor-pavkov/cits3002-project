# CITS3002 Project
### Overview
This project (originally completed on 25/05/2022), utilised sockets in a client-server architecture. It includes a server program, written in Python, as well as two versions of a client program, written in C and Python respectively. The server supports concurrency, allowing it to serve multiple clients simultaneously.

### Purpose
The project intent was to support the building and execution of C programs remotely on a host server, which was ideally more powerful than the client machine. The compilation instructions would be sent from the client to the server along with any required files. The server would then send the program output as well as any files produced by the program back to the client. More generally, the project allows for remote program execution, as the client is able to request a server to run any command-line program provided that it already exists on the server.

### Rakefiles
The client program reads commands from a modified type of Makefile called a Rakefile (Remote Makefile). An example Rakefile is given in the root directory. The set of hosts to contact along with a default port are specified at the start of the file. Commands are grouped into actionsets, with one actionset having to complete before the next one begins. Within an actionset, commands are run top-to-bottom. The 'remote' prefix specifies that the command should be run by the host, while commands without the prefix are run locally on the client. Any required files for a command can be specified with the 'requires' keyword and will be sent to the host provided they exist in the client directory.




### Protocol
As part of the project, a simple network protocol was developed for communication between the client and server programs. All communication and transmission of data between client and server occurs through the sending
of strings (with terminating null byte omitted), preceded by a variable-length header with a maximum size of 16 bytes. The header consists of two fields, both represented by strings of digits:
* CMDTYPE (max 2 bytes) – an enumerated type (represented by an integer) that indicates to
the receiving end what should be done with the message. The enumerated values are defined in common/protocol.py
* LENGTH (max 12 bytes) – The length of the message being sent (in bytes)

The remaining two bytes are taken by the ‘/’ dividers separating the fields from each other, and from the message. A typical packet (e.g., for the execution of the cal command) will look like the following: “3/11/cal 10 2022”

Although the maximum header length is 16 bytes, headers tend to be much shorter for most
messages (almost always less than 8 bytes). As the client and server both deal with variable header and field lengths, both programs can easily be modified to support larger header/field sizes if required.

When either the client or server expects to receive a packet, they first ‘peek’ the header using the MSG_PEEK recv() flag to determine the length of the header as well as the values of its fields. Then the header is received (without the flag) to remove it from the receive queue. The message itself can then be received, as the length field of the header indicates the number of bytes to receive.

After the client sends an action’s required files to the server, it waits on the recv() call for an acknowledgement that the server has received all of the files before sending the execute command. This is the only case where an acknowledgement is sent between client and server as ‘execute’ is the only command that has prerequisites for it to be handled. Otherwise, the protocol proceeds without the need for acknowledgements.

### Walkthrough
A typical set of steps for remote compilation is outlined below.

1. The client reads the Rakefile and stores the default port, host addresses and command
information in its data structures
2. The client begins with the first actionset, sequentially sending every host a quote request
along with the first command. These quotes determine which host is considered least expensive to use.
3. The client connects to the host that returned the lowest quote
4. The client sends a message consisting of the number of files required for the compilation
command, followed by the c file(s) to be compiled in the action
5. The server creates a temporary directory and receives the files, then sends back an
acknowledgement that it is ready to receive the compilation command
6. The client receives the acknowledgment and sends the compilation command
7. The server runs the command (via the subprocess.run() function), and stores the exit status
as well as any stdout and stderr output in variables
8. The server checks to see if a file was created with the command and stores the filename of
the newest file (the produced object file)
9. The server sends the exit status, stdout and stderr output, and produced file to the client,
followed by the End command to indicate that there is nothing more to be sent (then
removes the temporary directory and closes the socket)
10. The client receives the object file along with the other output from the command and closes
the socket upon receiving the End command
11. Steps 2-10 are repeated for the next actions/actionsets until all of the object files are
returned to the client directory
12. Next (following similar steps as above), the client sends all of the object files back to the
server
13. The server links the object files into one executable file
14. The server sends the executable file back to the client


### Ideal Conditions for Remote Compilation and Linking
Unlike local compilation, where required data can simply be looked up on disk, remote compilation
comes with the added overhead of having to send data (most notably, files) over networks. This
means that for smaller programs, a bottleneck will be hit and local compilation will be faster overall, even if a remote machine is significantly more powerful. However, for larger programs, this overhead becomes insignificant compared to the amount of time taken to compile, so remote
compilation on a more powerful machine will perform better.

The speed of remote compilation scales with the number of hosts available, as the total workload
can be split between the server machines. This is most impactful when the program to be compiled
and linked includes many source files, as a greater number of hosts allow more source files to be
compiled in parallel. This is another reason why larger programs would benefit more from remote
compilation.

Other considerations for using remote compilation include the distance between client and server
and the level of network congestion. Both of these factors will affect how quickly the client and
server can communicate and send data to each other as well as the likelihood that packets will be
lost, so low congestion and a server close to the client make remote compilation more viable. This means that remote compilation is significantly more practical if remote servers are in the same building or part of the same network as the client rather than in separate countries.