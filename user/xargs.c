#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

char delim;

int fork1(void);  // Fork but panics on failure.
void panic(char*);

int
getline(char *buf, int nbuf)
{
  char c;
  int i;
  memset(buf, 0, nbuf);
  if(!read(0, &c, 1))
    return -1;

  i = 0;
  do {
    buf[i++] = c;
    if(i == nbuf-1)
      goto ret;
    if(!read(0, &c, 1))
      goto ret;
  } while(c != '\n');
ret:
  buf[i] = 0;
  return 0;
}

int
parseargs(char *argv[], int n, char *buf)
{
  int state;
  int i;
  char *p;

  state = 0;
  i = 0;
  for(p=buf; *p != 0; p++){
    if(state == 0 && *p != delim){
      buf = p;
      state = 1;
    } else if(state == 1 && *p == delim){
      argv[i] = malloc(100);
      memset(argv[i], 0, sizeof(argv[i]));
      memcpy(argv[i], buf, p-buf);
      if(i++ == n-1)
        goto ret;
      state = 0;
    }
  }
  if(state == 1){
    argv[i] = malloc(100);
    memset(argv[i], 0, sizeof(argv[i]));
    memcpy(argv[i], buf, p-buf);
    i++;
  }
ret:
  argv[i] = 0;
  return i;
}

int
main(int argc, char *argv[])
{
  static char buf[100];
  char *xargs[MAXARG];
  int fd;
  int i;
  int n;

  delim = ' ';

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

  // Read and run input commands.
  while(getline(buf, sizeof(buf)) >= 0){
    n = parseargs(&xargs[argc-1], MAXARG - argc + 1, buf);

    if(fork1() == 0)
      exec(xargs[0], xargs);
    wait(0);
    
    for(i=0; i < n; i++)
      free(xargs[argc+i-1]);
  }
  for(i=0; i < argc-1; i++)
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
