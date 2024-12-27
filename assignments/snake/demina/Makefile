LDFLAGS = -lncurses -pthread -std=c++17
CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -O2

TARGET = snake
SOURCES = snake.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
        $(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.cpp
        $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
        rm -f $(OBJECTS) $(TARGET)