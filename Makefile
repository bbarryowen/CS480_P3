#825730146
# Compiler
CXX = g++
# Compiler flags
CXXFLAGS = -std=c++11 -Wall -g3

# Source files and object files
SOURCES = demandpaging.cpp pagetable.cpp vaddr_tracereader.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Target executable
PROGRAM = demandpaging

# Rules
all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(PROGRAM) $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(PROGRAM)

.PHONY: all clean
