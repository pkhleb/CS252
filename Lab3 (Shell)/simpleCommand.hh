#ifndef simplcommand_hh
#define simplecommand_hh

#include <string>
#include <vector>

extern std::string executablePath;
extern pid_t currentShellPID;
extern int lastChildStatus;
extern pid_t lastBackgroundPID;
extern std::string lastArg;

struct SimpleCommand {

  // Simple command is simply a vector of strings
  std::vector<std::string *> _arguments;

  SimpleCommand();
  ~SimpleCommand();
  void insertArgument( std::string * argument );
  void print();
};

#endif
