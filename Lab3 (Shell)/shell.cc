#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include "shell.hh"


int yyparse(void);

extern char ** environ;
std::string executablePath;
pid_t currentShellPID;
std::string strprompt = "myshell>";

void Shell::prompt() {
  if(isatty(0)) {
    if (getenv("PROMPT") != NULL) {
	strprompt = getenv("PROMPT");
    }
    fputs(strprompt.c_str(),stdout);
    fflush(stdout);
  }
}

extern "C" void disp( int sig ) {
  (void) sig;
  printf("\n");
  Shell::prompt();
}

extern "C" void killZombie(int sig) {
  (void) sig;
  pid_t pid;
  bool killedzombie = false;
  while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
    killedzombie = bPID.find(pid) != bPID.end();
    bPID.erase(pid);
    if (killedzombie) {
      printf("%d exited.\n", pid);
    }
  }
}

int main(int argc, char** argv) {
  (void) argc;
  executablePath = argv[0];
  currentShellPID = getpid();
  Shell::prompt();
  struct sigaction sa;
  struct sigaction zombiesa;
  sa.sa_handler = disp;
  zombiesa.sa_handler = killZombie;
  sigemptyset(&sa.sa_mask);
  sigemptyset(&zombiesa.sa_mask);
  sa.sa_flags = SA_RESTART;
  zombiesa.sa_flags = SA_RESTART;
  if (sigaction(SIGINT, &sa, NULL)) {
    perror("sigaction");
    exit(2);
  }
  if (sigaction(SIGCHLD, &zombiesa, NULL)) {
    perror("zombie sigaction");
    exit(2);
  }
  yyparse();
}

Command Shell::_currentCommand;
