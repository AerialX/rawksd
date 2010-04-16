CXX := g++
CXXFLAGS := -O2

LIBS := -lpthread
OBJECTS := riifs.o riifs_pthread.o

riifs: $(OBJECTS)
	$(CXX) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f riifs
