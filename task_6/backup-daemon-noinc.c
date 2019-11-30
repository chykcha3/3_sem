#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>


#define PATH_FOR_GEN "/etc/fstab"
#define PROJ_ID 2
#define CP_BUF_SIZE 4 * 1024
#define INC_BUF_SIZE 1024
#define MODE S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH



enum type;
enum copy_direction;
struct dir_tree_node;

int delete_next_nodes(struct dir_tree_node *);
int read_next_node(struct dir_tree_node *, int);
struct dir_tree_node * read_dir_tree(int);
int write_dir_tree(struct dir_tree_node *, int);
int get_next_node(const char *, struct dir_tree_node *);
struct dir_tree_node *get_dir_tree(const char *);

int cp(const char *, const char *, const char *);
int copy_files(struct dir_tree_node *, char *,  char *, enum copy_direction);

int refresh_inc_backup(char *, char*, char*);
char *create_snapshot(char *, char *);

int init_dir(char *);
void *main_d(void *);
void *listener_d(void *);
int start_d(char *, char *, int);




typedef enum type
{
  file,
  directory
} type_t;



typedef enum copy_direction
{
  from_tree_struct,
  to_tree_struct,
  without_struct
} copy_direction_t;



typedef struct msg
{
  long mtype;
  char mtext[sizeof(time_t)];
} msg_t;



typedef enum command_from_user
{
  restore,
  kill_d
} cmd_t;



typedef struct dir_tree_node
{
  int name_length;
  char name[PATH_MAX];
  type_t type;
  struct dir_tree_node *child;
  int number_of_child;
} dir_node;



typedef struct common_args
{
  char *data_path;
  char *backup_path;
  pthread_mutex_t *is_free;
  int timeout_sec;
  int log_fd;
} daemon_arg_t;



int delete_next_nodes(dir_node *head)
{
  int i;
  if (head -> type == directory)
  {
    for (i = 0; i < head -> number_of_child; i++)
     delete_next_nodes(head -> child + i);
    free(head -> child);
    head -> number_of_child = 0;
    head -> child = NULL;
  }
}



int read_next_node(dir_node *current, int fd)
{
  int i;

  read(fd, &(current -> name_length), sizeof(int));
  read(fd, current -> name, (current -> name_length) * sizeof(char));
  current -> name[current -> name_length] = '\0';
  read(fd, &(current -> type), sizeof(type_t));
  if (current -> type == file)
  {
    current -> number_of_child = 0;
    current -> child = NULL;
  }
  else
  {
    read(fd, &(current -> number_of_child), sizeof(int));
    if (current -> number_of_child == 0)
    {
      current -> child = NULL;
    }
    else
    {
      current -> child = (dir_node *)malloc(current -> number_of_child * sizeof(dir_node));
      for (i = 0; i < current -> number_of_child; i++)
        read_next_node(current -> child + i, fd);
    }
  }

  return 0;
}



dir_node *read_dir_tree(int fd)
{
  dir_node *head;
  int i;

  head = (dir_node *)malloc(sizeof(dir_node));
  read(fd, &(head -> name_length), sizeof(int));
  read(fd, head -> name, (head -> name_length) * sizeof(char));
  head -> name[head -> name_length] = '\0';
  read(fd, &(head -> type), sizeof(type_t));
  if (head -> type == file)
  {
    head -> number_of_child = 0;
    head -> child = NULL;
  }
  else
  {
    read(fd, &(head -> number_of_child), sizeof(int));
    if (head -> number_of_child == 0)
    {
      head -> child = NULL;
    }
    else
    {
      head -> child = (dir_node *)malloc(head -> number_of_child * sizeof(dir_node));
      for (i = 0; i < head -> number_of_child; i++)
        read_next_node(head -> child + i, fd);
    }
  }
  return head;
}



int write_dir_tree(dir_node *head, int fd)
{
  int i;
  
  write(fd, &(head -> name_length), sizeof(int));
  write(fd, head -> name, head -> name_length * sizeof(char));
  write(fd, &(head -> type), sizeof(type_t));
  if (head -> type == directory)
  {
    write(fd, &(head -> number_of_child), sizeof(int));
    for (i = 0; i < head -> number_of_child; i++)
      write_dir_tree(head -> child + i, fd);
  }

  return 0;
}



