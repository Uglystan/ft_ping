#include "main.h"
#include "printUtils.h"
#include "parse.h"
#include "ping.h"

int stop = 0;

void sigint_handler(int sig) {
  if (sig == SIGINT)
    stop = 1;
}

void setSignalAction(void) {
  struct sigaction act = {0};
  act.sa_handler = &sigint_handler;
  sigaction(SIGINT, &act, NULL);
}

int main(int argc, char **argv) {
  setSignalAction();
  struct arguments arguments = {0};

  if (parseArg(argc, argv, &arguments) == false)
    return (1);
  if (arguments.helpIsEnable)
    return (printHelp(), 1);
  ping(&arguments);
  return (0);
}