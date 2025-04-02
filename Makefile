# Un ping sur 2
# sudo iptables -A INPUT -p icmp -m statistic --mode random --probability 0.5 -j DROP
# Remettre normal
# sudo iptables -D INPUT -p icmp -m statistic --mode random --probability 0.5 -j DROP