int get_next_node(const char *path, dir_node *current)
{
  DIR *current_dir;
  struct dirent *entry;
  char child_path[PATH_MAX];

  current -> child = NULL;
  current -> number_of_child = 0;
  if ((current_dir = opendir(path)) != NULL)
  {
    current -> type = directory;
    while ((entry = readdir(current_dir)) != NULL)
      if ((strcmp(entry -> d_name, ".") != 0) && (strcmp(entry -> d_name, "..") != 0))
      {
        current -> number_of_child++;
        current -> child = (dir_node *)realloc(current -> child, current -> number_of_child * sizeof(dir_node));
        strcpy((current -> child + current -> number_of_child - 1) -> name, entry -> d_name);
        (current -> child + current -> number_of_child - 1) -> name_length = strlen(entry -> d_name);

        strcpy(child_path, path);
        strcat(child_path, "/");
        strcat(child_path, entry -> d_name);
        get_next_node(child_path, current -> child + current -> number_of_child - 1);
      }
    free(entry);
    free(current_dir);
  }
  else
  {
    current -> type = file;
  }

  return 0;
}



dir_node *get_dir_tree(const char *path)
{
  DIR *head_dir;
  dir_node *head;
  struct dirent *entry;
  char child_path[PATH_MAX];

  head = (dir_node *)malloc(sizeof(dir_node));
  *(head -> name) = '\0';
  head -> name_length = 0;
  head -> child = NULL;
  head -> number_of_child = 0;
  if ((head_dir = opendir(path)) != NULL)
  {
    head -> type = directory;
    while ((entry = readdir(head_dir)) != NULL)
      if ((strcmp(entry -> d_name, ".") != 0) && (strcmp(entry -> d_name, "..") != 0))
      {
        head -> number_of_child++;
        head -> child = (dir_node *)realloc(head -> child,       head -> number_of_child * sizeof(dir_node));
        strcpy((head -> child + head -> number_of_child - 1) -> name, entry -> d_name);
        (head -> child + head -> number_of_child - 1) -> name_length = strlen(entry -> d_name);

        strcpy(child_path, path);
        strcat(child_path, "/");
        strcat(child_path, entry -> d_name);
        get_next_node(child_path, head -> child + head -> number_of_child - 1);
      }
    free(entry);
    free(head_dir);
  }
  else
  {
    head -> type = file;
  }

  return head;
}



int cp(const char *filename, const char *source, const char *dest)
{
  int fd_src, fd_dest;
  char path[PATH_MAX];
  char buf[CP_BUF_SIZE];
  int count_of_read_bytes;

  strcpy(path, source);
  strcat(path, "/");
  strcat(path, filename);
  if ((fd_src = open(path, O_RDONLY | O_NONBLOCK,  S_IRUSR)) == -1)
    return -1;

  strcpy(path, dest);
  strcat(path, "/");
  strcat(path, filename);
  if ((fd_dest = open(path, O_CREAT | O_WRONLY | O_TRUNC, MODE)) == -1)
  {
    close(fd_src);
    return -1;
  }

  while ((count_of_read_bytes = read(fd_src, buf, CP_BUF_SIZE * sizeof(char))) > 0)
    write(fd_dest, buf, count_of_read_bytes);

  close(fd_src);
  close(fd_dest);
  return 0;
}



int copy_files(dir_node *current, char *src_path, char *dest_path, copy_direction_t mode)
{
  int i, fail = 0;
  char path[PATH_MAX];

  switch (mode)
  {
    case from_tree_struct:
      strcpy(path, src_path);
      if (current -> type == file)
      {
        if (cp(current -> name, path, dest_path) == -1)
          fail = -1;
      }
      else
      {
        strcat(path, current -> name);
        strcat(path, "/");
        for (i = 0; i < current -> number_of_child; i++)
          fail = (fail + 1) * (copy_files(current -> child + i, path, dest_path, mode) + 1) - 1;
      }
      break;
    case to_tree_struct:
      strcpy(path, dest_path);
      if (current -> type == file)
      {
        if (cp(current -> name, src_path, path) == -1)
          fail = -1;
      }
      else
      {
        strcat(path, current -> name);
        init_dir(path);
        strcat(path, "/");
        for (i = 0; i < current -> number_of_child; i++)
          fail = (fail + 1) * (copy_files(current -> child + i, src_path, path, mode) + 1) - 1;
      }
      break;
  }

  return fail;
}




