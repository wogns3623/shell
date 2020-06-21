#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

#define BUF_SIZE 4096
#define ARG_SIZE 1024
#define HIS_SIZE 1024

typedef enum {
    GET_COMMAND = 0,
    GET_SPECIAL_CHARACTER,
    GET_ARGUMENT,
    GET_INPUT_REDIRECTION,
    GET_OUTPUT_REDIRECTION,
    GET_PIPE
} parse_state;

typedef enum {
    DEFAULT = 0,
    CONTINUE,
    OVERWRITE
} redir_opt;

/**
 * 경로에 있는 프로그램 실행
 * 경로가 없다면 bin폴더에서 프로그램을 찾아봄
 * bin폴더에 없다면 시스템명령으로 처리함
 */
typedef struct {
    char *program;
    char *input_file_name;
    char *output_file_name;
    redir_opt output_opt;
    int pipefd[2];
    int amp; // 1이면 백그라운드에서 실행 == 기다리지 않음
    char *args[ARG_SIZE];
} command;

typedef struct {
    int count, in, out, cur;
    char queue[HIS_SIZE][BUF_SIZE];
    char current[BUF_SIZE];
} history;

history his;

// FILE *logfile;

void his_add(history his, char* comm) {
    strcpy(his.queue[his.in], comm);
    if (his.count == HIS_SIZE)
        his.out = (his.out+1)%HIS_SIZE;
    else 
        his.count++;

    his.in = (his.in+1)%HIS_SIZE;
}
void cur_right() {
    fputc(27, stdout);
    fputc(91, stdout);
    fputc(67, stdout);
}
void cur_left() {
    fputc(27, stdout);
    fputc(91, stdout);
    fputc(68, stdout);
}
int getcom(char *com_buf);
int parse(char *com_buf);
int execute(command com);

int main(int argc, char *argv[]) {
    int run = 1;
    char com_buf[BUF_SIZE];
    // logfile = fopen("log.txt", "w");

    if (argc == 1) { // run shell
        // setting terminal
        struct termios old, new;
        tcgetattr(0, &old);
        new = old;
        new.c_lflag &= ~ICANON & ~ECHO;
        tcsetattr(0, TCSANOW, &new);

        while (run) {
            fputs("> ", stdout);

            fflush(stdout);
            memset(com_buf, 0, sizeof(char)*BUF_SIZE);
            his.cur = his.count;

            getcom(com_buf);
            parse(com_buf);
        }
    } else { //execute input command
        // printf("|%s|\n", argv[1]);
        parse(argv[1]);
    }

    return 0;
}

