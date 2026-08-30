# Un ping sur 2
# sudo iptables -A INPUT -p icmp -m statistic --mode random --probability 0.5 -j DROP
# Remettre normal
# sudo iptables -D INPUT -p icmp -m statistic --mode random --probability 0.5 -j DROP
NAME = ft_ping
CC = cc
CFLAGS = -Werror -Wall -Wextra
RM = rm -rf

SRC = main.c \
	printUtils.c \

OBJ = $(SRC:.c=.o)

all : $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME) -lm

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

docker-build:
	docker build -t ft_ping_env .

docker:
	@docker image inspect ft_ping_env > /dev/null 2>&1 || docker build -t ft_ping_env .
	docker run --rm -it --cap-add=NET_RAW -v "$$(pwd)":/app -w /app ft_ping_env bash

test:
	@docker image inspect ft_ping_env > /dev/null 2>&1 || docker build -t ft_ping_env .
	docker run --rm -it --cap-add=NET_RAW -v "$$(pwd)":/app -w /app ft_ping_env sh -c "make && ./ft_ping 8.8.8.8"

PHONY: all clean fclean re docker docker-build test


#ping --ttl=1 8.8.8.8 -v