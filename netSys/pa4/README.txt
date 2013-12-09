Author: Gabe Russotto
Class: Network Systems
Date: 11.8.13

Programming Assignment 4

COMPILATION:
	Simply call 'make' to compile client_PFS.c and server_PFS.c into
	executables and run. Or 'make clean' to clean directory.

USAGE: (Same as the writeup)
	$ ./server_PFS <Port>
	$ ./client_PFS <Client_name> <ServerIP> <ServerPort>
		
		Client Commands:
		1) 'get <filename>' - Downloads file <filename> from a client
		2) 'ls'             - Prints the master file list
		3) 'exit'           - Shuts down client safely

	
PROGRAM DESCRIPTION: This program implements a simple peer file sharing
	program. The file server is ran on a specific port, then any number of
	clients may connect the file server, up to 5 clients were tested.
	Once a client connects it shares the files in the directory it is 
	running in with the central file server, who then shares the files 
	with all other connected clients. The clients may then begin sharing 
	files with each other. File size up to 64kb work, larger files may not
	transfer correctly. 
	
BUGS:
- If client does not exit properly (exit is not typed at client), the 
file list on the server becomes corrupt.

- Spaces within file names are not read correctly by program from stdin.
