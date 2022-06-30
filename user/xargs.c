#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

int fork1(void);  // Fork but panics on failure.
void panic(char*);

int
getline(char *buf, int nbuf)
{
  char c;
  int i;
  memset(buf, 0, nbuf);
  if(read(0, &c, 1) && c == 0)
    return -1;

  i = 0;
  do {
    buf[i++] = c;
    if(i == nbuf)
      return 0;
    read(0, &c, 1);
  } while(c != '\n');
  buf[i] = 0;
  return 0;
}

int
main(int argc, char *argv[])
{
  static char buf[100];
  char *xargs[MAXARG];
  int fd;
  int i;

  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  for(i=0; i < argc-1; i++){
    xargs[i] = malloc(100);
    strcpy(xargs[i], argv[i+1]);
  }
  xargs[argc-1] = malloc(100);
  xargs[argc] = 0;

  // Read and run input commands.
  while(getline(buf, sizeof(buf)) >= 0){
    strcpy(xargs[argc-1], buf);

    if(fork1() == 0)
      exec(xargs[0], xargs);
    wait(0);
  }
  for(i=0; i < argc; i++)
    free(xargs[i]);
  exit(0);
}

void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(1);
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}
