/*
    COMP3511 Spring 2023 
    PA1: Simplified Linux Shell (myshell)

    Your name:  HUANG, I Wei
    Your ITSC email:    ihuangaa@connect.ust.hk 

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks. 

*/

// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h> // For open/read/write/close syscalls

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LEN 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters: 
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements, 
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
// We also need to add an extra NULL item to be used in execvp
// Thus: 8 + 1 = 9
//
// Example: 
//   echo a1 a2 a3 a4 a5 a6 a7 
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT]; 
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the  Standard file descriptors here
#define STDIN_FILENO    0       // Standard input
#define STDOUT_FILENO   1       // Standard output 

// Define some templates for printf
#define TEMPLATE_MYSHELL_START "Myshell (pid=%d) starts\n"
#define TEMPLATE_MYSHELL_END "Myshell (pid=%d) ends\n"


 
// This function will be invoked by main()
// TODO: Implement the multi-level pipes below
void process_cmd(char *cmdline);

// This function will be invoked by main()
// TODO: Replace the shell prompt with your own ITSC account name
void show_prompt();


// This function will be invoked by main()
// This function is given. You don't need to implement it.
int get_cmd_line(char *cmdline);

// This function helps you parse the command line
// read_tokens function is given. You don't need to implement it.
//
// Suppose the following variables are defined:
//
// char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
// int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
// char cmdline[MAX_CMDLINE_LEN]; // The input command line
//
// Sample usage:
//
//  read_tokens(pipe_segments, cmdline, &num_pipe_segments, "|");
// 
void read_tokens(char **argv, char *line, int *numTokens, char *token);


/* The main function implementation */
int main()
{
    char cmdline[MAX_CMDLINE_LEN];
    printf(TEMPLATE_MYSHELL_START, getpid());

    // The main event loop
    while (1)
    {
        show_prompt();
        if (get_cmd_line(cmdline) == -1)
            continue; /* empty line handling */

        // Implement the exit command 
        if ( strcmp(cmdline, "exit") == 0 ) {
            printf(TEMPLATE_MYSHELL_END, getpid());
            exit(0);
        }

        pid_t pid = fork();
        if (pid == 0) {
            // the child process handles the command
            //process_cmd(cmdline);
            // the child process terminates without re-entering the loop
            exit(0); 
        } else {
            // the parent process simply waits for the child and do nothing
            process_cmd(cmdline);
            wait(0);
            // the parent process re-enters the loop and handles the next command
        }
    }
    return 0;
}

void process_cmd(char *cmdline)
{
    // Un-comment this line if you want to know what is cmdline input parameter
    // printf("The input cmdline is: %s\n", cmdline);
    // TODO: Write your program to handle the command
    char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
    int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
    read_tokens(pipe_segments, cmdline, &num_pipe_segments, "|");
    int numPipes = num_pipe_segments-1;

    char *command[MAX_PIPE_SEGMENTS][MAX_PIPE_SEGMENTS];
    int num_seg_args[MAX_PIPE_SEGMENTS];
    for(int i=0;i<num_pipe_segments;i++)
    {
        read_tokens(command[i],pipe_segments[i],&num_seg_args[i],SPACE_CHARS);
    }
    
    /* test
    printf("num pipe segements: %d\n\n",num_pipe_segments);

    for(int i=0;i<num_pipe_segments;i++)
    {
        for(int j=0;j<num_seg_args[i];j++)
        {
            printf("%s ",command[i][j]);
        }
        printf("\n");
        
    }
    */
    
    int status;
    //pid_t pid;
    //printf("%d\n",getpid());
    int pipefds[2*numPipes];

    // parent creates all needed pipes at the start 
    for(int i = 0; i < numPipes; i++ ){
        if( pipe(pipefds + i*2) < 0 ){
            perror("pipe create");
            exit(EXIT_FAILURE);
        }
    }

    int commandc = 0;
    //printf("num_pipe_segs: %d\n",num_pipe_segments);
    while(commandc<num_pipe_segments){
        pid_t pid=fork(); //pid = fork();
        if( pid == 0 ){

            //for redirection
            int red_index,in=0,out=0;
            char input[MAX_CMDLINE_LEN],output[MAX_CMDLINE_LEN];
            for(red_index=0;red_index<num_seg_args[commandc];red_index++)
            {
                if(strcmp(command[commandc][red_index],"<")==0)
                {
                    command[commandc][red_index]=NULL;
                    strcpy(input,command[commandc][red_index+1]);
                    in=2; 
                    continue;   
                }               
                if(strcmp(command[commandc][red_index],">")==0)
                {      
                    command[commandc][red_index]=NULL;
                    strcpy(output,command[commandc][red_index+1]);
                    out=2;
                    continue;
                }        
            }

            if(in)
            {   
                if ((pipefds[(commandc-1)*2] = open(input, O_RDONLY, 0)) < 0) {
                    perror("Couldn't open input file");
                    exit(EXIT_FAILURE);
                } 
                if( dup2(pipefds[(commandc-1)*2], 0) < 0){
                    //printf("error1\n");
                    perror("pipe in");
                    exit(EXIT_FAILURE);
                }
            }
            if(out)    //if '>' char was found in string inputted by user 
            {
                if ((pipefds[commandc*2+1] = creat(output, 0644)) < 0) {
                    perror("Couldn't open the output file");
                    exit(EXIT_FAILURE);
                }           
                if( dup2(pipefds[commandc*2+1], 1) < 0 ){
                    //printf("error2\n");
                    perror("pipe out");
                    exit(EXIT_FAILURE);
                }
            }

            // child gets input from the previous command, if it's not the first command 
            //printf("child\n");
            if( commandc!=0 ){
                if( dup2(pipefds[(commandc-1)*2], 0) < 0){
                    //printf("error1\n");
                    perror("pipe not first");
                    exit(EXIT_FAILURE);
                }
            }
            // child outputs to next command, if it's not the last command 
            if( commandc<num_pipe_segments-1){
                if( dup2(pipefds[commandc*2+1], 1) < 0 ){
                    //printf("error2\n");
                    perror("pipe not last");
                    exit(EXIT_FAILURE);
                }
            }
            for(int i = 0; i < 2*numPipes; i++){
                    close(pipefds[i]);
            }
            //printf("cmd: %s\n",command[commandc][0]);
            if(execvp(command[commandc][0],command[commandc])<0){
                //printf("error3\n");
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            //exit(0);
        } else if( pid < 0 ){
            //rintf("error4\n");
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        //printf("parent\n");
        //printf("%d loop: \n",commandc);
        commandc++;
        //exit(0);
    }

    // parent closes all of its copies at the end 
    for(int i = 0; i < 2 * numPipes; i++ ){
        close( pipefds[i] );
    }

    for(int i = 0; i < numPipes + 1; i++){
        wait(&status);
    }
    
}

// Implementation of read_tokens function
void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
    argv[argc] = NULL;
}

// Implementation of show_prompt
void show_prompt()
{
    // TODO: replace the shell prompt with your ITSC account name
    // For example, if you ITSC account is cspeter@connect.ust.hk
    // You should replace ITSC with cspeter
    printf("ihuangaa> ");
}

// Implementation of get_cmd_line
int get_cmd_line(char *cmdline)
{
    int i;
    int n;
    if (!fgets(cmdline, MAX_CMDLINE_LEN, stdin))
        return -1;
    // Ignore the newline character
    n = strlen(cmdline);
    cmdline[--n] = '\0';
    i = 0;
    while (i < n && cmdline[i] == ' ') {
        ++i;
    }
    if (i == n) {
        // Empty command
        return -1;
    }
    return 0;
}