NAME = tsp_solver

SRC = main.cpp

OBJ = $(SRC:.cpp=.o)

INCLUDES = -Iincludes

default:
	g++ $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(NAME)