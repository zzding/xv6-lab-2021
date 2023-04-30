#include "kernel/types.h"
#include "kernel/stat.h"
#include <user/user.h>
int main(){
    int pid = fork();
    int p[2];
    pipe(p);
    int p2[2];
    pipe(p2);
    if(pid == 0){
        close(0);
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        char buf[512];
        read(0, buf, (int)sizeof(buf));
        printf("%d: received ping\n", getpid());
        
        close(p2[0]);
        write(p2[1], "ping\n", 5);
        close(p2[1]);
    }else{
        close(p[0]);
        write(p[1], "ping\n", 5);
        close(p[1]);

	wait(0);
        close(0);
        char buf[512];
        close(p2[0]);
        close(p2[1]);
        dup(p2[0]);
        read(0, buf, (int)sizeof(buf));
        printf("%d: received pong\n", getpid());
    }
    exit(0);
}
