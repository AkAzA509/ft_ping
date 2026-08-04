BIN_DIR		:= bin
NAME		:= $(BIN_DIR)/ft_ping

CXX			:= gcc
CFLAGS		:= -Wall -Wextra -Werror -O2 -fsanitize=address,leak -g2

SRC			:= main.c parsing.c

OBJ_DIR		:= objs/
OBJ			:= $(SRC:%.c=$(OBJ_DIR)%.o)

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CFLAGS) -o $(NAME) $(OBJ)

$(OBJ_DIR)%.o: %.c
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)

fclean: clean
	@rm $(NAME)

re: fclean all