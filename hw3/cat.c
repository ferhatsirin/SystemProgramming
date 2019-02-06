#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#define BUFSIZE 1024

int main(int argc, char** argv) {
    
    if(argc < 2){
        exit (EXIT_FAILURE);
    }
    int fdRead, fdWrite;
    fdRead =open(argv[1],O_RDONLY);
    if(fdRead ==-1){
        perror("Error ");
        exit (EXIT_FAILURE);
    }
    
    if(argc == 3){
        mode_t mod =S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP;
        fdWrite =open(argv[2],O_WRONLY | O_CREAT | O_TRUNC, mod);
        if(fdWrite == -1){
            perror("Error ");
            exit (EXIT_FAILURE);
        }
        dup2(fdWrite,STDOUT_FILENO);
    }
    
    char buf[BUFSIZE];
    int count,i;
    
    while((count =read(fdRead,buf,BUFSIZE)) >0){
        for(i =0;i<count;++i){
            printf("%c",buf[i]);
        }
    }

    if(close(fdRead) ==-1){
        perror("Error ");
        exit (EXIT_FAILURE);
    }
    if(argc == 3 && close(fdWrite) == -1){
        perror("Error ");
        exit (EXIT_FAILURE);
    }
    
    return (EXIT_SUCCESS);
}


