#define MAX_NAME_LEN 32
#define MAX_BUFF_LEN 512
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char* argv[]){

    char name[MAX_NAME_LEN];
    char* new_argv[MAXARG];
    memcpy(name, argv[1], sizeof(argv[1]));

    new_argv[0] = malloc(sizeof(argv[1]) + 1);
    memcpy(new_argv[0], argv[1], sizeof(argv[1]));
    new_argv[0][sizeof(argv[1])] = '\0';
    int arg_count = 1;
    // printf("argc is %d\n",argc);
    for(int i = 2; i < argc; ++i){
        new_argv[arg_count] = malloc(sizeof(argv[i]) + 1);
        memcpy(new_argv[arg_count], argv[i], sizeof(argv[i]));
        new_argv[arg_count][sizeof(argv[i])] = '\0';
        arg_count++;
    }
    char buff[MAX_BUFF_LEN];
    int buf_len = 0;
    while ((buf_len = read(0, buff, sizeof(buff))) > 0)
    {
    //    for(int i = 0; i < buf_len; ++i){
    //     printf("%c\n",buff[i]);
    //    }
       int pre_index = 0; int index = 0;
       while(index < buf_len && buff[index] == ' '){
        index++;
       } 
       pre_index = index;
       while(index < buf_len){
        while(index < buf_len && buff[index] != ' ') index++;
        new_argv[arg_count] = malloc(index - pre_index + 1);
        memcpy(new_argv[arg_count], &(buff[pre_index]), index - pre_index);
        new_argv[arg_count][index - pre_index] = '\0';
        arg_count++;
        while(index < buf_len && buff[index] == ' ')index++;
        pre_index = index;  
       }
    }
    for(int i = 0; i < arg_count; ++i){
        printf("%s\n", new_argv[i]);
    }
    if(fork() == 0){
        exec(name, new_argv);
        fprintf(2, "xargs: exec failed\n");
        exit(1);
    }
    wait(0);
    exit(0);
}
