#include "kernel/types.h"
#include "user/user.h"

void
cull(int p, int pip0, int pip1)
{
  int x;
  while(read(pip0, &x, sizeof(x)))
    if(x % p)
      write(pip1, &x, sizeof(x));
}

void
right(int pip0)
{
  int x;
  int p;
  int pip[2];
  int pid;

  if(!read(pip0, &p, sizeof(p))){
    close(pip0);
    return;
  }
  printf("prime %d\n", p);

  pipe(pip);
  if(fork()){
    close(pip[0]);
    cull(p, pip0, pip[1]);
    close(pip[1]);

    do {
      pid = wait(0);
    } while(pid != -1);
  } else {
    close(pip[1]);
    right(pip[0]);
  }
  close(pip0);
}

int
main(int argc, char *argv[])
{
  int pip[2];
  int i;
  int pid;

  pipe(pip);

  if(fork()){
    close(pip[0]);

    for(i = 2; i <= 35; i++)
      write(pip[1], &i, sizeof(i));
    close(pip[1]);

    do {
      pid = wait(0);
    } while(pid != -1);

  } else {
    close(pip[1]);
    right(pip[0]);
  }

  exit(0);
}
