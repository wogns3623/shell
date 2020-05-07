#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#define BUF_SIZE 100 /* The maximum length command */

int main(int argc, char *argv[]) {
  int run = 1, eoc = 1;
  char *buf = (char *)malloc(BUF_SIZE*sizeof(char));

  if (argc > 1) {
    //execute input command
  } else {
    // run shell
    while (run) {
      fputs("> ", stdout);
      fflush(stdout);
      fgets(buf, BUF_SIZE, stdin);
      buf[strlen(buf)-1] = '\0';
      system(buf);
    }
  }

  return 0;
}
