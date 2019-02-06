#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define BUFSIZE 1024

int main(int argc, char** argv) {

    if(argc < 2){
        exit (EXIT_FAILURE);
    }
    
    DIR *directory =opendir(argv[1]);
    if(directory == NULL){
        perror("Error ");
        exit (EXIT_FAILURE);
    }
    
    int fdWrite; 
    if(argc == 3){
        mode_t mod =S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP;
        fdWrite =open(argv[2],O_WRONLY | O_CREAT | O_TRUNC, mod);
        if(fdWrite == -1){
            perror("Error ");
            exit (EXIT_FAILURE);
        }
        dup2(fdWrite,STDOUT_FILENO);
    }
    
    struct dirent *directInfo;
    struct stat fileInfo;
    char filePath[BUFSIZE];
    
    while((directInfo =readdir(directory)) !=NULL){
    
        snprintf(filePath,BUFSIZE,"%s/%s",argv[1],directInfo->d_name);
        if(stat(filePath,&fileInfo) == -1){
            perror("Error ");
            exit (EXIT_FAILURE);
        }
        
        printf( (S_ISDIR(fileInfo.st_mode)) ? "d" : "-");
        printf( (fileInfo.st_mode & S_IRUSR) ? "r" : "-");
        printf( (fileInfo.st_mode & S_IWUSR) ? "w" : "-");
        printf( (fileInfo.st_mode & S_IXUSR) ? "x" : "-");
        printf( (fileInfo.st_mode & S_IRGRP) ? "r" : "-");
        printf( (fileInfo.st_mode & S_IWGRP) ? "w" : "-");
        printf( (fileInfo.st_mode & S_IXGRP) ? "x" : "-");
        printf( (fileInfo.st_mode & S_IROTH) ? "r" : "-");
        printf( (fileInfo.st_mode & S_IWOTH) ? "w" : "-");
        printf( (fileInfo.st_mode & S_IXOTH) ? "x" : "-");
        printf(" %d %s\n",(int)fileInfo.st_size,directInfo->d_name);
    }
    
    if(closedir(directory) == -1){
        perror("Error ");        
        exit (EXIT_FAILURE);
    }
     
    if(argc == 3 && close(fdWrite) == -1){
        perror("Error ");
        exit (EXIT_FAILURE);
    }
    
    return (EXIT_SUCCESS);
}
