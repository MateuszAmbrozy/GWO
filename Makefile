NAME = tsp_solver

SRC = main.cpp

OBJ = $(SRC:.cpp=.o)

INCLUDES = -Iincludes

FLAGS = -O3 -pg -g -fno-omit-frame-pointer -std=c++17

default:
	g++ $(CXXFLAGS) $(INCLUDES) $(SRC) $(FLAGS) -o $(NAME)