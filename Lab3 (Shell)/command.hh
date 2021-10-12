#ifndef command_hh
#define command_hh

#include "simpleCommand.hh"

// Command Data Structure

struct Command {
  std::vector<SimpleCommand *> _simpleCommands;
  std::string * _outFile;
  std::string * _inFile;
  std::string * _errFile;
  bool _background;
  bool _append;
  bool _outputRedirect;
  bool _errorRedirect;
  bool _multiRedirect;
  std::string * _newDir;

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void shexit();
  void qshexit();
  void shint();
  void clear();
  void print();
  void execute();
  void multiRedirect();
  void changeDir();
  void goHome();

  void setenvvar(int i);
  void unsetenvvar(int i);

  static SimpleCommand *_currentSimpleCommand;
};

#endif
