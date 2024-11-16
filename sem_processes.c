/***
Name: Carleton Williams II
Student ID: @03028817 | 003028817
Date: 11/14/2024
Course: Operating Systems
Professor: Dr. Burge
Project 2: Sychronization Semaphores
Current File: sem_processes.c
***/
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdbool.h>

//Preproccessor directives
#define SEMAPHRONE_NAME "NSYNC"

//Global variables
int sentinel = 0;

void handle_signal_int(int sig)
{
    sentinel = 1;
}

int main(int argc, char *argv[]) 
{
    // Register signal handler
    signal(SIGINT, handle_signal_int);
    
    //Check for the appropiate number of arguments
    if(argc < 3)
    {
        fprintf(stderr,"Usage: %s <parents> <children> (where parent = [1 | 2], children = [N > 0]\n", argv[0]);
        return 1;
    }
    
    //Convert the arguments to integer and check to see if the values are valid
    int parentCount = atoi(argv[1]);
    if(parentCount < 1 || parentCount > 2)
    {
        fprintf(stderr,"Invalid parent count: %s\n", argv[1]);
        return 1;
    }
    
    //Convert the command line integer argument and check the poor student amount
    int studentCount = atoi(argv[2]);
    if(studentCount < 1)
    {
        fprintf(stderr,"Invalid student count: %s\n", argv[2]);
        return 1;
    }
    
    //Boolean flag for the second child process
    bool is_two_parents = false;
    //Check if the second child process is needed
    if(parentCount == 2)
    {
        is_two_parents = true;
    }
    
    //Semaphore
    sem_t *semaphore;
    //Open the semaphrone
    semaphore = sem_open(SEMAPHRONE_NAME, O_CREAT, 0666, 1);

    //Now check if the semaphore was created
    if(semaphore == SEM_FAILED)
    {
        perror("semaphore creation failed");
        exit(1);
    }
 
    //Shared memory id
    int shared_mem_id;
    //Begin initializing the shared memory ID for both variables; size of the 2 integers
    shared_mem_id = shmget(IPC_PRIVATE, 2*sizeof(int), IPC_CREAT | 0666);

    //Pointer to shared memory
    int *shared_mem_ptr;
    //Insert the shared memory ID to the memory pointer
    shared_mem_ptr = (int*) shmat(shared_mem_id, NULL, 0);

  //Now check if the inesertion was successful
  if(shared_mem_ptr == (int *)  -1)
  {
      perror("The insertion of the shared memory ID was unsuccessful");
      exit(0);
  }

  //Pointer to memory id with their respective shared variables
  //for BankAccount and the Turn
    int *BankAccount = &shared_mem_ptr[0], *Turn = &shared_mem_ptr[1];

    //Initialize the shared variables
    *BankAccount = 0;

    //Set the seed random number generator for the time
    srand(time(NULL));

    //Check shared memory ID init success
    if(shared_mem_id < 0)
    {
        perror("The shared memory ID initialization process was unsuccessful!");
        exit(0);
    }

    //Declare the two child processes for the Poor Student and Loveable Mom, respectively
    pid_t pid_students[studentCount], pid2, pid3;

    //Begin the first N child processes: The Poor Student(s)
    int i = 0;
    for(i = 0; i < studentCount; i++)
    {
            //Start the ith child process
            pid_students[i] = fork();
            //Check if the ith child process was successfully created
            if(pid_students[i] < 0)
            {
                fprintf(stderr,"The creation of the child process #%d was unsuccessful\n",i);
                exit(0);
            }

            //The Child process
            if(pid_students[i] == 0)
            {
                //Reset the local balance
                int localBalance = 0;

                // Register the signal handler for SIGINT
                signal(SIGINT, handle_signal_int);

                //While the sentinel is not set to 1, by the use of the SIGINT signal handler
                while(!sentinel)
                {
                    //If the SIGINT signal is received, then break the loops
                    if(sentinel) break;
                    //Sleep some random amount of time between 0 - 5 seconds
                    sleep(rand() % 5 + 0);
                    printf("Poor Student #%d: Attempting to Check Balance\n", i+1);

                    //Set a random number between 0 - 50 for the decision and the amount of money to be withdrawn
                    int decision = rand() % 50 + 0;

                    //Critcal Section #1; Process needs to use the shared memory
                    //Wait for the turn to be 0
                    sem_wait(semaphore);
                    //Copy the conents of the BankAccount to the localBalance
                    localBalance = *BankAccount;
                    //Allow another process to use the shared memory
                    sem_post(semaphore);

                    //Check if the needed amount is an even or odd number
                    if(decision % 2 == 0)
                    {
                        //If the needed amount is even, then the student will withdraw the amount
                        int need = rand() % 50 + 0;
                        printf("Poor Student #%d needs $%d\n",i+1, need);

                        //Check if the needed amount is less than the current balance
                        if(need <= localBalance)
                        {
                            //Critical Section #2; Process needs to use the shared memory
                            sem_wait(semaphore);
                            //Withdraw from the bank and the display the transacation
                            localBalance -= need;
                            //Copy the contents of the Bank Account to the local Balance
                            *BankAccount = localBalance;
                            //Allow another process to use the shared memory
                            sem_post(semaphore);

                            //Diplay the transacation
                            printf("Poor Student #%d: Withdraws $%d / Balance = $%d\n",i+1, need, localBalance);
                        }
                        else
                        {
                            //If the needed amount is greater than the current balance, then the student will not withdraw
                            printf("Poor Student #%d: Not Enough Cash ($%d)\n",i+1,  localBalance);
                        }
                    }
                    else
                    {
                        //If the needed amount is odd, then the student will not withdraw the amount
                        printf("Poor Student #%d: Last Checking Balance = $%d\n",i+1, localBalance);
                    }
                }
                exit(0);
            }
    }

    //Check if the second child process is needed
    if(is_two_parents)
    {
        //The Loveable Mom: Second Child Process
        pid2 = fork();
        //Check if the second child process was successfully created
        if(pid2 < 0)
        {
            perror("The creation of the second child process was unsuccessful");
            exit(1);
        }
        if(pid2 == 0)
        {
            // Register the signal handler for SIGINT
            signal(SIGINT, handle_signal_int);
            
            //Reset the local balance
            int localBalance = 0;
            //Run indefinetly 
            while(!sentinel)
            {
                //If the SIGINT signal is received, then break the loops
                if(sentinel) break;
                //Sleep some random amount of time between 0 - 10 seconds
                sleep(rand() % 10 + 0);
                printf("Loveable Mom: Attempting to Check Balance\n");

                //Critcal Section #3; Process needs to use the shared memory
                //Wait for the turn to be 0
                sem_wait(semaphore);
                //Copy the conents of the BankAccount to the localBalance
                localBalance = *BankAccount;
                //Allow another process to use the shared memory
                sem_post(semaphore);

                //Check if the local balance is less than or equal to 100
                if(localBalance <= 100)
                {
                    //Then the mom will deposit the amount randomly from a range of $0 - $125
                    int amount = rand() % 125 + 0;

                    //Critical Section #4; Process needs to use the shared memory
                    sem_wait(semaphore);
                    //Add the amount to the account
                    localBalance += amount;
                    *BankAccount = localBalance;
                    //Allow another process to use the shared memory
                    sem_post(semaphore);
                    //Diplay the transacation
                    printf("Loveable Mom: Deposits $%d / Balance = $%d\n", amount, localBalance);
                }
            }
             exit(0);
        }
    }
    
    //The Dear Old Dad: Parent Process
    pid3 = fork();
    //Check if the parent process was successfully created
    if(pid3 < 0)
    {
        perror("The creation of the parent process was unsuccessful");
        exit(1);
    }
    if(pid3 == 0)
    {
        // Register the signal handler for SIGINT
        signal(SIGINT, handle_signal_int);
        
        //Reset the local balance
        int localBalance = 0;
        //Run indefinitely
        while(!sentinel)
        {
            //If the SIGINT signal is received, then break the loops
            if(sentinel) break;
            //Sleep some random amount of time between 0 - 5 seconds
            sleep(rand() % 5 + 0);
            printf("Dear Old Dad: Attempting to Check Balance\n");

            //Set a random number between 0 - 100 for Dad's decision and the possible deposited amount 
            int decision = rand() % 100 + 0;
            int amount = rand() % 100 + 0;

            //Critical Section #5; Process needs to use the shared memory
            sem_wait(semaphore);
            //Copy the contents of the BankAccount to the localBalance
            localBalance = *BankAccount;
            //Allow another process to use the shared memory
            sem_post(semaphore);

            //If decision is even, add it to the account
            if(decision % 2 == 0)
            {
                //If the account is less than 100, add it to the account
                if(localBalance < 100)
                {
                    //If amount being deposited is even, add it to the account
                    if(amount % 2 == 0)
                    {
                        //Critical Section #6; Process needs to use the shared memory
                        sem_wait(semaphore);
                        //Add the amount to the account
                        localBalance += amount;
                        *BankAccount = localBalance;
                        //Allow another process to use the shared memory
                        sem_post(semaphore);

                        //Display the transaction
                        printf("Dear old Dad: Deposits: $%d / Balance = $%d\n", amount, localBalance);

                    }
                    else
                    {
                        //If amount deposited is odd, print that it doesn't have any money to give
                        printf("Dear old Dad: Doesn't have any money to give\n");
                    }
                }
                else
                {
                    //If the account is greater than 100, print that the student has enough cash
                    printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
                }
            }
            else
            {
                //If decision is odd, print that the student doesn't have enough cash
                printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
            }
        }
    }

    // Wait for all processes to terminate by checking the status of the child processes. 
    //If the child process is terminated, then the returned value is the process ID of the terminated child process.
    //If the child process is not terminated, then the returned value is less than or equal 0.
    while(wait(NULL) > 0);
    
    //Free and clean all traces of memory and resources of the program, respectvely
    printf("Signal Interruption Detected...Cleaning up...\n");
    sem_close(semaphore);
    sem_unlink(SEMAPHRONE_NAME);
    shmdt(shared_mem_ptr);
    shmctl(shared_mem_id, IPC_RMID, NULL);
    return 0;
}
