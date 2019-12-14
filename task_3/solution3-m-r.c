#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/msg.h>


#define BUF_SIZE 4096
#define PATH "/etc/fstab"
#define MODE S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH


struct mymsg
{
  long mtype;
  char mtext[BUF_SIZE];
};



int main (int argc, char **argv)
{
  int fd, msqid;
  struct mymsg msg;

  fd = open(*(argv + 1), O_CREAT | O_TRUNC | O_WRONLY, MODE);
  msqid = msgget(ftok(PATH, 0), IPC_CREAT | 0666);

  do
  {
    msgrcv(msqid, &msg, BUF_SIZE, 0, 0);
    if (msg.mtype != BUF_SIZE + 1)
      write(fd, msg.mtext, msg.mtype);
  }
  while (msg.mtype != BUF_SIZE + 1);
  msg.mtype = BUF_SIZE + 2;
  msgsnd(msqid, &msg, BUF_SIZE, 0);

  close(fd);

  return 0;
}
