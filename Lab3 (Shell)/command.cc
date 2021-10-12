/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <termios.h>

#include <iostream>

#include "command.hh"
#include "shell.hh"


Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }
    bool samefd = false;
    if (_outFile == _errFile) samefd = true;

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _outFile ) {
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile && !samefd) {
        delete _errFile;
    }
    _errFile = NULL;

    //REDACTED
}

void Command::multiRedirect() {
    printf("Ambiguous output redirect.\n");
    _multiRedirect = true;
}

void Command::shexit() {
    printf("\n Good bye!!\n\n");
    tty_term_mode();
    exit(0);
}

void Command::qshexit() {
    tty_term_mode();
    exit(0);
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

void Command::setenvvar(int i) {
	  const char* env1 = _simpleCommands[i]->_arguments[1]->c_str();
	  if (getenv(env1)) {
	    unsetenv(env1);
	  }
	  const char* env2 = _simpleCommands[i]->_arguments[2]->c_str();
	  char* putstring = new char[strlen(env1)+strlen(env2)+1];
	  //REDACTED
}

void Command::unsetenvvar(int i) {
	  const char* env1 = _simpleCommands[i]->_arguments[1]->c_str();
	  if (getenv(env1)) {
	    unsetenv(env1);
	  }
}

void Command::goHome() {
  chdir(getenv("HOME"));
}

void Command::changeDir() {
  variableExpand(_newDir);
  if (chdir((char*) _newDir->c_str())) {
    fprintf(stderr, "cd: can't cd to %s\n", _newDir->c_str());
  }
  clear();
}

void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0) {
        Shell::prompt();
        return;
    }
    if (_multiRedirect) {
	clear();
	Shell::prompt();
	return;
    }


    //save in/out
    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperr = dup(2);

    int fdin, fdout, fderr;

    //set the initial input
    if (_inFile) {
	fdin = open(_inFile->c_str(), O_RDONLY);
    } else {
	//Use default input
	fdin = dup(tmpin);
    }

    // Print contents of Command data structure
    //print();

    pid_t ret;
    if (_errFile) {
	if (_append){
	  fderr = open(_errFile->c_str(), O_CREAT|O_APPEND|O_WRONLY,0664);
	} else {
	  fderr = open(_errFile->c_str(), O_CREAT|O_WRONLY|O_TRUNC,0664);
	}
    } else {
	fderr = dup(tmperr);
	close(fderr);
    }	
    // For every simple command fork a new process
    int size = _simpleCommands.size();

    int argsize2 = _simpleCommands[size-1]->_arguments.size();
    lastArg = _simpleCommands[size-1]->_arguments[argsize2-1]->c_str();
    for (int i = 0; i < size; i++) {
	//redirect input
	//REDACTED

	if (i == size-1) {
		// Last simple command
		if (_outFile) {
			if (_append) {
			  fdout = open(_outFile->c_str(),O_CREAT|O_APPEND|O_WRONLY,0664);
			} else {
			  fdout = open(_outFile->c_str(),O_CREAT|O_WRONLY|O_TRUNC,
					0664);
			}
		} else {
			// Use default output
			//REDACTED
		}
	} else {
		// Not last simple command create pipe
		//REDACTED
	}

	//Redirect output
	//REDACTED

	//create child process
	
	const char * command = _simpleCommands[i]->_arguments[0]->c_str();

	if ( !strcmp( command, "setenv") ) {
		setenvvar(i);
	} 
	else if (!strcmp(command,"unsetenv")) {
		unsetenvvar(i);
	} 
	else {
    	  ret = fork();
	  if (_background) {
	  	bPID.insert(ret);
	  }
    	  if (ret == 0) {
		if (!strcmp(command,"printenv")) {
		  char **p = environ;
		  while (*p != NULL) {
		    printf("%s\n", *p);
		    p++;
		  }
		  exit(0);
		}
		else {
	  	  int argsize = _simpleCommands[i]->_arguments.size();
	  	  char ** cArgs = new char*[argsize+1];
	  	  //REDACTED
	  	  execvp(command, cArgs);
	  	  perror("execvp");
	  	  exit(1);
		}
    	  }
	  else if (ret < 0) {
	  	perror("fork");
	  	return;
    	  }
	}
    }
    if (!_background) {
    	waitpid(ret, &lastChildStatus, 0);
	lastChildStatus = WEXITSTATUS(lastChildStatus);
	if (lastChildStatus != 0) {
	  if (getenv("ON_ERROR") != NULL) {
	    fprintf(stderr,"%s\n",getenv("ON_ERROR"));
	  }
	}
    } else {
	lastBackgroundPID = ret;
    }

    //restore in/out defaults
    dup2(tmpin,0);
    dup2(tmpout,1);
    dup2(tmperr,2);
    close(tmpin);
    close(tmpout);
    close(tmperr);


    // Clear to prepare for next command
    clear();

    // Print new prompt
    Shell::prompt();
}

SimpleCommand * Command::_currentSimpleCommand;
