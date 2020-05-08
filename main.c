#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 1024
#define MAX_COMMAND_LENGTH 4096

int getcom(char *command, int command_length, int buf_size) {
  int eoc = 1;
  char buf[buf_size];
  command[0] = '\0';

  do {
    fgets(buf, buf_size, stdin);
    if (buf[strlen(buf)-1] == '\n')
      buf[strlen(buf)-1] = '\0';

    if (buf[strlen(buf)-1] == '\\') {
      buf[strlen(buf)-1] = '\0';
    } else {
      eoc = 0;
    }
    strcat(command, buf);
  } while (eoc);

  return 0;
}

int parse();

int main(int argc, char *argv[]) {
  int run = 1, eoc = 1;
  char *command = (char *)malloc(MAX_COMMAND_LENGTH*sizeof(char));

  if (argc > 1) {
    //execute input command
  } else {
    // run shell
    while (run) {
      fputs("> ", stdout);
      fflush(stdout);
      getcom(command, MAX_COMMAND_LENGTH, BUF_SIZE);
      system(command);
    }
  }

  return 0;
}
