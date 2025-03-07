# Compiler
CXX = g++ -g

# Compilation options
CXXFLAGS = -std=c++11 -I$(GUROBI_HOME)/include/
LDFLAGS = -L$(GUROBI_HOME)/lib/ -lgurobi_c++ -lgurobi110 -lm

# Target executable
TARGET = mlrcs

# Source files and object files
SRCS = main.cpp Graph.cpp Node.cpp Scheduler.cpp ILPScheduler.cpp
OBJS = $(SRCS:.cpp=.o)

# Build target
all: $(TARGET)

# Compile each object file
%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# Generate executable
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)
