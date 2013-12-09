all: client server
client: client_PFS.c
	gcc -o client_PFS client_PFS.c -I.
server: server_PFS.c
	gcc -o server_PFS server_PFS.c -I.
clean:
	rm -f server_PFS client_PFS *.o
