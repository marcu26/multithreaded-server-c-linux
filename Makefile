output: client server

server:
	rm -f server
	gcc server.c -o server -pthread

client:
	rm -f client
	gcc client.c -o client -pthread 

clean:
	rm -f server
	rm -f client