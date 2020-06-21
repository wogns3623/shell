#include "shell.h"

#define HIS_SIZE 1024

typedef struct {
    int count, in, out, cursor;
    char queue[HIS_SIZE][BUF_SIZE];
    char modified_queue[HIS_SIZE][BUF_SIZE];
    char current[BUF_SIZE];
} history;

void his_set(history his);
void his_add(history his, char* comm);
char *his_at(char** array, int location);
void his_cursor_up(history his);
void his_cursor_down(history his);
char *his_current(history his);