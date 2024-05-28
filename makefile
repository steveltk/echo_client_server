all: echo-client echo-server

echo-client: echo-client.o
	g++ -pthread -o echo-client echo-client.o

echo-server: echo-server.o
	g++ -pthread -o echo-server echo-server.o

echo-client.o: echo-client.cpp

echo-server.o: echo-server.cpp

clean:
	rm -rf echo-client echo-server *.o
