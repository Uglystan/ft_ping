NAME = ft_ping
CC = cc
CFLAGS = -Werror -Wall -Wextra -Iinclude
RM = rm -rf

SRC = src/main.c \
	src/printUtils.c \
	src/parse.c \
	src/createPacket.c \
	src/ping.c

OBJ = $(SRC:.c=.o)

all : $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME) -lm

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

PHONY: all clean fclean re

#sudo ip netns add router
#sudo ip link add veth-vm type veth peer name veth-rtr
#sudo ip link set veth-rtr netns router
#sudo ip addr add 192.168.99.1/24 dev veth-vm
#sudo ip link set veth-vm up
#sudo ip netns exec router ip addr add 192.168.99.2/24 dev veth-rtr
#sudo ip netns exec router ip link set veth-rtr up
#sudo ip netns exec router sysctl -w net.ipv4.ip_forward=1
#sudo ip route add 1.1.1.1 via 192.168.99.2


#sudo ip route del 1.1.1.1
#sudo ip link del veth-vm
#sudo ip netns del router