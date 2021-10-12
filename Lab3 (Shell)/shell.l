
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%x src

%{

#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include "y.tab.hh"

extern "C" char* read_line();

int mygetc(FILE * f) {
  static char *p;
  char ch;
  
  if (!isatty(0)) {
    return getc(f);
  }

  if (p == NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

#define MAX_DEPTH 10
YY_BUFFER_STATE source_stack[MAX_DEPTH];
int source_stack_ptr = 0;

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%%

//REDACTED

"cd" {
  return CHDIR;
}

source		BEGIN(src);

<src>[ \t]* /*eat the whitespace*/
<src>[^ \t\n]+ {/*got the filename*/
  if (source_stack_ptr >= MAX_DEPTH) {
    fprintf( stderr, "Too much source");
    exit(1);
  }
  
  source_stack[source_stack_ptr++] = YY_CURRENT_BUFFER;
  
  yyin = fopen( yytext, "r" );

  yy_switch_to_buffer( yy_create_buffer( yyin, YY_BUF_SIZE ) );
  BEGIN(INITIAL);
}

<<EOF>> {
  if ( --source_stack_ptr < 0 ) {
    yyterminate();
  } else {
    yy_switch_to_buffer( source_stack[source_stack_ptr] );
  }
}

(\\\".+\\\") {
  yylval.cpp_string = new std::string(yytext);
  yylval.cpp_string->erase(0,1);
  yylval.cpp_string->erase(yylval.cpp_string->size()-2,1);
  return QUOTEDWORD;
}

(\".+\") {
  yylval.cpp_string = new std::string(yytext);
  yylval.cpp_string->erase(0,1);
  yylval.cpp_string->erase(yylval.cpp_string->size()-1);
  return WORD;
}

([^ \t\n]*\\[^ \t\n]+) {
  yylval.cpp_string = new std::string(yytext);
  int pos;
  while ((pos = yylval.cpp_string->find("\\")) != std::string::npos && 
          pos != yylval.cpp_string->size()-1) {
    yylval.cpp_string->erase(pos,1);
  }
  return WORD;
}

"exit" {
  return EXIT;
}

"qexit" {
  return QEXIT;
}

"&" {
  return AMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

">>" {
  return GREATGREAT;
}

">&" {
  return GREATAMPERSAND;
}

"2>" {
  return TWOGREAT;
}

"<" {
  return LESS;
}

"|" {
  return PIPE;
}

\n {
  return NEWLINE;
}

";" {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

[^ \t\n\|<>&'>>''<<''>&'][^ \t\n\|<>&'>>''<<''>&']*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}
