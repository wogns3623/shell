#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

#define BUF_SIZE 1024
#define ARG_SIZE 1024
#define HIS_SIZE 1024

typedef enum {
    GET_COMMAND = 0,
    GET_ARGUMENT
} parse_state;

// 경로에 있는 프로그램 실행
// 경로가 없다면 bin폴더에서 프로그램을 찾아봄
// bin폴더에 없다면 시스템명령으로 처리함
typedef struct {
    char *program;
    char *path;
    char *args[1024];
} command;

typedef struct {
    int count, in, out, cursor;
    char queue[HIS_SIZE][BUF_SIZE];
    char modified_queue[HIS_SIZE][BUF_SIZE];
    char current[BUF_SIZE];
} history;

char com_buf[BUF_SIZE];
history his;

// FILE *logfile;

void his_set(history his) {
    memset(his.current, 0, sizeof(char)*BUF_SIZE);
    his.cursor = his.count;
}

/**
 * add string to history queue
 */
void his_add(history his, char* comm) {
    his.queue[his.in] = comm;
    if (his.count == HIS_SIZE)
        his.out = (his.out+1)%HIS_SIZE;
    else 
        his.count++;

    his.in = (his.in+1)%HIS_SIZE;
}

/**
 * 0부터 his.count까지를 내부적으로 history.queue의 처음부터 끝까지 매핑시켜준다
 * his_at(0) = history.queue[his.out]
 * his_at(his.count-1) = history.queue[his.in-1]
 * his_at(his.count) = history.current;
 */
char *his_at(char** array, int location) {
    if (location > his.count) { // out of index error
        perror("OUT OF INDEX ERROR");
        exit(1);
    } else if (location == his.count) {
        return his.current;
    } else { // 0~his.count-1;
        return array[(his.out+location)%HIS_SIZE];
    }
}

void his_cursor_up(history his) {
    if (his.cursor > 0) {
        // 만약 history.modified_queue[history.cur-1]이 NULL이라면 history.queue[history.cur-1]에서 복사해서 넣음
        if (his_at(his.modified_queue, his.cursor-1) == NULL) {
            strcpy(his_at(his.modified_queue, his.cursor-1), his_at(his.queue, his.cursor-1));
        }
        his.cursor--;
    }
}

void his_cursor_down(history his) {
    if (his.cursor < his.count) {
        his.cursor++;
    }
}

char *his_current(history his) {
    if (his.cursor == his.count) {
        return his.current;
    else { // 0 <= his.cursor < his.count
        return his.modified[his.cursor];
    }
}

void cursor_right() {
    fputc(27, stdout);
    fputc(91, stdout);
    fputc(67, stdout);
}
void cursor_left() {
    fputc(27, stdout);
    fputc(91, stdout);
    fputc(68, stdout);
}
int getcom();
int parse(int com_len);
int execute();

int main(int argc, char *argv[]) {
    int run = 1, com_len;
    // logfile = fopen("log.txt", "w");

    // setting terminal
    struct termios old, new;
    tcgetattr(0, &old);
    new = old;
    new.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(0, TCSANOW, &new);

    if (argc == 1) { // run shell
        while (run) {
            fputs("> ", stdout);

            fflush(stdout);
            memset(com_buf, 0, BUF_SIZE);
            his_set(his);

            com_len = getcom();
            // parse(com_len);
            // execute();

            // system(command);
        }
    } else { //execute input command

    }

    return 0;
}

int getcom() {
    char buf;
    int buf_cur = 0, buf_len = 0;

    // 명령어 저장을 his_current(his)에 저장함
    while ((buf = fgetc(stdin)) !=  '\n') {
        switch (buf) {
        case 127: // backspace
            if (buf_len > 0) {
                if (buf_cur == buf_len) {
                    com_buf[buf_cur--] = '\0';
                    buf_len--;
                    cursor_left();
                    fputc(' ', stdout);
                    cursor_left();
                } else {
                    char tmp[buf_len-buf_cur];
                    strcpy(tmp, com_buf+buf_cur);
                    buf_cur--;
                    buf_len--;
                    strcpy(com_buf+buf_cur, tmp);

                    cursor_left();
                    fputs(tmp, stdout);
                    fputc(' ', stdout);
                    for (int i = 0; i < buf_len-buf_cur+1; i++)
                        cursor_left();
                }
            }
            break;

        case 27: // special input
            if ((buf = fgetc(stdin)) == 91) {
                buf = fgetc(stdin);
                switch (buf) {
                // 이전 명령어를 불러옴
                case 65: // up
                    his_cursor_up(his);
                    break;
                case 66: // down
                    his_cursor_down(his);
                    break;
                
                // 줄의 임의 위치에 커서를 옮길 수 있어야 함
                // 버퍼의 처음부터 끝까지만 가능
                // 현재 버퍼가 차있는 만큼만 움직일 수 있음
                // 버퍼의 커서도 같이 움직임
                case 67: // right
                    if (buf_cur < buf_len) {
                        cursor_right();
                        buf_cur++;
                    }
                    break;
                case 68: // left
                    if (buf_cur > 0) {
                        cursor_left();
                        buf_cur--;
                    }
                    break;
                case 70: // end
                    while (buf_cur < buf_len) {
                        cursor_right();
                        buf_cur++;
                    }
                    break;
                case 72: // home
                    while (buf_cur > 0) {
                        cursor_left();
                        buf_cur--;
                    }
                    break;
                }
                break;
            }

        default:
            // 버퍼가 꽉 찼을 때 예외처리 해줘야함
            if (buf_cur == buf_len) { // 커서가 마지막에 위치함
                com_buf[buf_cur++] = buf;
                buf_len++;

                fputc(buf, stdout);
            } else { // 커서가 문자열 사이에 위치함
                char tmp[buf_len-buf_cur];
                strcpy(tmp, com_buf+buf_cur);
                com_buf[buf_cur++] = buf;
                buf_len++;
                strcpy(com_buf+buf_cur, tmp);

                fputc(buf, stdout);
                fputs(tmp, stdout);

                for (int i = 0; i < buf_len-buf_cur; i++)
                    cursor_left();
            }
            break;
        }
    }
    fputc('\n', stdout);

    his_add(his, com_buf);

    return buf_len;
}

int parse(int com_len) {
    int cur = 0;
    parse_state state = GET_COMMAND;

    while (cur < com_len) {
        switch (state) {
        case GET_COMMAND:
            if (com_buf[cur] == ' ') {

            }
            break;

        case GET_ARGUMENT:
            break;

        case ';': // end of command
            break;

        case '&': // end of command
            break;

        case '>':
            break;

        case '<':
            break;

        case '|':
            break;

        
        default:
            break;
        }
        cur++;
    }
}