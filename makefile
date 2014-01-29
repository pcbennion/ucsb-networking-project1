server_cpp_tcp.o: server_cpp_tcp.cpp
	g++ -c server_cpp_tcp.cpp

server_cpp_udp.o: server_cpp_udp.cpp
	g++ -c server_cpp_udp.cpp

client_cpp_tcp.o: client_cpp_tcp.cpp
	g++ -c client_cpp_tcp.cpp

client_cpp_udp.o: client_cpp_udp.cpp
	g++ -c client_cpp_udp.cpp

all: server_cpp_tcp.o server_cpp_udp.o client_cpp_tcp.o client_cpp_udp.o
	g++ server_cpp_tcp.o -o server_tcp
	g++ server_cpp_udp.o -o server_udp
	g++ client_cpp_tcp.o -o client_tcp
	g++ client_cpp_udp.o -o client_udp

clean:
	rm -rf *.o server_tcp server_udp client_tcp client_udp