char *create_snapshot(char *data_path, char *backup_path)
{
  dir_node *dir_tree;
  int dir_struct_fd;
  char current_path[PATH_MAX], path_error[PATH_MAX];
  struct timespec *time;
  char *timestamp;
  int i;

  time = (struct timespec *)malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_MONOTONIC, time);
  timestamp = (char *)malloc(sizeof(time_t) + 1);
  *(timestamp + sizeof(time_t) + 1) = '\0';
  for (i = sizeof(time_t) - 1; i >= 0; i--)
  {
    *(timestamp + i) = (char)((time -> tv_sec) % 10 + 48);
    (time -> tv_sec) /= 10;
  }

  strcpy(current_path, backup_path);
  strcat(current_path, "/snapshot-");
  strcat(current_path, timestamp);
  mkdir(current_path, MODE);
  strcat(current_path, "/struct_of_dir.backup");
  dir_struct_fd = open(current_path, O_CREAT | O_WRONLY | O_TRUNC, MODE);

  strcpy(current_path, backup_path);
  strcat(current_path, "/snapshot-");
  strcat(current_path, timestamp);
  strcat(current_path, "/files");
  mkdir(current_path, MODE);

  dir_tree = get_dir_tree(data_path);
  if (copy_files(dir_tree, data_path, current_path, from_tree_struct) == -1)
  {
    strcpy(current_path, backup_path);
    strcat(current_path, "/snapshot-");
    strcat(current_path, timestamp);   
    strcpy(path_error, current_path);
    strcat(path_error, "--BAD_SNAP");
    rename(current_path, path_error);
    *timestamp = -1;
  }
  write_dir_tree(dir_tree, dir_struct_fd);

  delete_next_nodes(dir_tree);
  free(dir_tree);
  free(time);
  close(dir_struct_fd);
  return timestamp;
}



int init_dir(char *path)
{
  DIR *dir;

  if ((dir = opendir(path)) == NULL)
    mkdir(path, MODE);
  free(dir);

  return 0;
}



void *main_d(void * arg)
{
  char *backup_path, *data_path, *timestamp;
  int *timeout, *log_fd;
  pthread_mutex_t *is_free;

  data_path = ((daemon_arg_t *)arg) -> data_path;  
  backup_path = ((daemon_arg_t *)arg) -> backup_path;
  timeout = &(((daemon_arg_t *)arg) -> timeout_sec);
  is_free = ((daemon_arg_t *)arg) -> is_free;
  log_fd = &(((daemon_arg_t *)arg) -> log_fd);

  while (*timeout != 0)
  {
    pthread_mutex_lock(is_free);
    dprintf(*log_fd, "Creating snapshot...\n");
    if (*(timestamp = create_snapshot(data_path, backup_path)) == -1)
      dprintf(*log_fd, "ERROR! Can't create snapshot!\n");
    else
      dprintf(*log_fd, "Snapshot created with %s timestamp\n", timestamp);
    free(timestamp);
    pthread_mutex_unlock(is_free);
    sleep(*timeout);
  }

  pthread_exit(NULL);
}



int restore_d(char *data_path, char *backup_path, char *timestamp_backup)
{
  dir_node *dir_tree;
  int dir_struct_fd;
  char current_path[PATH_MAX];

  init_dir(data_path);

  strcpy(current_path, backup_path);
  strcat(current_path, "/snapshot-");
  strcat(current_path, timestamp_backup);
  strcat(current_path, "/struct_of_dir.backup");
  dir_struct_fd = open(current_path, O_RDONLY | O_NONBLOCK,  S_IRUSR);

  dir_tree = read_dir_tree(dir_struct_fd);

  strcpy(current_path, backup_path);
  strcat(current_path, "/snapshot-");
  strcat(current_path, timestamp_backup);
  strcat(current_path, "/files");

  if ((copy_files(dir_tree, current_path, data_path, to_tree_struct)) == -1)
    return -1;

  delete_next_nodes(dir_tree);
  free(dir_tree);
  return 0;
}



cmd_t msg2cmd(long msg)
{
  switch(msg)
  {
    case 10:
      return kill_d;
    case 20:
      return restore;
  }
}



