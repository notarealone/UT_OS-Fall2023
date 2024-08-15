CC = g++ -std=c++11

run : main.o
	./ImageFilters.out input.bmp

main.o : main.cpp
	$(CC) main.cpp -o ImageFilters.out

.PHONY: clean
clean:
	rm -f output.bmp ImageFilters.out