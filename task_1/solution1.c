/*
Task_1: 
  Write a 'shell-wrapper' program which allow you to:
- run another programs via command line cyclically getting commands from STDIN and running it somewhere, e.g. in child process.
- get exit codes of terminated programs
## TIPS:
1. Use "2_fork_wait_exit.c" and "4_exec_dir.c" from examples. Combine them.
2. Parse input string according to the type of exec* (for ex, see "man execlp").
   a) if execvp is used, string splitting into "path" and "args" is all you need.
   b) the program should be environment($PATH variable)-sensitive: for ex.,'/bin/ls' should be executed with string 'ls'. 
3. Collect exit codes via waitpid/WEXITSTATUS.
4. Note that a size of command can reach a lot of kbytes: type "getconf ARG_MAX" cmd to check it.
*/

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>


const int MAX_CMD_LENGTH = 255;

const char check_exit_cmd(char *s)
{
  return (*s == 'e') && (*(s + 1) == 'x') && (*(s + 2) == 'i') && (*(s + 3) == 't');
}

char ** parse_cmd(const char *cmd)
{
  const char delim = ' ';
  char **string = (char**)malloc(MAX_CMD_LENGTH * sizeof(char*));
  char *safe_cmd_copy = (char*)malloc(MAX_CMD_LENGTH * sizeof(char));
  int i = 0, j = 0;

  for (i; *(cmd + i) != '\n'; i++)
    *(safe_cmd_copy + i) = *(cmd + i);
  *(safe_cmd_copy + i) = '\0';

//  parsing

  i = 0;
  while (*(safe_cmd_copy + i) != '\0')
  {
    if (*(safe_cmd_copy + i) == delim)
    {
      *(safe_cmd_copy + i) = '\0';
      i++;
    }
    while (*(safe_cmd_copy + i) == delim)
      i++;
    *(string + j) = safe_cmd_copy + i;
    j++;
    while ( (*(safe_cmd_copy + i) != delim) && (*(safe_cmd_copy + i) != '\0'))
      i++;
  }
  *(string + j) = NULL;

  return string;
}

const int run_cmd(const char *cmd)
{
  const pid_t pid = fork();
  char **args;
  int i, status;

  if (pid < 0) 
    printf("ERROR! Fork failed!\n");
  else
    if (pid == 0)
    {
      args = parse_cmd(cmd);

      if (check_exit_cmd(*args))
        status = 42;
      else
        status = execvp(*args, args);

      for (i = 0; *(args + i) != NULL; i++)
        free(*args + i);
      free(args);
      exit(status);
    }
    else
      waitpid(pid, &status, 0);

  return WEXITSTATUS(status);
}


int main()
{
  char *cmd = (char *)malloc(MAX_CMD_LENGTH * sizeof(char));
  int exit_code;

  while(1) 
  {
    fgets(cmd, MAX_CMD_LENGTH, stdin);
    switch (exit_code = run_cmd(cmd))
    {
      case -1:
        printf("ERROR! Exec failed!\n");
      break;
      case 42:
        free(cmd);
        exit(0);
      break;
    }    
  }

  return 0;
}

