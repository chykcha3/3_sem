#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>


#define BUF_SIZE 1024 * 1024 * 4
#define PATH "/etc/fstab"


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
  struct timespec start, end;

  fd = open(*(argv + 1), O_RDONLY, S_IRUSR);
  shmid = shmget(ftok(PATH, 0), sizeof(struct transmission_buf), IPC_CREAT | 0666);
  data = (struct transmission_buf *)shmat(shmid, NULL, 0);

  
  clock_gettime(CLOCK_MONOTONIC, &start);

  while ((data -> bytes_in_buf = read(fd, data -> buf, BUF_SIZE)) > 0)
  {
    data -> status = ready_for_using_by_recevier;
    while (data -> status != ready_for_using_by_sender);
  }
  data -> end = 1;
  data -> status = ready_for_using_by_recevier;
  while (data -> end);

  clock_gettime(CLOCK_MONOTONIC, &end);
  
  close(fd);
  shmdt(data);
  shmctl(shmid, IPC_RMID, NULL);

  printf("%.5f\n", end.tv_sec - start.tv_sec + (double)(end.tv_nsec - start.tv_nsec) / 1000000000);
  return 0;
}
