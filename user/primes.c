#include "kernel/types.h"
#include "kernel/stat.h"
#include <user/user.h>

void func(int p[]){
    int p2[2];
    pipe(p2);
    int num;
    read(p[0], &num, 4);
    if(num == -1){
    	exit(0);
    }
    printf("prime %d\n", num);
    if(fork() == 0){
        close(p[1]);
	close(p[0]);
        func(p2);
    }else{
        close(p[1]);
	close(p2[0]);
        int receive_num = 0;
        while(read(p[0], &receive_num, 4)){
            if(receive_num % num != 0){
                write(p2[1], &receive_num, 4);
            }
        }
	close(p2[1]);
    }
}
int main(){
    int p[2];
    pipe(p);
    if(fork() == 0){
        func(p);
    }else{
	int num = 2;
        for(; num <= 35; ++num){
            write(p[1], &num, 4);
        }
	int eof = -1;
        write(p[1], &eof, 4);
	close(p[1]);
    }
    wait(0);
    exit(0);
}
