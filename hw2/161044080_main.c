#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

double* sequence, *Xre, *Xim;;
int fileDesc;

int takeInputs(char **argv,char** fileName,int* seqLength,int* length);
void calculateTransform(double *seq,double *Xre,double *Xim,int length);
void parentExit(int sigNo);
void childExit(int sigNo);

int main(int argc, char** argv) {
    
    if(argc !=7){
    
        fprintf(stderr,"Command line argument error \n");
        fprintf(stderr,"It should be like ./progName -N 5 -X file.dat -M 100");
        exit (EXIT_FAILURE);
    }
    // resources
    int seqLength, length;
    char* fileName;  
    int childP, currentPos, i, seqSize, req;
   
    if ( takeInputs(argv,&fileName,&seqLength,&length) == -1){       
        fprintf(stderr,"Command line argument error \n");
        fprintf(stderr,"It should be like ./progName -N 5 -X file.dat -M 100");
        exit (EXIT_FAILURE);
    }
    int flags = O_RDWR | O_CREAT | O_TRUNC;
    mode_t modes =S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP;
    
    fileDesc = open(fileName,flags,modes);
        
    if(fileDesc == -1){
        fprintf(stderr,"File %s can not be opened ",fileName);
        exit (EXIT_FAILURE);
    }
  
    sequence =(double*)malloc(sizeof(double)*seqLength);
    Xre =(double*)malloc(sizeof(double)*seqLength);
    Xim =(double*)malloc(sizeof(double)*seqLength);
    if(sequence == NULL || Xre == NULL || Xim == NULL){
    
        fprintf(stderr,"malloc() error ");
        exit (EXIT_FAILURE);
    }
    seqSize =sizeof(double)*seqLength;
    
    sigset_t set;
    sigfillset(&set);
    
    sigset_t exitMask;
    sigfillset(&exitMask);
    sigdelset(&exitMask,SIGCHLD);
           
    struct sigaction sigExit;  
    memset(&sigExit,0,sizeof(sigExit));
    sigExit.sa_flags =0;
    sigemptyset(&sigExit.sa_mask);
    
    struct flock lock;
    memset(&lock,0,sizeof(lock));
    
    setvbuf(stdout,NULL,_IONBF,0);
         
    childP =fork();
    if(childP == -1){
        fprintf(stderr,"fork() error ");
        exit (EXIT_FAILURE);
    }
    
    if(childP != 0){ // parent process   
        sigExit.sa_handler =&parentExit;
        sigaction(SIGCHLD,&sigExit,NULL);
        srand(time(NULL));

        for(;;){             
            for(;seqSize*length <= lseek(fileDesc,0,SEEK_CUR);){ 
            }
            
            for(i= 0; i<seqLength;++i){
                sequence[i] = (double)rand() /(RAND_MAX/100.00);
            }
            sigprocmask(SIG_SETMASK,&set,NULL);  // start to write file
            lock.l_type =F_WRLCK;
            req =fcntl(fileDesc,F_SETLK,&lock);
            if(req != -1){
            
		write(fileDesc,sequence,seqSize);
		currentPos =lseek(fileDesc,0,SEEK_CUR); 
		    
	        printf("Process A: i’m producing a random sequence for line %2d: ",currentPos/seqSize);
                for(i=0;i<seqLength;++i){                
		        printf("%5.2lf ",sequence[i]);
		}
		printf("\n");
		                
	        lock.l_type =F_UNLCK;
		fcntl(fileDesc,F_SETLK,&lock); 
            }
            sigprocmask(SIG_SETMASK,&exitMask,NULL);
        }
    }
    else{  // child process 
        sigExit.sa_handler =&childExit;          
        sigaction(SIGINT,&sigExit,NULL);
        
        for(;;){
            
            for(;lseek(fileDesc,0,SEEK_CUR) <= 0;){
            }
            
            sigprocmask(SIG_SETMASK,&set,NULL);        // start to read file               
            lock.l_type =F_WRLCK;
            req =fcntl(fileDesc,F_SETLK,&lock);                   
            if(req !=-1){
		currentPos =lseek(fileDesc,seqSize*-1,SEEK_CUR);  
		read(fileDesc,sequence,seqSize);
		ftruncate(fileDesc,currentPos);
		lseek(fileDesc,seqSize*-1,SEEK_CUR);  
		               
		printf("Process B: the DFT of line %d ( ",currentPos/seqSize+1);
		for(i= 0; i<seqLength;++i){
			printf("%5.2lf ",sequence[i]);
		}
		printf(") is: ");
		calculateTransform(sequence,Xre,Xim,seqLength);
		for(i =0;i<seqLength;++i){
		        printf("%5.2lf%+.2lfi ",Xre[i],Xim[i]);
		}
                printf("\n");
		                
	        lock.l_type =F_UNLCK;
		fcntl(fileDesc,F_SETLK,&lock);
            }  
            sigprocmask(SIG_UNBLOCK,&set,NULL);
        }
    }
       
    free(sequence);
    
    if(close(fileDesc)== -1){
        fprintf(stderr,"File close error ");
        exit(EXIT_FAILURE);
    }

    return (EXIT_SUCCESS);
}

void calculateTransform(double *seq,double *Xre,double *Xim,int length){
    int i,j;
    for(i =0;i<length;++i){
        Xre[i] =0.0; Xim[i] =0.0;
        for(j=0;j<length;++j){
            Xre[i] +=seq[j]*cos(i*j*2*M_PI/(double)length);
            Xim[i] -=seq[j]*sin(i*j*2*M_PI/(double)length);
        }
    }
}

void parentExit(int sigNo){

    free(sequence);    
    free(Xre);
    free(Xim);
    
    if(close(fileDesc) == -1){
        exit (EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}

void childExit(int sigNo){
    
    free(sequence);
    free(Xre);
    free(Xim);
    
    if(close(fileDesc) == -1){
        _exit (EXIT_FAILURE);
    }
    
    _exit(EXIT_SUCCESS);
}

int takeInputs(char **argv,char** fileName,int* seqLength,int* length){

    *seqLength =0; *length =0; *fileName =NULL;
    for(int i=1;i<7;i +=2){    
        if(strcmp(argv[i],"-N") == 0 || strcmp(argv[i],"-n") == 0){
        
            *seqLength =strtol(argv[i+1],NULL,10);
        }
        else if(strcmp(argv[i],"-M") == 0 || strcmp(argv[i],"-m") == 0){
        
            *length =strtol(argv[i+1],NULL,10);
        }
        else if(strcmp(argv[i],"-X") == 0 || strcmp(argv[i],"-x") == 0){
        
            *fileName =argv[i+1];
        }
        else{
        
            return -1;
        }
    }
    if(*seqLength <= 0 || length <= 0 || *fileName ==NULL){
    
        return -1;
    }
    else
        return 0;
}

