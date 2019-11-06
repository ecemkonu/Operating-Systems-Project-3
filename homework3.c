#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#define KEYSHM1 ftok(argv[0], 42)
#define KEYSHM2 ftok(argv[0], 43)
#define KEYSEM1 ftok(argv[0],44)
#define KEYSEM2 ftok(argv[0],45)

const char colors[6] = {'R', 'G', 'O', 'B', 'Y','P'};
const int waitTime[6] = {1,2,2,2,1,1};

int colorIndex(char c)
{
   int j=0;
   for(j=0; j<6; j++)
   {
      if(c ==colors[j])
      {
         return j;
      }
   }
}
//(increment operation)
void sem_signal(int semid,int semnum, int val)
{
   struct sembuf semaphore;
   semaphore.sem_num = semnum;
   semaphore.sem_op = val;
   semaphore.sem_flg = 1;
   semop(semid, &semaphore, 1);
}

//(decrement operation)
void sem_wait(int semid, int semnum, int val)
{
   struct sembuf semaphore;
   semaphore.sem_num = semnum;
   semaphore.sem_op = (-1 * val);
   semaphore.sem_flg = 1;
   semop(semid, &semaphore, 1);
}

int main(int argc, char *argv[])
{
   int arrLen = 0;
   int shmid1 = 0, shmid2 = 0;
   FILE *fp;
   int i = 0;

   char c;
   int numColors = 6;
   if (argc < 2)
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

   fscanf(fp, "%d\n", &arrLen);

   shmid1 = shmget(KEYSHM1, arrLen * sizeof(char), 0700 | IPC_CREAT);
   char * colorOrder = (char *)shmat(shmid1, 0, 0);
   shmid2 = shmget(KEYSHM2, arrLen * sizeof(int), 0700 | IPC_CREAT);
   int semBoxSelect = semget(KEYSEM1, arrLen, 0700|IPC_CREAT); //Sem for detecting which box to print.
   semctl(semBoxSelect, 0, SETALL, 0);
   int semPaintingTable = semget(KEYSEM2, 2,0700|IPC_CREAT);
   semctl(semPaintingTable, 0, SETVAL, 1);  //at max 2 boxes can be painted, create mutex with value 2 for scheduling.

   int * isColored = (int *)shmat(shmid2, 0, 0);
   pid_t f;
   pid_t * process_ids = (pid_t *)malloc(arrLen*sizeof(pid_t));     //stores process ids in an array.
   for (i = 0; i < arrLen; i++)
   {
      fscanf(fp, "%c\n", &c);
      colorOrder[i] =c;
      printf("%c\n", colorOrder[i]);
      isColored[i] = 0; //integer array is implemented to keep bool state of colored, 1 for true 0 for false.
   }
   fclose(fp);
      printf("here");

   for(i=0; i<arrLen;i++)
   {
      process_ids[i] = f = fork();
      if(f == 0)
			break;
   }
   if(f==0)
   {
      //in child process
      sem_wait(semBoxSelect, i, 1); //wait from parent to process box. Required for ordering processes by color. Each color has its own semaphore.
      printf("%d\t%c\n", getpid(),colorOrder[i]);
      int index = colorIndex(colorOrder[i]);
      sleep(waitTime[index]);
      shmdt(colorOrder);
      shmdt(isColored);
      sem_signal(semPaintingTable,0, 1); 
   }
   else
   { //parent process
      char currentColor = colorOrder[0];
      char nextColor;
      int next = 0;
      int colorChanges =1;
      int j=0;
      int dyeOrder[arrLen];
      int remainingBoxes = arrLen;
      while(remainingBoxes) //while there are still boxes to paint
      {
         for(j=0;j<arrLen; j++)
         {
            if(isColored[j]==0)
            {
               if(colorOrder[j]==currentColor)
               {
                  dyeOrder[arrLen-remainingBoxes] = j;
                  remainingBoxes--; //delete dyed box from remaining counter/
                  isColored[j] =1;
                  sem_signal(semBoxSelect,j,1);
                  sem_wait(semPaintingTable,0, 1);
               }
            else if(next ==0)
            {
               nextColor = colorOrder[j]; //find the next color coming after current color. next is resetted after each iteration of for loop.
               next++;
               colorChanges++; 
            }
            }
         }
         next =0;
         sem_wait(semPaintingTable, 0, 1);
         sem_signal(semPaintingTable, 0,1);
         currentColor = nextColor; // get next color from order to paint.
      }   
      printf("%s %d\n", "Color changes:", colorChanges);
      FILE* out_fp = fopen(argv[2], "w");                 // Open file stream for output file.
      fprintf(out_fp, "%d\n", colorChanges); //colro changes written to output file
      for (i = 0; i < arrLen; ++i) {
         fprintf(out_fp, "%d\t%c\n", process_ids[dyeOrder[i]],colorOrder[dyeOrder[i]]);   // Then print the boxes and related information to the output file.
      }
      fclose(out_fp);                             // Close file stream for output file.

      semctl(semPaintingTable, 0, IPC_RMID); //releases semaphores.
      semctl(semBoxSelect, 0, IPC_RMID);         
      shmdt(colorOrder);
      shmdt(isColored);
      shmctl(shmid1, IPC_RMID, 0);
      shmctl(shmid2, IPC_RMID,0); 
      free(process_ids);
     
   }
   

   return 0;
}
