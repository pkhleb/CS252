
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>
#include <regex.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <algorithm>
#include <cstddef>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#define DEBUG false
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD QUOTEDWORD
%token QEXIT SUBSHELL CHDIR EXIT NOTOKEN GREAT NEWLINE PIPE LESS TWOGREAT GREATAMPERSAND GREATGREAT GREATGREATAMPERSAND AMPERSAND

%{
//#define yylex yylex
#include <cstdio>
#include <signal.h>
#include <cstdlib>
#include "shell.hh"

void yyerror(const char * s);
void expandWildcardsIfNecessary(std::string* arg);
bool foundMatch;
int yylex();

%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command: 
  pipe_list io_modifier_list background_opt NEWLINE {
    if (DEBUG) {
      printf("   Yacc: Execute command\n");
    }
    Shell::_currentCommand.execute();
  }
  | NEWLINE {
    if (DEBUG) {
      printf("   Yacc: Execute command\n");
    }
    Shell::_currentCommand.execute();
  } 
  | EXIT {
    Shell::_currentCommand.shexit();
  }
  | QEXIT {
    Shell::_currentCommand.qshexit();
  }
  | CHDIR {
    if (DEBUG) {
      printf("   Yacc: Execute command\n");
    }
    Shell::_currentCommand.goHome();
  }
  | CHDIR WORD {
    if (DEBUG) {
      printf("   Yacc: Execute command\n");
    }
    Shell::_currentCommand._newDir = $2;
    Shell::_currentCommand.changeDir();
  }
  | error NEWLINE { yyerrok; }
  ;

pipe_list: 
  command_and_args
  | pipe_list PIPE command_and_args
  ;

io_modifier_list:
  io_modifier_list io_modifier
  | io_modifier
  |
  ;

io_modifier:
  GREAT WORD {
    if (DEBUG) {
      printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    }
    if (!Shell::_currentCommand._outputRedirect) {
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._outputRedirect = true;
    }
    else {
      Shell::_currentCommand.multiRedirect();
    }
  }
  | LESS WORD {
    if (DEBUG) {
      printf("   Yacc: insert input \"%s\"\n", $2->c_str());
    }
    Shell::_currentCommand._inFile = $2;
  }
  | TWOGREAT WORD {
    if (DEBUG) {
      printf("   Yacc: redirect stderr to \"%s\"\n", $2->c_str());
    }
    if (!Shell::_currentCommand._errorRedirect) {
      Shell::_currentCommand._errFile = $2;
      Shell::_currentCommand._errorRedirect = true;
    }
    else {
      Shell::_currentCommand.multiRedirect();
    }
  }
  | GREATGREATAMPERSAND WORD {
    if (DEBUG) {
      printf("   Yacc: redirect and append stderr and output to \"%s\"\n", $2->c_str());
    }
    if (!Shell::_currentCommand._errorRedirect && !Shell::_currentCommand._outputRedirect) {
      Shell::_currentCommand._errFile = $2;
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._append = 1;
      Shell::_currentCommand._outputRedirect = true;
      Shell::_currentCommand._errorRedirect = true;
    }
    else {
      Shell::_currentCommand.multiRedirect();
    }
  }
  | GREATGREAT WORD {
    if (DEBUG) {
      printf("   Yacc: redirect and append output to \"%s\"\n", $2->c_str());
    }
    if (!Shell::_currentCommand._outputRedirect) {
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._append = 1;
      Shell::_currentCommand._outputRedirect = true;
    }
    else {
      Shell::_currentCommand.multiRedirect();
    }
  }
  | GREATAMPERSAND WORD {
    if (DEBUG) {
      printf("   Yacc: redirect stderr and output to \"%s\"\n", $2->c_str());
    }
    if (!Shell::_currentCommand._outputRedirect && !Shell::_currentCommand._errorRedirect) {
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._errFile = $2;
      Shell::_currentCommand._outputRedirect = true;
      Shell::_currentCommand._errorRedirect = true;
    }
    else {
      Shell::_currentCommand.multiRedirect();
    }
  }
  ;

background_opt:
  AMPERSAND {
    if (DEBUG) {
      printf("   Yacc: insert background amperstand\n");
    }
    Shell::_currentCommand._background = 1;
  }
  |
  ;

command_and_args:
  command_word argument_list {
    Shell::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
    if (DEBUG) {
      printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    }
    /*Command::_currentSimpleCommand->insertArgument( $1 );*/
    foundMatch = false;
    expandWildcardsIfNecessary($1);
    if (foundMatch) {
      delete $1;
    }
  }
  | QUOTEDWORD {
    if (DEBUG) {
      printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    }
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

command_word:
  WORD {
    if (DEBUG) {
      printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    }
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  | QUOTEDWORD {
    if (DEBUG) {
      printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    }
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

bool compareVal(std::string* s1, std::string* s2) { return (*s1 < *s2); }

void expandWildcardsIfNecessary(std::string* arg)
{
  //REDACTED
}

#if 0
main()
{
  yyparse();
}
#endif
