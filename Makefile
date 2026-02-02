NAME     := webserv
CC       := c++
INC_DIR  := include
CFLAGS   := -Wall -Wextra -Werror -std=c++98 -I$(INC_DIR)
RM       := rm -rf

SRC_DIR  := src
OBJ_DIR  := objs

SRCS     := $(SRC_DIR)/main.cpp \
						$(SRC_DIR)/Server.cpp \
						$(SRC_DIR)/ListenSocket.cpp

OBJS     := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
