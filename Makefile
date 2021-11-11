all: tracker client
tracker:tracker.cpp
	g++ -std=c++17 tracker.cpp -pthread -o tracker
client:client.cpp
	g++ -std=c++17 client.cpp -pthread -lcrypto -o client
clean:
	rm tracker client
