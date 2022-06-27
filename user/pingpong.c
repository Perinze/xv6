#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid;
  int pc[2];
  int cp[2];
  char c;

  pipe(pc);
  pipe(cp);

  if(fork()){
    pid = getpid();
    close(pc[0]);
    close(cp[1]);

    write(pc[1], "", 1);
    read(cp[0], &c, 1);
    printf("%d: received pong\n", pid);
    write(pc[1], &c, 1);
  }else{
    pid = getpid();
    close(pc[1]);
    close(cp[0]);
    
    read(pc[0], &c, 1);
    printf("%d: received ping\n", pid);
    write(cp[1], &c, 1);
  }

  exit(0);
}
