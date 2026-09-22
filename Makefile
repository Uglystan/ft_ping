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

setup-host-unreachable:
	@sudo ip netns add router 2>/dev/null || true
	@sudo ip link add veth-vm type veth peer name veth-rtr 2>/dev/null || true
	@sudo ip link set veth-rtr netns router 2>/dev/null || true
	@sudo ip addr add 192.168.99.1/24 dev veth-vm 2>/dev/null || true
	@sudo ip link set veth-vm up
	@sudo ip netns exec router ip addr add 192.168.99.2/24 dev veth-rtr 2>/dev/null || true
	@sudo ip netns exec router ip link set veth-rtr up
	@sudo ip netns exec router sysctl -w net.ipv4.ip_forward=1 >/dev/null
	@sudo ip route add 1.1.1.1 via 192.168.99.2 2>/dev/null || true
	@echo "Routeur virtuel active ! Vous pouvez tester './ft_ping 1.1.1.1'"

setup-ttl:
	@sudo ip netns add router 2>/dev/null || true
	@sudo ip link add veth-vm type veth peer name veth-rtr 2>/dev/null || true
	@sudo ip link set veth-rtr netns router 2>/dev/null || true
	@sudo ip addr add 192.168.99.1/24 dev veth-vm 2>/dev/null || true
	@sudo ip link set veth-vm up
	@sudo ip netns exec router ip addr add 192.168.99.2/24 dev veth-rtr 2>/dev/null || true
	@sudo ip netns exec router ip link set veth-rtr up
	@sudo ip netns exec router sysctl -w net.ipv4.ip_forward=1 >/dev/null
	@sudo ip netns exec router ip link add dummy0 type dummy 2>/dev/null || true
	@sudo ip netns exec router ip link set dummy0 up
	@sudo ip netns exec router ip route add default dev dummy0 2>/dev/null || true
	@sudo ip route add 1.1.1.1 via 192.168.99.2 2>/dev/null || true
	@echo "Routeur virtuel actif. TTL=1 sur 1.1.1.1"


clean-ttl:
	@sudo ip route del 1.1.1.1 2>/dev/null || true
	@sudo ip link del veth-vm 2>/dev/null || true
	@sudo ip netns del router 2>/dev/null || true
	@echo "Routeur virtuel supprime."


PHONY: all clean fclean re clean-ttl setup-ttl