#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <math.h>
#include <sys/random.h>
#include <limits.h>

#define PATHNAME "/etc/fstab"
#define PROJ_ID 1
#define shmflg IPC_CREAT | 0666

// info about integral in calc process
#define LEFT_RANGE 0
#define RIGHT_RANGE 1
#define Y_SCALE 1

// integrate function
double f(double x)
{
  return pow(log(x + sqrt(pow(x, 2) + 1)), 2) / (pow(x, 2) + 1) / pow(log(x + sqrt(pow(x, 2) + 1)), 2);
}

static inline double norm(unsigned int x)
{
  return (double)x / UINT_MAX;
}

void *calculate(void *arg)
{
// local var are faster than global or shm var
  int points_per_thread = *((int *)arg);
  unsigned int buf[256 / sizeof(unsigned int)];
  int points_per_session;
  int i;
  int k = 0;

  while (points_per_thread > 0)
  {
    points_per_session = getrandom(buf, 256, 0) / 2 / sizeof(unsigned int);
    for (i = 0; (i < points_per_thread) && (i < points_per_session); i++)
      if( (norm(buf[2 * i + 1]) * Y_SCALE) < f(LEFT_RANGE + (RIGHT_RANGE - LEFT_RANGE) * norm(buf[2 * i])))
        k++;
    points_per_thread -= points_per_session;
  }

  (*((int *)arg)) = k;

  pthread_exit(NULL);
}

int main()
{
  int i;
  key_t key = ftok (PATHNAME, PROJ_ID);
  key_t key_sync = ftok (PATHNAME, PROJ_ID + 1);
  int shmid, shmid_sync = shmget(key_sync, 3 * sizeof(int), shmflg);
  int *shm_ptr;
  int *shm_ptr_sync = shmat(shmid_sync, NULL, 0);  
  pthread_t *thread_id;

// wait for control process
  while (*shm_ptr_sync == 0);

// initialize shared memory for main data
  shmid = shmget(key, *shm_ptr_sync * sizeof(int), shmflg);
  shm_ptr = shmat(shmid, NULL, 0);
  thread_id = (pthread_t *)malloc(*shm_ptr_sync * sizeof(pthread_t));

// prepare data for threads and start them
  for (i = 0; i < *shm_ptr_sync; i++)
  {
    *(shm_ptr + i) = *(shm_ptr_sync + 1);
    pthread_create(thread_id + i, NULL, calculate, shm_ptr + i);
  }

// wait for calculation
  for (i = 0; i < *shm_ptr_sync; i++)
    pthread_join(*(thread_id + i), NULL);

// send data for answer
  *(shm_ptr_sync + 2) = Y_SCALE;
  *(shm_ptr_sync + 1) = RIGHT_RANGE - LEFT_RANGE;
// this command mean "end of calculation"
  *shm_ptr_sync = 0;

  free(thread_id);
  shmdt(shm_ptr_sync);
  shmdt(shm_ptr);
  return 0;
}
