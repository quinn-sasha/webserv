NAME     := webserv
CC       := c++
INC_DIR  := include
CFLAGS   := -Wall -Wextra -Werror -std=c++98 -I$(INC_DIR)
RM       := rm -rf

SRC_DIR  := src
OBJ_DIR  := objs

SRCS_NO_MAIN := $(SRC_DIR)/AcceptHandler.cpp \
                $(SRC_DIR)/ClientHandler.cpp \
                $(SRC_DIR)/ListenSocket.cpp \
                $(SRC_DIR)/Parser.cpp \
                $(SRC_DIR)/RequestProcessor.cpp \
                $(SRC_DIR)/Response.cpp \
                $(SRC_DIR)/Server.cpp \
                $(SRC_DIR)/pollfd_utils.cpp \
                $(SRC_DIR)/string_utils.cpp

OBJS_NO_MAIN := $(SRCS_NO_MAIN:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
MAIN_OBJ     := $(OBJ_DIR)/main.o

TEST_NAME := unit_test
TEST_DIR  := test
GTEST_INC := -I/opt/homebrew/include
GTEST_LIB := -L/opt/homebrew/lib -lgtest -lgtest_main -lpthread
TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS := $(TEST_SRCS:$(TEST_DIR)/%.cpp=$(OBJ_DIR)/test/%.o)
test: CFLAGS := -Wall -Wextra -Werror -std=c++17 -I$(INC_DIR)

all: $(NAME)

$(NAME): $(OBJS_NO_MAIN) $(MAIN_OBJ)
	$(CC) $(CFLAGS) $^ -o $(NAME)

test: $(TEST_NAME)
	./$(TEST_NAME)

$(TEST_NAME): $(OBJS_NO_MAIN) $(TEST_OBJS)
	$(CC) $(TEST_CFLAGS) $(GTEST_INC) $^ $(GTEST_LIB) -o $(TEST_NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/test/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(GTEST_INC) -c $< -o $@

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME) $(TEST_NAME)

re: fclean all

.PHONY: all clean fclean re
