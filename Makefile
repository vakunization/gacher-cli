CXX = g++
CXXFLAGS = -std=c++17 -I lib -I include
LDFLAGS = -lcurl -lssl -lcrypto -lsqlite3

TARGET = gacher

all: $(TARGET)

$(TARGET) : src/main.cpp include/db_interface.hpp include/parser.hpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean :
	rm -f $(TARGET)
