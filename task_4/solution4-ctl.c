#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <time.h>

#define PATHNAME "/etc/fstab"
#define PROJ_ID 1
#define shmflg IPC_CREAT | 0666

// info about how to calc in ctl process
#define COUNT_OF_POINTS 1000000

int main(int argc, char **argv)
{
  int i;
  key_t key = ftok (PATHNAME, PROJ_ID);
  key_t key_sync = ftok (PATHNAME, PROJ_ID + 1);
  int shmid, shmid_sync = shmget(key_sync, 3 * sizeof(int), shmflg);
  int *shm_ptr;
  int *shm_ptr_sync = shmat(shmid_sync, NULL, 0);  
  int threads_count = 0;
  int points_per_thread;
  int points_in_area = 0;
  struct timespec begin, end;

// parsing argument [thread_counts]
  if (argc > 1)
    for (i = 0; *(*(argv + 1) + i) != '\0'; i++)
      threads_count = threads_count * 10 + (*(*(argv + 1) + i) - 48);

// initialize shared memory for main data
  shmid = shmget(key, threads_count * sizeof(int), shmflg);
  shm_ptr = shmat(shmid, NULL, 0);

  points_per_thread = COUNT_OF_POINTS / threads_count;
  *(shm_ptr_sync + 1) = points_per_thread;

  clock_gettime(CLOCK_MONOTONIC, &begin);

// this command mean "start calculation"
  *shm_ptr_sync = threads_count;

// wait for calculation process
  while (*shm_ptr_sync != 0);

// collect data
  for (i = 0; i < threads_count; i++)
    if (*(shm_ptr + i) > 0)
    {
      points_in_area += *(shm_ptr + i);
      *(shm_ptr + i) = 0;
    }

  clock_gettime(CLOCK_MONOTONIC, &end);

// answer
  printf("%0.20E\n", *(shm_ptr_sync + 1) * *(shm_ptr_sync + 2) * (double)points_in_area / (points_per_thread * threads_count));
// time for calculation
  printf("%0.10f\n", end.tv_sec - begin.tv_sec + (double)(end.tv_nsec - begin.tv_nsec ) / 1000000000);

  shmdt(shm_ptr_sync);
  shmdt(shm_ptr);
  shmctl(shmid_sync, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);
  return 0;
}
