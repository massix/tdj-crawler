CXX = clang++
CXXFLAGS = -Wall -fPIC -std=c++11 -I../liblali/flate -I../liblali/json11 -I../liblali/web -O3
TARGET = tdj-crawler
OBJECTS = connection.o request.o response.o game.o collection.o user.o database.o main.o

.PHONY: clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) -L../liblali/web -L../liblali/json11 -L../liblali/flate -lflate -ljson11 -llali -lsqlite3 $(OBJECTS)

%.cpp: %.o

clean:
	rm -f *.o $(TARGET)