void *listener_d(void * arg)
{
  char *backup_path, *data_path;
  pthread_mutex_t *is_free;
  key_t key;
  int msq, chat_log_fd, *log_fd;
  msg_t *msg;
  cmd_t cmd;
  char *path;

  path = (char *)malloc(PATH_MAX);
  data_path = ((daemon_arg_t *)arg) -> data_path;  
  backup_path = ((daemon_arg_t *)arg) -> backup_path;
  is_free = ((daemon_arg_t *)arg) -> is_free;
  strcpy(path, backup_path);
  strcat(path, "/chat.log");
  chat_log_fd = open(path, O_CREAT | O_WRONLY | O_APPEND, MODE);
  free(path);
  log_fd = &(((daemon_arg_t *)arg) -> log_fd);

  key = ftok(PATH_FOR_GEN, PROJ_ID);
  msq = msgget(key, IPC_CREAT | 0666);
  msg = (msg_t *)malloc(sizeof(msg_t));

  do
  {
    dprintf(chat_log_fd, "Listen to pipe with key %i...\n", key);
    msgrcv(msq, msg, sizeof(msg_t), 0, 0);
    (msg -> mtype)++;
    msgsnd(msq, msg, sizeof(msg_t), 0);
    (msg -> mtype)--;
    cmd = msg2cmd(msg -> mtype);
    switch (cmd)
    {
      case restore:
        dprintf(chat_log_fd, "Get restore command, backup timestamp: %s\n", msg -> mtext);
        pthread_mutex_lock(is_free);
        dprintf(*log_fd, "Restoring from backup with %s timestamp\n", msg -> mtext);
        if (restore_d(data_path, backup_path, msg -> mtext) == -1)
        {
          dprintf(*log_fd, "ERROR! Can't restore data!\n");
          (msg -> mtype)++;
          *(msg -> mtext) = 0;
          msgsnd(msq, msg, sizeof(msg_t), 0);
          (msg -> mtype)--;
        }
        else
        {
          dprintf(*log_fd, "Data recovered successfully\n");
          (msg -> mtype)++;
          *(msg -> mtext) = 1;
          msgsnd(msq, msg, sizeof(msg_t), 0);
          (msg -> mtype)--;
        }
        pthread_mutex_unlock(is_free);
        break;
      case kill_d:
        dprintf(chat_log_fd, "Get exit command\n\n");
        msgctl(msq, IPC_RMID, NULL);
        break;
    }
    sleep(1);
  }
  while (cmd != kill_d);

  pthread_mutex_lock(is_free);
  dprintf(*log_fd, "Sending exit signal to backup creator...\n");
  ((daemon_arg_t *)arg) -> timeout_sec = 0;
  pthread_mutex_unlock(is_free);
  free(msg);
  close(chat_log_fd);
  pthread_exit(NULL);
}



int start_d(char *data_path, char *backup_path, int timeout_sec)
{
  daemon_arg_t *args;
  pthread_t main_id, listener_id;
  char *path;

  args = (daemon_arg_t *)malloc(sizeof(daemon_arg_t));
  args -> data_path = data_path;
  args -> backup_path = backup_path;
  args -> is_free = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(args -> is_free, NULL);
  args -> timeout_sec = timeout_sec;

  init_dir(backup_path);

  path = (char *)malloc(PATH_MAX);
  strcpy(path, backup_path);
  strcat(path, "/main.log");
  args -> log_fd = open(path, O_CREAT | O_WRONLY | O_APPEND, MODE);
  free(path);

  dprintf(args -> log_fd, "Starting daemon with next parametres:\n");
  dprintf(args -> log_fd, "    Data directory: %s\n", data_path);
  dprintf(args -> log_fd, "    Backup directory: %s\n", backup_path);
  dprintf(args -> log_fd, "    Backup timeout: %i sec\n", timeout_sec);

  dprintf(args -> log_fd, "Starting main threads...\n");
  pthread_create(&main_id, NULL, main_d, args);
  pthread_create(&listener_id, NULL, listener_d, args);
  dprintf(args -> log_fd, "Main threads started succesfully\n");

  pthread_join(listener_id, NULL);
  pthread_join(main_id, NULL);
  dprintf(args -> log_fd, "Exiting...\n\n");
  close(args -> log_fd);
  free(args -> is_free);
  free(args);
  return 0;
}



int main(int argc, char *argv[])
{
  pid_t pid;
  int i, timeout = 0;

  if (argc >= 4)
  {pid =0;
    pid = fork();
    if (pid < 0)
    {
      printf("ERROR! fork()\n");
      return -1;
    }
    else
      if (pid == 0)
      {
        setsid();
        close(0);
        close(1);
        for (i = 0; *(argv[3] + i) != '\0'; i++)
          timeout = timeout * 10 + *(argv[3] + i) - 48;
        start_d(argv[1], argv[2], timeout);
      }
  }
  else
    printf("ERROR! too few arg\n");

  return 0;
}
