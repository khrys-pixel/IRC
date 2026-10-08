# ft_irc Makefile
NAME = ircserv

BBLK = \e[1;30m
BRED = \e[1;31m
BGRN = \e[1;92m
BYEL = \e[1;33m
BBLU = \e[1;34m
BMAG = \e[1;35m
BCYN = \e[1;36m
BWHT = \e[1;37m
CRESET = \e[0m

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98
INCLUDES = -I includes
RM = rm -rf

SRC_DIR = srcs/
OBJ_DIR = objs/

SRCS = $(addprefix $(SRC_DIR), main.cpp IrcServer.cpp IrcServerCommands.cpp IrcServerGetCommands.cpp Client.cpp Channel.cpp)
OBJS = $(SRCS:.cpp=.o)
OBJS := $(patsubst $(SRC_DIR)%, $(OBJ_DIR)%, $(OBJS))

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@printf "\r\e[K$(BGRN)Compiling: %s$(CRESET)" "$<"
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

all: $(NAME)

$(NAME): $(OBJS)
	@printf "\r\e[K"
	@echo "$(BBLU)Compiling $(NAME)$(CRESET)"
	@$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(NAME)
	@echo "$(BBLU)$(NAME) compiled!$(CRESET)"

# %.o: %.cpp
# 	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@$(RM) $(OBJ_DIR)
	@echo "$(BRED)object files cleaned!$(CRESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(BRED)$(NAME) cleaned!$(CRESET)"
	@echo ""

re: fclean all

.PHONY: all clean fclean re
