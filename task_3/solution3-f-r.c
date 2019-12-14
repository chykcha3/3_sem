#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <unistd.h>


#define BUF_SIZE 1024 * 1024 * 4
#define MODE S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH



int main (int argc, char **argv)
{
  int fd, fd_fifo_data, fd_fifo_ctl_write, fd_fifo_ctl_read;
  int count_of_bytes;
  char buf[BUF_SIZE];

  mkfifo("fifo_data", MODE);
  mkfifo("fifo_ctl_to_snd", MODE);
  mkfifo("fifo_ctl_to_rcv", MODE);

  fd = open(*(argv + 1), O_CREAT | O_TRUNC | O_WRONLY, MODE);
  fd_fifo_data = open("fifo_data", O_RDONLY | O_NONBLOCK);
  fd_fifo_ctl_read = open("fifo_ctl_to_rcv", O_RDONLY | O_NONBLOCK);
  fd_fifo_ctl_write = open("fifo_ctl_to_snd", O_WRONLY);


  while (((count_of_bytes = read(fd_fifo_data, buf, BUF_SIZE)) > 0) || (read(fd_fifo_ctl_read, buf, BUF_SIZE) == -1))
    if (count_of_bytes > 0)
      write(fd, buf, count_of_bytes);
  write(fd_fifo_ctl_write, buf, BUF_SIZE);

  close(fd);

  return 0;
}
