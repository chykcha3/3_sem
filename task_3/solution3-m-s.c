#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/msg.h>


#define BUF_SIZE 4096
#define PATH "/etc/fstab"


struct mymsg
{
  long mtype;
  char mtext[BUF_SIZE];
};



int main (int argc, char **argv)
{
  int fd, msqid;
  struct mymsg msg;
  struct timespec start, end;

  fd = open(*(argv + 1), O_RDONLY, S_IRUSR);
  msqid = msgget(ftok(PATH, 0), IPC_CREAT | 0666);

  
  clock_gettime(CLOCK_MONOTONIC, &start);

  while ((msg.mtype = read(fd, msg.mtext, BUF_SIZE)) > 0)
    msgsnd(msqid, &msg, BUF_SIZE, 0);
  msg.mtype = BUF_SIZE + 1;
  msgsnd(msqid, &msg, BUF_SIZE, 0);
  msgrcv(msqid, &msg, BUF_SIZE, BUF_SIZE + 2, 0);

  clock_gettime(CLOCK_MONOTONIC, &end);
  
  close(fd);
  msgctl(msqid, IPC_RMID, NULL);

  printf("%.5f\n", end.tv_sec - start.tv_sec + (double)(end.tv_nsec - start.tv_nsec) / 1000000000);
  return 0;
}
