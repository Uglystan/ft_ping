#ifndef PARSE_H
# define PARSE_H

#include "main.h"

bool parseArg(int argc, char **argv, struct arguments *arguments);
bool resolveHost(struct arguments *arguments);

#endif
