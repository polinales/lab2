all:
	gcc server.c -o lab2server -w -pthread
	gcc client.c -o lab2client -w
	
