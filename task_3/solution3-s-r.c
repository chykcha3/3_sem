#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>


#define BUF_SIZE 1024 * 1024 * 4
#define PATH "/etc/fstab"
#define MODE S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH


enum current_status
{
  ready_for_using_by_sender,
  ready_for_using_by_recevier
};


struct transmission_buf
{
  char end;
  enum current_status status;
  int bytes_in_buf;
  char buf[BUF_SIZE];
};



int main (int argc, char **argv)
{
  int fd, shmid;
  struct transmission_buf *data;

  fd = open(*(argv + 1), O_CREAT | O_TRUNC | O_WRONLY, MODE);
  shmid = shmget(ftok(PATH, 0), sizeof(struct transmission_buf), IPC_CREAT | 0666);
  data = (struct transmission_buf *)shmat(shmid, NULL, 0);

  while (!(data -> end))
  {
    while (data -> status != ready_for_using_by_recevier);
    if (data -> bytes_in_buf > 0)
    {
      write(fd, data -> buf, data -> bytes_in_buf);
      data -> status = ready_for_using_by_sender;
    }
  }
  data -> end = 0;

  close(fd);
  shmdt(data);

  return 0;
}
