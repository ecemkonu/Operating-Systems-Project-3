#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#define _GNU_SOURCE
#define KEYSHM ftok(strcat(get_current_dir_name() ,argv[0]),1)
 int main(int argc, char* argv[])
 {
     int arrLen =0;
     int shmid =0;
     FILE *fp;
     char c;
     char *arr;
     if(argc <2)
     {
        perror("Arguments necessary for execution are not provided, exiting\n");
        exit(EXIT_FAILURE);
     }
     fp = fopen(argv[1], "r");
    if (fp == NULL)
   {
      perror("File could not be opened.\n");
      exit(EXIT_FAILURE);
   }
    c = fgetc(fp);
    arrLen = atoi(&c);
    shmid = shmget(KEYSHM, arrLen*sizeof(char), IPC_CREAT);
    arr = (char *)shmat(shmid,0,0);
    return 0;
 }
