#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 8
#define MODE S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH


int main (int argc, char **argv)
{
  int fd, fd_fifo_data, fd_fifo_ctl_write, fd_fifo_ctl_read;
  int count_of_bytes;
  char buf[BUF_SIZE];
  struct timespec start, end;

  mkfifo("fifo_data", MODE);
  mkfifo("fifo_ctl_to_snd", MODE);
  mkfifo("fifo_ctl_to_rcv", MODE);

  fd = open(*(argv + 1), O_RDONLY, S_IRUSR);
  fd_fifo_data = open("fifo_data", O_WRONLY);
  fd_fifo_ctl_write = open("fifo_ctl_to_rcv", O_WRONLY);
  fd_fifo_ctl_read = open("fifo_ctl_to_snd", O_RDONLY);


  clock_gettime(CLOCK_MONOTONIC, &start);

  while ((count_of_bytes = read(fd, buf, BUF_SIZE)) > 0)
    write(fd_fifo_data, buf, count_of_bytes);
  write(fd_fifo_ctl_write, buf, BUF_SIZE);
  read(fd_fifo_ctl_read, buf, BUF_SIZE);

  clock_gettime(CLOCK_MONOTONIC, &end);

  close(fd);
  remove("fifo_data");
  remove("fifo_ctl_to_snd");
  remove("fifo_ctl_to_rcv");

  printf("%.5f\n", end.tv_sec - start.tv_sec + (double)(end.tv_nsec - start.tv_nsec) / 1000000000);
  return 0;
}
