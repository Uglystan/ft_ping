#include "parse.h"

/*
struct sockaddr_in exclusivement dedie a l'IPV4:
    - sin_family (2 octects): pour dire qu'elle type d'adresse (AF_INET = IPV4)
    - sin_port (2 octects): Le numero de port a 0 pour ping
    - sin_addr (4 octects): Chaque octect est un nombre de l'adresse ip (sous structure uint32_t s_addr)
    - sin_zero[8] (8 octects): Remplis de 0 pour que la structure fasse 16 octects comme sockaddr qui est la structure generique
*/

/*
struct addrinfo pour communiquer avec getaddrinfo:
    - ai_flags (int): Option supplementaire specifique
    - ai_family (int): Pour specifier un type d'ip de retour (IPV4 = AF_INET, IPV6 AF_INET6 ou les deux AF_UNSPEC)
    - ai_socktype (int): type de socket (SOCK_STREAM -> TCP, SOCK_RAW -> socket brut)
    - ai_protocol (int): Protocole utilise (0 pour ping mais sinon ca sert a preciser si on veut du TCP ou de l'UDP)
    - ai_addrlen (socklen_t): Taille de ai_addr (IPV4 16octects)
    - ai_addr (* sockaddr): Adresse IP resolue -> pointeur vers sockaddr_in
    - ai_canonname (* char): Le nom d'hote officiel
    - ai_next (*addrinfo): Pointeur vers le resultat suivant de la lsite chainee (1 nom de domaine peut avoir plusieurs IP dans le monde)
*/

bool resolveHost(struct arguments *arguments) {
  struct addrinfo hints = {0}; // Restriction de recherche pour getaddrinfo
  struct addrinfo *res = NULL; // 

  hints.ai_family = AF_INET; // Forcer IPv4 pour n'avoir que les IPV4
  int status = getaddrinfo(arguments->host, NULL, &hints, &res); // Resolution DNS pas de port car socket_raw
  if (status != 0) {
    return (false);
  }
  // Récupérer l'adresse IPv4
  struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr; // On a uniquement besoins de l'IP du nom de domaine (sockaddr_in uniquement pour IPV4)
  arguments->destAddress = *ipv4;
  inet_ntop(AF_INET, &ipv4->sin_addr, arguments->addressPrintable, INET_ADDRSTRLEN); // Convertit l'ip en texte
  freeaddrinfo(res);
  return (true);
}

bool parseArg(int argc, char **argv, struct arguments *arguments) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-?") == 0) {
      arguments->helpIsEnable = true;
      return (true);
    } else if (strcmp(argv[i], "-v") == 0) {
      arguments->verboseIsEnable = true;
    } else if (argv[i][0] == '-') {
      printf("ping: invalid option -- '%s'\nTry 'ping --help' or 'ping "
             "--usage' for more information.\n",
             argv[i]);
      return (false);
    } else {
      arguments->host = argv[i];
    }
  }
  if (arguments->host == NULL) {
    printf("ping: missing host operand\nTry 'ping --help' or 'ping --usage' "
           "for more information.\n");
    return (false);
  }
  if (!resolveHost(arguments)) {
    printf("ping: unknown host\n");
    return (false);
  }
  return (true);
}