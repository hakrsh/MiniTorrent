server:tracker.cpp
	g++ -std=c++17 tracker.cpp -pthread -o server
client:client.cpp
	g++ -std=c++17 -g sha.cpp client.cpp -pthread -lcrypto -o client
clean:
	rm tracker client
