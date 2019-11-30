#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


#define PATH_FOR_GEN "/etc/fstab"
#define PROJ_ID 2



typedef struct msg
{
  long mtype;
  char mtext[sizeof(time_t)];
} msg_t;



int main(int argc, char *argv[])
{
  char cmd;
  msg_t msg;
  int msq;

  msq = msgget(ftok(PATH_FOR_GEN, PROJ_ID), IPC_CREAT | 0666);

  do
  {
    printf("\nCommand (h for help): ");
    scanf("%c%*c", &cmd);
    switch (cmd)
    {
      case 'h':
        printf("\nHelp:\n\n");
        printf("h  print help\n");
        printf("k  kill daemon\n");
        printf("q  quit\n");
        printf("r  restore backup\n");
        break;
      case 'k':
        msg.mtype = 10;
        printf("Sending kill signal to daemon...\n");
        msgsnd(msq, &msg, sizeof(msg_t), 0);
        msgrcv(msq, &msg, sizeof(msg_t), 0, 11);
        printf("Message delivered\n");
        break;
      case 'q':
        printf("Exiting...\n");
        break;
      case 'r':
        msg.mtype = 20;
        printf("Please, enter timestamp of backup: ");
        scanf("%s%*c", msg.mtext);
        printf("Sending restore signal to daemon...\n");
        msgsnd(msq, &msg, sizeof(msg_t), 0);    
        msgrcv(msq, &msg, sizeof(msg_t), 0, 21);
        printf("Message delivered\n");  
        msgrcv(msq, &msg, sizeof(msg_t), 0, 21);
        if (*msg.mtext == 1)
          printf("Data recovered successfully\n");
        if (*msg.mtext == 0)
          printf("ERROR! Can't restore data!\n");
        break;
    }
  }
  while (cmd != 'q');

  return 0;
}
