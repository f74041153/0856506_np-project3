all : http_server.cpp console.cpp
	g++ http_server.cpp -o http_server -std=c++11 -Wall -pedantic -pthread -lboost_system
	g++ console.cpp -o console.cgi -std=c++11 -Wall -pedantic -pthread -lboost_system
main: main.cpp
	g++ main.cpp -o main -std=c++11 -Wall -pedantic -pthread -lboost_system
