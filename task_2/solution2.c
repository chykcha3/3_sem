/*
Write a duplex pipe implementation.
typedef struct {
  int txd[2]; 
  int rxd[2]; 
} dpipe_t;
1) This pipe should connect some process with his child, for continuous communication.
2) Be careful with opened descriptors.
3) Monitor the integrity of transmitted data.
4) When one process is terminated, the other should also exit.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


typedef struct duplex_pipe
{
  int ptc[2]; 
  int ctp[2]; 
} dpipe_t;


int dpipe_create(dpipe_t *x)
{
  return pipe((*x).ptc) + pipe((*x).ctp);
}


int isItExitCode(const char *s)
{
  return (*s == 'e') && (*(s + 1) == 'x') && (*(s + 2) == 'i') && (*(s + 3) == 't') && (*(s + 4) == '\n');
}


int main()
{
  char buf[4096];
  int size;
  dpipe_t data_pipe;
  pid_t pid;
  int echo_counter, echo_times;

// initialization

  printf("Number of echoes ");
  scanf("%i", &echo_times);

  if (dpipe_create(&data_pipe) < 0) 
  {
    printf("Data pipe creation is failed!\n");
    return -1;
  }  
  if ((pid = fork()) < 0) 
  {
    printf("Fork failed!\n");
    return -1;
  }

  if (pid)
  {
// parent process
    close(data_pipe.ptc[0]);
    close(data_pipe.ctp[1]);
    do
    {
// read data from stdin
      if ((size = read(0, buf, sizeof(buf)-1)) > 0)
      {
        buf[size] = '\0';
        size++;
        printf("Got data from stdin: %s", buf);
      }
      if (isItExitCode(buf))
        echo_times = 1;
// send data to child & recieve echo
      for (echo_counter = 0; echo_counter < echo_times; echo_counter++)
      { 
        write(data_pipe.ptc[1], buf, size);
        printf("\nSent data to child (%i times left)\n", echo_times - echo_counter);
        size = read(data_pipe.ctp[0], buf, sizeof(buf) - 1);
        printf("Got echo data from child: %s", buf);
      }
      printf("End of transmission\n\n");
    }
    while(!isItExitCode(buf));
    printf("Parent is waiting for the child to exit...\n");
    waitpid(pid, NULL, 0);
    close(data_pipe.ptc[1]);
    close(data_pipe.ctp[0]);
    printf("Parent exited\n");
  }
  else
  {
//  child process
    close(data_pipe.ptc[1]);
    close(data_pipe.ctp[0]);
    do
    {
      size = read(data_pipe.ptc[0], buf, sizeof(buf) - 1);
      printf("Got data from parent: %s", buf);
      write(data_pipe.ctp[1], buf, size);
    }
    while(!isItExitCode(buf));
    close(data_pipe.ptc[0]);
    close(data_pipe.ctp[1]); 
    printf("Child exited\n");
  }

  return 0;
}
