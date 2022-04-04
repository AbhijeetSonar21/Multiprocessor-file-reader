/*************************************************
 * C program to count no of lines, words and *
 * characters in a file. *
 *************************************************/

#include <stdio.h>

#include <unistd.h>

#include <math.h>

#include <stdlib.h>

#include <sys/wait.h>

#include<string.h>

#include <signal.h>

#include <sys/types.h>


#define MAX_PROC 100
#define MAX_FORK 1000
int CRASH = 0;
//int Lodu = 0;
int status;

// int flag;
typedef struct count_t {
    int linecount;
    int wordcount;
    int charcount;
    int s_count;
}
count_t;

count_t word_count(FILE * fp, long offset, long size) {

    char ch;
    long rbytes = 0;

    count_t count;
    // Initialize counter variables
    count.linecount = 0;
    count.wordcount = 0;
    count.charcount = 0;
    count.s_count = 0;


    //printf("\n \n[pid %d] reading %ld bytes from offset %ld\n", getpid(), size, offset);

    if (fseek(fp, offset, SEEK_SET) < 0) {
        printf("[pid %d] fseek error!\n", getpid());
    }

    while ((ch = getc(fp)) != EOF && rbytes < size) {
       // printf("%c",ch);
        // Increment character count if NOT new line or space
        if (ch != ' ' && ch != '\n') {
            ++count.charcount;
        }

        // Increment word count if new line or space character
        if (ch == ' ' || ch == '\n') {
            ++count.wordcount;
        }

        // Increment line count if new line character
        if (ch == '\n') {
            ++count.linecount;
        }
        rbytes++;
        count.s_count++;
    }

    srand(getpid());
    if (CRASH > 0 && (rand() % 100 < CRASH)) {

        printf("[pid %d] crashed.\n", getpid());
        abort();

    }

    return count;
}

int main(int argc, char ** argv) 
{
    long fsize;
    FILE * fp;
    int failed_process[200];
    int numJobs;
    count_t total, count;
    int i, pid, status;
    int nFork = 0;
    int pipefd[2];
    pipe(pipefd);
    //int pipefd1[2];
    //int pipefd2[2];
    //char buffer[256];
    //int under_flag;
    int crash_counter=0;

    if (argc < 3) {
        printf("usage: pa <# of processes> <filname>\n");
        return 0;
    }
    if (argc > 3) {
        CRASH = atoi(argv[3]);
        if (CRASH < 0) CRASH = 0;
        if (CRASH > 50) CRASH = 50;
    }
    printf("CRASH RATE: %d\n", CRASH);

    numJobs = atoi(argv[1]);

    if (numJobs > MAX_PROC) numJobs = MAX_PROC;

    count.linecount = 0;
    count.wordcount = 0;
    count.charcount = 0;

    fp = fopen(argv[2], "r");

    if (fp == NULL) {
        printf("File open error: %s\n", argv[1]);
        printf("usage: pa <filname>\n");
        return 0;
    }

    fseek(fp, 0L, SEEK_END);
    fsize = ftell(fp);

    fclose(fp);

    
    long child_offset[1000];
    long wordcountarray[10];
    long charcountarray[10];
    long linecountarray[10];

    //int wordstr;  
    char wordstr_pipe[10];
    //int charstr;
    char charstr_pipe[10];
    //int linestr;
    char linestr_pipe[10];
    //int Extra = 0;
    int pipe_array[3];
    
    int num=numJobs;
    crash_counter=num;
    long ChildSize1 = (fsize / num);
    
    int inc_crash=0;
    int counter_failure=0;
    int cd[100];
    int accessed=0;
    
    for (i = 0; i < numJobs; i++) 
    {
        int i_replica=i;
        
        if(counter_failure>0 && counter_failure==accessed)
        {
            break;
        }
        inc_crash=i_replica;
        if(i>num)
        {
            i_replica=cd[accessed];
            inc_crash=i_replica;
            //printf("\n Executing again this Process %d \n numjobs=%d \n counter_failure=%d \n cd[accessed]=%d\n",i_replica,numJobs,counter_failure,cd[accessed]);
            accessed++;
        }
        child_offset[i] = (ChildSize1) * i_replica;

        // printf("\nExecuted this Process %d, numjobs=%d, counter_failure=%d, i=%d\n",i_replica,numJobs,counter_failure,i);

        

        if (nFork++ > MAX_FORK) return 0;
        pid = fork();
        if (pid < 0) 
        {
            printf("Fork failed.\n");
        } 
        else if (pid == 0) 
        {
            fp = fopen(argv[2], "r");
            fseek(fp, 0L, SEEK_END);

            if (i < numJobs ) 
            {
                count = word_count(fp, child_offset[i], ChildSize1);
            }
            if (i == num - 1) 
            {
                count = word_count(fp, child_offset[i], (fsize - child_offset[i]));
            }
            wordcountarray[i]=0;
            charcountarray[i]=0;
            linecountarray[i]=0;

            wordcountarray[i] = count.wordcount;
            charcountarray[i] = count.charcount;

            linecountarray[i] = count.linecount;

            pipe_array[0] = wordcountarray[i];
            pipe_array[1] = charcountarray[i];
            pipe_array[2] = linecountarray[i];

            
            
            write(pipefd[1], pipe_array, sizeof(int) * 3);
            fclose(fp);
            close(pipefd[1]);
            return 0;
            
        } 
        else if (pid > 0) 
        {
            wait( & status);
            if (WIFEXITED(status)) // normal termination
            {
                read(pipefd[0], pipe_array, sizeof(int) * 3);
                count.linecount = count.linecount + pipe_array[2];
                count.wordcount = count.wordcount + pipe_array[0];
                count.charcount = count.charcount + pipe_array[1];

            }
            if (WIFSIGNALED(status)) // abnormal termination
            {
                
                if(counter_failure==0)
                {
                    numJobs++;
                }
                cd[counter_failure]=i;
                if(i>num)
                {
                    cd[counter_failure]=inc_crash;
                }
                
                counter_failure++;
                numJobs++;

                
                //printf("\n ''''Adding this Process %d, numjobs=%d, counter_failure=%d\n''''",i,numJobs,counter_failure);
                
            }

            
        }
    }

    printf("\n=========================================\n");
    printf("Total Lines : %d \n", count.linecount);
    printf("Total Words : %d \n", count.wordcount);
    printf("Total Characters : %d \n", count.charcount);
    printf("=========================================\n");
   
    return (0);
}