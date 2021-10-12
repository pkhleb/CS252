#ifndef shell_hh
#define shell_hh

#include "command.hh"
#include <set>

static std::set<pid_t> bPID;

struct Shell {

  static void prompt();

  static Command _currentCommand;
};

#endif
