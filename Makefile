FLAGS=-Wall -Wextra -std=c++17

server:
	g++ $(FLAGS) server.cpp utils.cpp -o server
subscriber:
	g++ $(FLAGS) subscriber.cpp utils.cpp -o subscriber

clean:
	rm -f server subscriber