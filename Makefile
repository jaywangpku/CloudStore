all:server client test

server:server.o server_func.o
	cc -o server server.o server_func.o -lpthread

server.o:server.c server_func.h
	cc -c server.c -lpthread

server_func.o:server_func.c server_func.h
	cc -c server_func.c -lpthread

client:client.o client_func.o client_file.o
	cc -o client client.o client_func.o client_file.o -lpthread

client.o:client.c client_func.h
	cc -c client.c -lpthread

client_func.o:client_func.c client_func.h
	cc -c client_func.c -lpthread

client_file.o:client_file.c client_func.h
	cc -c client_file.c

test:test.o
	cc -o test test.c
