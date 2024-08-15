CC = g++ -std=c++17

main.o: main.cpp bills.o building.o
	$(CC) main.cpp -o main.o

counter.o: counter.cpp
	$(CC) counter.cpp -o counter.o

building.o: bills.o counter.o building.cpp 
	$(CC) building.cpp -o building.o

bills.o: counter.o bills.cpp
	$(CC) bills.cpp -o bills.o

.PHONY: clean
clean:
	rm -f counter.o bills.o building.o main.o 