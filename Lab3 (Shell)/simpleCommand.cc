#include <cstdio>
#include <cstdlib>
#include <limits.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <pwd.h>

#include "simpleCommand.hh"

SimpleCommand::SimpleCommand() {
  _arguments = std::vector<std::string *>();
}

SimpleCommand::~SimpleCommand() {
  // iterate over all the arguments and delete them
  //REDACTED
}

void SimpleCommand::insertArgument( std::string * argument ) {
  variableExpand(argument);
  
  _arguments.push_back(argument);
}

// Print out the simple command
void SimpleCommand::print() {
  for (auto & arg : _arguments) {
    std::cout << "\"" << arg->c_str() << "\" \t";
  }
  // effectively the same as printf("\n\n");
  std::cout << std::endl;
}
