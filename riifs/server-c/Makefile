TARGET := riifs

CXX ?= g++
CXXFLAGS := -O2

OBJECTS := riifs.o

ifeq ($(OS),Windows_NT)
OBJECTS += riifs_win32.o
LIBS := -lws2_32
TARGET := $(TARGET).exe
else
OBJECTS += riifs_pthread.o
LIBS := -lpthread
endif

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f $(TARGET)
