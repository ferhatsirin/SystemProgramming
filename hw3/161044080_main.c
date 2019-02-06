#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define BUFSIZE 1024
#define FIFONAME "fifoFile"

struct stackNode{
    char data[BUFSIZE];
    struct stackNode *next;
}*root;


int runCommand(char *command);
void sigHandler(int sigNo);
void cleanCode();
void emptyStack();
struct stackNode* newNode(const char data[]);
void push(struct stackNode **root, const char data[]);
char * pop(struct stackNode **root);

char mainDirectory[BUFSIZE];
int mainSize;

int main(int argc, char** argv) { 
    
    char cmd[BUFSIZE]; // command
    char *charP, *directory;
    int childPid;
    int status;
    getcwd(mainDirectory,BUFSIZE);
    mainSize =strlen(mainDirectory);
    root =NULL;
    sigset_t fullSet, termSet;
    sigfillset(&fullSet);
    sigfillset(&termSet);
    sigdelset(&termSet,SIGINT);
    sigdelset(&termSet,SIGTERM);
    struct sigaction sa;
    sa.sa_flags =0;
    sa.sa_handler =&sigHandler;
    sigaction(SIGINT,&sa,NULL);
    sigaction(SIGTERM,&sa,NULL);
    
    atexit(&cleanCode);
                   
    mode_t mod =S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP;
    if(mkfifo(FIFONAME,mod) == -1){
        perror("Error ");
        exit (EXIT_FAILURE);
    }
    
    for(;;){
        getcwd(cmd,BUFSIZE);
        directory =strtok(cmd,"/");
        while(( charP=strtok(NULL,"/")) !=NULL){
            directory =charP;
        }
        printf("~%s$ ",directory);
        
        if(fgets(cmd,sizeof(cmd),stdin) == NULL){       
            fprintf(stderr,"Invalid input!!!\n");
            continue;
        }
        
        sigprocmask(SIG_SETMASK,&fullSet,NULL);
        
        cmd[strlen(cmd)-1] ='\0';  // discard '\n'
        if(cmd[0] == '\33' && cmd[2] == 'A'){ // up arrow
            charP =strtok(cmd,"^[[A");
            while(strtok(NULL,"^[[A") !=NULL){
                pop(&root);
            }
            childPid =runCommand(pop(&root));
        }
        else{
          push(&root,cmd);
          childPid =runCommand(cmd);
        }
                  
        if(childPid > 0){ // wait process to finish its job
            waitpid(childPid,&status,0);
        }
        else if(childPid < 0){ // error case
            perror("Error ");            
        }
        sigprocmask(SIG_SETMASK,&termSet,NULL);
    }
    
    return (EXIT_SUCCESS);
}

int runCommand(char *command){
    
    if(command ==NULL){
        return 0;
    }    
    else if(strncmp(command,"cd",2) == 0){
        return chdir(&command[3]);
    }
    else if(strcmp(command,"exit") == 0){
        exit (EXIT_SUCCESS);
    }   
    else if(strncmp(command,"wc",2) == 0 || strncmp(command,"cat",3) == 0 || strncmp(command,"ls",2) == 0 || strncmp(command,"pwd",3) == 0 || strncmp(command,"help",4) == 0){        
        char *argv[4]; int pipe =0;
        char nextC[BUFSIZE]; char cwd[BUFSIZE]; char fifoLink[BUFSIZE];
                      
        argv[0] =strtok(command," ");
        if(strcmp(command,"pwd") == 0){
            argv[0] ="prt";
        }
        if(strcmp(argv[0],"ls") == 0 || strcmp(command,"pwd") == 0){
            getcwd(cwd,sizeof(cwd));
            argv[1] =cwd;
        }
        else if(strcmp(argv[0],"help") == 0){
            argv[0] ="prt";
            snprintf(cwd,BUFSIZE,"%s\n%s\n%s\n%s\n%s\n%s\n%s","- ls; which will list file type, access rights, file size and file name of all files in the present working directory",
                "- pwd; which will print the present working directory",
                "- cd; which will change the present working directory to the location provided as argument",
                "- help; which will print the list of supported commands",
                "- cat; which will print on standard output the contents of the file provided to it as argument",
                "- wc; which will print on standard output the number of lines in the file provided to it as argument",
                "- exit; which will exit the shell");
            argv[1] =cwd;
        }
        else{
            argv[1] =strtok(NULL," ");
        }
        command =strtok(NULL," ");
        if(command != NULL){
            if(strcmp(command,">") == 0 ){
                argv[2] =strtok(NULL," ");
                argv[3] =NULL;
            }
            else if(strcmp(command,"|") == 0){
                pipe =1;
         
                mainDirectory[mainSize] ='\0';
                strncpy(fifoLink,mainDirectory,BUFSIZE);
                strncat(fifoLink,"/",BUFSIZE);
                strncat(fifoLink,FIFONAME,BUFSIZE);
                
                argv[2] =fifoLink;
                argv[3] =NULL;
                
                strncpy(nextC,strtok(NULL," "),BUFSIZE);
                strncat(nextC," ",BUFSIZE);
                strncat(nextC,fifoLink,BUFSIZE);
                command =strtok(NULL,"");
                if(command != NULL){
                    strncat(nextC," ",BUFSIZE);
                    strncat(nextC,command,BUFSIZE);
                }
            }
            else{
                argv[2] =NULL;
            }
        }
        else{
            argv[2] =NULL;
        }
        int childPid =fork();
       
        if(childPid == 0){
            mainDirectory[mainSize] ='\0';
            strncat(mainDirectory,"/",BUFSIZE);
            strncat(mainDirectory,argv[0],BUFSIZE);
            execve(mainDirectory,argv,NULL);
            perror("Error ");
            emptyStack();
            _exit(EXIT_FAILURE);
        }
        if(pipe ==1){       
            return runCommand(nextC);
        }
        else{
            return childPid;
        }
    }
    
    else{    
        fprintf(stderr,"No command '%s' found\n",command);
        return 0;
    }
    
    return 0;
}

void sigHandler(int sigNo){
    exit (EXIT_SUCCESS);
}
void cleanCode(){
    emptyStack();
             
    mainDirectory[mainSize] ='\0';
    strncat(mainDirectory,"/",BUFSIZE);
    strncat(mainDirectory,FIFONAME,BUFSIZE);
    unlink(mainDirectory);
}

void emptyStack(){
    while(pop(&root) !=NULL); 
}

void push(struct stackNode **root, const char data[]){

    struct stackNode *temp =newNode(data);
    temp->next =*root;
    *root =temp;
}

char * pop(struct stackNode **root){
    if((*root) ==NULL){
        return NULL;
    }
    
    struct stackNode *temp =*root;
    *root =(*root)->next;
    char *str =temp->data;
    free(temp);
    
    return str;
}

struct stackNode* newNode(const char data[]){

    struct stackNode *node =(struct stackNode*)malloc(sizeof(struct stackNode));
    strncpy(node->data,data,BUFSIZE);
    node->next =NULL;
    return node;
}

