#include "history.h"

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
        modified
    }
}