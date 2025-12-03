CXX = g++
CXXFLAGS = -std=c++17 -O3 -g -pg -fno-omit-frame-pointer -pthread -march=native
INCLUDES = -Iincludes

SRC = main.cpp
OBJ = $(SRC:.cpp=.o)
NAME = tsp_solver

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(NAME)