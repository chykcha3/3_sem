1. backup-daemon-noinc.c
Usage:
  backup-daemon_EXEC_NAME DATA_DIR WORK_DIR TIMEOUT
Daemon save DATA_DIR backup to WORK_DIR with timestamp every TIMEOUT seconds. If any errors detected in process, backup will be mark "--BAD_SNAP". Can restore data. Write log to main.log. Write messages from control programm to chat.log.

2. backup-daemon-ctl.c
Usage:
  backup-daemon-ctl_EXEC_NAME
Press h for help. Can send command to restore data with timestamp or kill running daemon.

