/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);
extern void tty_term_mode(void);

// Buffer where line is stored
int line_length;
int cursorLocation;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.

char ** cmdhistory;
int historySize = 0;
int history_index = 0;
char * history [] = {
  "ls -al | grep x", 
  "ps -e",
  "cat read-line-example.c",
  "vi hello.c",
  "make",
  "ls -al | grep xxx | grep yyy"
};
int history_length = sizeof(history)/sizeof(char *);

void read_line_print_usage()
{
  char* usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}
void moveCursorRight() {
	char ch;
	if (cursorLocation < line_length) {
	  ch = line_buffer[cursorLocation];
	  cursorLocation++;
	  write(1,&ch,1);
	}
}

void writeCharacter(char ch) {
      if (cursorLocation < line_length) {
	for (int i = line_length; i > cursorLocation; i--) {
	  line_buffer[i] = line_buffer[i-1];
	}
	for (int i = cursorLocation+1; i < line_length+1; i++) {
          char ch0;
	  ch0 = line_buffer[i];
	  write(1,&ch0,1);
	}
	for (int i = 0; i < line_length-cursorLocation; i++) {
	  char ch0;
	  ch0 = 8;
	  write(1,&ch0,1);
	}
      }
      line_buffer[cursorLocation]=ch;
      line_length++;
      cursorLocation++;
}

/* 
 * Input a line with some basic editing.
 */
int searching = 0;
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  cursorLocation = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32 && ch <= 126) {
      // It is a printable character. 

      // Do echo
      write(1,&ch,1);

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break; 

      // add char to buffer.
      writeCharacter(ch);
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      cmdhistory = (char**) realloc(cmdhistory, (++historySize)*sizeof(char*));
      cmdhistory[historySize-1] = (char*) malloc((line_length+1)*sizeof(char));
      memcpy(cmdhistory[historySize-1],line_buffer,line_length);
      cmdhistory[historySize-1][line_length] = '\0';
      history_index = historySize-1;
      searching = 0;
      
      // Print newline
      write(1,&ch,1);

      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 8) {
      // <backspace> was typed. Remove previous character read.

      // Go back one character
      if (cursorLocation != 0) {
        ch = 8;
        write(1,&ch,1);

        // Write a space to erase the last character read
        /*ch = ' ';
        write(1,&ch,1);

        // Go back one character
        ch = 8;
        write(1,&ch,1);

        // Remove one character from buffer
        line_length--;
        cursorLocation--;*/
	cursorLocation--;
        for (int i = cursorLocation; i < line_length; i++) {
 	  line_buffer[i] = line_buffer[i+1];
	  write(1,&line_buffer[i],1);
        }
        ch = 8;
        for (int i = cursorLocation; i < line_length; i++) {
	  write(1,&ch,1);
        }
	line_length--;
      }
    }
    else if (ch==4) {
      for (int i = cursorLocation; i < line_length; i++) {
	line_buffer[i] = line_buffer[i+1];
	write(1,&line_buffer[i],1);
      }
      ch = 8;
      for (int i = cursorLocation; i < line_length; i++) {
	write(1,&ch,1);
      }
      if (line_length != cursorLocation) {
        line_length--;
      }
    }
    else if (ch==1) {
	ch = 8;
	for (int i = 0; i < line_length; i++) {
	  write(1,&ch,1);
	}
	cursorLocation = 0;
    }
    else if (ch==5) {
	for (int i = cursorLocation; i < line_length; i++) {
	  moveCursorRight();
	}
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
	// Up arrow. Print next line in history.

	// Erase old line
	// Print backspaces
	int i = 0;
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}

	// Print spaces on top
	for (i =0; i < line_length; i++) {
	  ch = ' ';
	  write(1,&ch,1);
	}

	// Print backspaces
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}	

	//printf("up history_index 1: %d\n", history_index);

	// Copy line from history
	if (history_index >= 0 && historySize != 0) {
	  if (history_index != 0 && searching) {
	    history_index--;
	  }
	  searching = 1;
	  //printf("up history_index 2: %d\n", history_index);
	  strcpy(line_buffer, cmdhistory[history_index]);
	  line_length = strlen(line_buffer);
	  /*if (history_index != 0) {
	    history_index--;
	  }*/
	}
	//printf("up history_index 3: %d\n", history_index);
	cursorLocation = line_length;
	
	// echo line
	write(1, line_buffer, line_length);
      }
      else if (ch1==91 && ch2==66) {
	// Down arrow
	// Erase old line
	// Print backspaces
	int i = 0;
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}

	// Print spaces on top
	for (i =0; i < line_length; i++) {
	  ch = ' ';
	  write(1,&ch,1);
	}

	// Print backspaces
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}	

	// Copy line from history
	//printf("down history_index1: %d\n", history_index);
	if (history_index <= historySize) {
	  history_index++;
	  //printf("down history_index2: %d\n", history_index);
	  if (history_index <= historySize-1) {
	    strcpy(line_buffer, cmdhistory[history_index]);
	    line_length = strlen(line_buffer);
	  } else {
	    for (i = 0; i < line_length; i++) {
	      line_buffer[i] = '\0';
	    }
	    history_index =  historySize-1;
	    cursorLocation = line_length;
	    for (i =0; i < line_length; i++) {
	      ch = ' ';
	      write(1,&ch,1);
	    }
	    for (i =0; i < line_length; i++) {
	      ch = 8;
	      write(1,&ch,1);
	    }
	    searching = 0;
	    line_length = 0;
	  }
	  //printf("down history_index3: %d\n", history_index);
	}
	    cursorLocation = line_length;
	
	// echo line
	write(1, line_buffer, line_length);
      }
      else if (ch1==91 && ch2==68) {
	// Left arrow
	if (cursorLocation != 0) {
          ch = 8;
	  cursorLocation--;
	  write(1,&ch,1);
	}
      }
      else if (ch1==91 && ch2==67) {
	// Right arrow
	if (cursorLocation < line_length) {
	  ch = line_buffer[cursorLocation];
	  cursorLocation++;
	  write(1,&ch,1);
	}
      }
    }

  }
    tty_term_mode();

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  return line_buffer;
}

