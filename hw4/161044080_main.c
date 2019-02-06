#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>

#define N 6

enum Ingredients{
    cake =0,
    sugar =1,
    flour =2,
    butter =4,
    egg =8,
};

enum Condition{
    sugarFlour =3,
    sugarButter =5,
    sugarEgg =9,
    flourButter =6,
    flourEgg =10,
    butterEgg =12
};

char *printEnum(enum Ingredients prd);
void assignProduct(enum Ingredients product[],int cond);
void closeFiles();
void unLinkFiles();
void exitChef();
void exitSaler();

char *semName[2] ={"semCond","semCake"};
char *sharedMem="shared";
sem_t *semCond, *semCake;
int shmFd;
enum Ingredients *product;
    
int main(int argc, char** argv) {
    
    int chefNo =0;
    int childP; int randIndex;
    enum Ingredients exist[2], lack[2];
    int choice[6]={3,5,9,6,10,12};
    
    shmFd =shm_open(sharedMem,O_RDWR | O_CREAT | O_TRUNC,S_IRUSR|S_IWUSR);
    if(shmFd == -1){
        perror("Error ");
        exit (EXIT_FAILURE);
    }
    
    if (ftruncate(shmFd,2*sizeof(enum Ingredients)) == -1){
        perror("Error ");
        exit (EXIT_FAILURE);
    }
 
    product =mmap(NULL,2*sizeof(enum Ingredients),PROT_READ | PROT_WRITE,MAP_SHARED,shmFd,0);
    if(product == MAP_FAILED){
        perror("Error ");
        exit(EXIT_FAILURE);
    }
    
    semCond =sem_open(semName[0],O_RDWR| O_CREAT,S_IRUSR | S_IWUSR,0);
    semCake =sem_open(semName[1],O_RDWR| O_CREAT,S_IRUSR | S_IWUSR,0);
    
    if(semCond == SEM_FAILED || semCake ==SEM_FAILED){
        perror("Error ");
        exit (EXIT_FAILURE);
    }
   
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    setbuf(stdout,NULL);
    
    while(chefNo < N){  // chefs
        childP =fork();
        if(childP == -1){
            perror("Error ");
            exit (EXIT_FAILURE);
        }
        
        if(childP == 0){  // Chefs         
            sa.sa_handler =&exitChef;
            sigaction(SIGINT,&sa,NULL);
            
            assignProduct(lack,choice[chefNo]);
            assignProduct(exist,15-choice[chefNo]);
            
            printf("chef%d has an endless supply of %s and %s but lacks %s and %s\n",chefNo,
                    printEnum(exist[0]),printEnum(exist[1]),printEnum(lack[0]),printEnum(lack[1]));
                           
            printf("chef%d is waiting for %s and %s\n",chefNo,printEnum(lack[0]),printEnum(lack[1]));
            while(1){                            
                sem_wait(semCond);
                if(product[0] == lack[0] && product[1] == lack[1]){

                    printf("chef%d has taken the %s\n",chefNo,printEnum(lack[0]));
                    printf("chef%d has taken the %s\n",chefNo,printEnum(lack[1]));

                    printf("chef%d is preparing the dessert\n",chefNo);
                    printf("chef%d has delivered the dessert to the wholesaler\n",chefNo);
                    sem_post(semCake);
                    printf("chef%d is waiting for %s and %s\n",chefNo,printEnum(lack[0]),printEnum(lack[1]));
                }
                else{
                    sem_post(semCond);
                }
            }
               
            _exit (EXIT_SUCCESS);
            
        }
        else{
            ++chefNo;
        }
    }
    
    // Wholesaler
            
    srand(time(NULL));
    sa.sa_handler =&exitSaler;
    sigaction(SIGINT,&sa,NULL);

    while(1){
        
        randIndex =rand()%6;
        assignProduct(product,choice[randIndex]);
        printf("wholesaler delivers %s and %s\n",printEnum(product[0]),printEnum(product[1]));
        sem_post(semCond);
        
        printf("wholesaler is waiting for the desert\n");
        sem_wait(semCake);
        printf("wholesaler has obtained the dessert and left to sell it\n");
    }   

    return (EXIT_SUCCESS);
}

void exitChef(){
    closeFiles();
    _exit(EXIT_SUCCESS);
}

void exitSaler(){
    closeFiles();
    unLinkFiles();
    exit (EXIT_SUCCESS);
}

void closeFiles(){
    sem_close(semCond);
    sem_close(semCake);
    close(shmFd);
    munmap(product,2*sizeof(enum Ingredients));
}

void unLinkFiles(){ 
    int i;
    for(i=0;i<2;++i){
        sem_unlink(semName[i]);
    }
    shm_unlink(sharedMem);
}

char *printEnum(enum Ingredients prd){
    switch(prd){
    
        case sugar :
            return "sugar";
        case flour :
            return "flour";
        case butter :
            return "butter";
        case egg :
            return "egg";
        case cake:
            return "cake";
    }
    return NULL;
}

void assignProduct(enum Ingredients product[],int cond){
    int i;
    for(i=0;i<2;++i){
        if(cond & sugar){
            product[i] =sugar;
            cond -=sugar;
        }
        else if(cond & flour){
            product[i] =flour;
            cond -=flour;
        }
        else if(cond & butter){
            product[i] =butter;
            cond -=butter;
        }
        else{
            product[i] =egg;
            cond -=egg;
        }
    }
}