int getcom(char *com_buf) {
    char buf;
    int buf_cur = 0, buf_len = 0;

    // 명령어 저장을 history[current_cursor]에 저장함
    while ((buf = fgetc(stdin)) !=  '\n') {
        if (buf_len < BUF_SIZE) {
        switch (buf) {
        case 127: // backspace
            if (buf_len > 0) {
                if (buf_cur == buf_len) {
                    com_buf[buf_cur--] = '\0';
                    buf_len--;
                    cur_left();
                    fputc(' ', stdout);
                    cur_left();
                } else {
                    char tmp[buf_len-buf_cur];
                    strcpy(tmp, com_buf+buf_cur);
                    buf_cur--;
                    buf_len--;
                    strcpy(com_buf+buf_cur, tmp);

                    cur_left();
                    fputs(tmp, stdout);
                    fputc(' ', stdout);
                    for (int i = 0; i < buf_len-buf_cur+1; i++)
                        cur_left();
                }
            }
            break;

        case 27: // special input
            if ((buf = fgetc(stdin)) == 91) {
                buf = fgetc(stdin);
                switch (buf) {
                // 이전 명령어를 불러옴
                // history배열에서 current로 불러옴
                case 65: // up
                    break;
                case 66: // down
                    break;
                
                // 줄의 임의 위치에 커서를 옮길 수 있어야 함
                // 버퍼의 처음부터 끝까지만 가능
                // 현재 버퍼가 차있는 만큼만 움직일 수 있음
                // 버퍼의 커서도 같이 움직임
                case 67: // right
                    if (buf_cur < buf_len) {
                        cur_right();
                        buf_cur++;
                    }
                    break;
                case 68: // left
                    if (buf_cur > 0) {
                        cur_left();
                        buf_cur--;
                    }
                    break;
                case 70: // end
                    while (buf_cur < buf_len) {
                        cur_right();
                        buf_cur++;
                    }
                    break;
                case 72: // home
                    while (buf_cur > 0) {
                        cur_left();
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
                    cur_left();
            }
            break;
        }
        }
    }
    fputc('\n', stdout);
    his_add(his, com_buf);

    return buf_len;
}

// 명령어가 남아있으면 1을 리턴
// 문자열을 끝까지 처리했으면 0을 리턴
int parse(char *com_buf) {
    int eoc = 1;
    int retval = 0;
    int buf_cur = 0;
    int arg_cur = 0;
    int start_cur = buf_cur;
    char **save_location;
    parse_state state = GET_COMMAND;

    command *com = malloc(sizeof(command));
    memset(com, 0, sizeof(command));
    
    while (eoc) {
        switch (state) {
        case GET_SPECIAL_CHARACTER:

            if (com_buf[buf_cur] == '\0') // buffer end
                eoc = 0;

            if (com_buf[buf_cur] == ' ' || com_buf[buf_cur] == ';' ||
            com_buf[buf_cur] == '&' || com_buf[buf_cur] == '<' ||
            com_buf[buf_cur] == '>' || com_buf[buf_cur] == '|') {
                break;
            } else { // non-special character
                char *sc = malloc(sizeof(char)*(buf_cur-start_cur));
                strncpy(sc, com_buf+start_cur, buf_cur-start_cur);
                sc = strtok(sc, " ");
                
                start_cur = buf_cur;
                if (sc == NULL) { // ' '
                    state = GET_ARGUMENT;
                } else if (strcmp(sc, ">") == 0) { // redirection
                    state = GET_OUTPUT_REDIRECTION;
                    com->output_opt = DEFAULT;
                } else if (strcmp(sc, ">>") == 0) { // redirection(이어쓰기)
                    state = GET_OUTPUT_REDIRECTION;
                    com->output_opt = CONTINUE;
                } else if (strcmp(sc, ">|") == 0) { // redirection(force overwrite)
                    state = GET_OUTPUT_REDIRECTION;
                    com->output_opt = OVERWRITE;
                } else if (strcmp(sc, "<") == 0) { // input redirection
                    state = GET_INPUT_REDIRECTION;
                } else if (strcmp(sc, "|") == 0) { // pipe
                    state = GET_PIPE;
                } else if (strcmp(sc, "&") == 0) { // background실행
                    com->amp = 1;
                    execute(*com);
                    state = GET_COMMAND;
                } else if (strcmp(sc, ";") == 0) { // 그냥 실행
                    execute(*com);
                    state = GET_COMMAND;
                }
                continue;
            }

        case GET_PIPE: // current output is next command's input
            if (pipe(com->pipefd) < 0) {
                perror("pipe error\n");
                exit(1);
            }
        case GET_COMMAND:
            save_location = &com->program;
            break;

        case GET_ARGUMENT:
            save_location = &com->args[arg_cur];
            break;

        case GET_INPUT_REDIRECTION:
            save_location = &com->input_file_name;
            break;

        case GET_OUTPUT_REDIRECTION:
            save_location = &com->output_file_name;
            break;
        }
        if (state != GET_SPECIAL_CHARACTER) {
            if (com_buf[buf_cur] == '\0' || com_buf[buf_cur] == ' ' ||
            com_buf[buf_cur] == ';' || com_buf[buf_cur] == '&' ||
            com_buf[buf_cur] == '<' || com_buf[buf_cur] == '>' ||
            com_buf[buf_cur] == '|') {
                *save_location = malloc(sizeof(char)*(buf_cur-start_cur));
                strncpy(*save_location, com_buf+start_cur, buf_cur-start_cur);
                if (state == GET_COMMAND) {
                    save_location = &com->args[arg_cur];
                    *save_location = malloc(sizeof(char)*(buf_cur-start_cur));
                    strncpy(*save_location, com_buf+start_cur, buf_cur-start_cur);
                    arg_cur++;
                } else  if (state == GET_ARGUMENT) arg_cur++;

                state = GET_SPECIAL_CHARACTER;
                start_cur = buf_cur;
                buf_cur--;
            }
        }

        buf_cur++;
    }

    return retval;
}

int execute(command com) {
    pid_t pid;

    printf("amp : |%d|\n", com.amp);
    printf("input_file_name : |%s|\n", com.input_file_name);
    printf("output_opt : |%d|\n", com.output_opt);
    printf("output_file_name : |%s|\n", com.output_file_name);
    printf("program : |%s|\n", com.program);
    
    for (int i=0; com.args[i] != (char*)NULL; i++)
        printf("argment%d : |%s|\n", i, com.args[i]);

    // if (strcmp(com.program, "exit") == 0) {
    // } else if (strcmp(com.program, "cd") == 0) {
    // } else if (strcmp(com.program, "history") == 0) {
    // } else if (com.program[0] == '!') { // !
    // } else if (strcmp(com.program, "set") == 0) { // set [+|-]o noclobber
    // }

    // if((pid = fork()) < 0) {
    //     perror("fork error\n");
    //     return -1;
    // } else if (pid > 0) { // parent
        
    // } else { // child
    //     if (com.input_file_name != NULL) {
    //         int inputfd = open(com.input_file_name, O_RDONLY);
    //         if (inputfd == -1) {
    //             perror("input file not exist");
    //             return -1;
    //         } else {

    //         }

    //     }
    //     execvp(com.program, com.args);
    // }
    
    return 0;
}