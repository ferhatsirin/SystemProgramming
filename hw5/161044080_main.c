#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#define BUFSIZE 100

struct Client{

    char name[BUFSIZE];
    int cordX;
    int cordY;
    char request[BUFSIZE];
};

struct Queue
{
    int front;
    int rear;
    int size;
    int capacity;
    struct Client array[BUFSIZE];
};

struct Florist{
    char name[BUFSIZE];
    int cordX;
    int cordY;
    double speed;
    int open;
    char type[BUFSIZE][BUFSIZE];
    int typeSize;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    struct Queue client;
};

struct SaleStatistic{
    char name[BUFSIZE];
    int time;
    int count;
};

int isFull(struct Queue* queue);
int isEmpty(struct Queue* queue);
void enqueue(struct Queue* queue, struct Client *item);
struct Client* dequeue(struct Queue* queue);
double distance(struct Florist *florist,struct Client *client);
char *readFlorist(struct Florist **florist,int *size,char *file);
int isThere(struct Florist *florist,struct Client *client);
void *flowerShop(void *flor);

int main(int argc, char** argv) {
    
     if(argc != 2 ){
        fprintf(stderr,"Wrong line argument enter only the file name");
        exit(EXIT_FAILURE);
    }
    int fd =open(argv[1],O_RDONLY);
    
    if(fd == -1){
        fprintf(stderr,"File does not exist or can not be read ");
        exit (EXIT_FAILURE);
    }
    // resources 
    char *file;
    struct stat fileInfo;
    
    if(stat(argv[1],&fileInfo) < 0){
        fprintf(stderr,"Program can't read file information");
        exit (EXIT_FAILURE);
    }

    file =(char*)malloc(fileInfo.st_size);
    
    int fileSize =read(fd,file,fileInfo.st_size);
    if(fileSize != fileInfo.st_size){
        fprintf(stderr,"The program could not read the whole file");
        exit(EXIT_FAILURE);
    }
    else{
        close(fd);
    }
    
    struct Florist *florist;
    struct Florist *min;
    struct Client client;
    struct SaleStatistic **statistics;
    pthread_t *flowerShops;
    int size,i;
    char *line;
    setbuf(stdout,NULL);
    srand(time(NULL));
    printf("Florist application initializing from file : %s\n", argv[1]);
    line =readFlorist(&florist,&size,file);
    printf("%d florists have been created\n",size);
    flowerShops =malloc(sizeof(pthread_t)*size);
    statistics =(struct SaleStatistic**)malloc(sizeof(struct SaleStatistic*)*size);
    for(i=0;i<size;++i){
        pthread_create(&flowerShops[i],NULL,flowerShop,&florist[i]);
    }
    printf("Processing requests\n");
    while(line !=NULL){
        sscanf(line,"%s (%d,%d): %s",client.name,&client.cordX,&client.cordY,client.request);
        min =NULL;
        for(i=0;i<size;++i){
            if(isThere(&florist[i],&client) && (min ==NULL || (distance(&florist[i],&client) < distance(min,&client)))){
                min =&florist[i];
            }
        }
        pthread_mutex_lock(&min->mutex);
        enqueue(&min->client,&client);
        pthread_cond_signal(&min->cond);
        pthread_mutex_unlock(&min->mutex);
        line =strtok(NULL,"\n");        
    }
    
    for(i=0;i<size;++i){
        pthread_mutex_lock(&florist[i].mutex);
        florist[i].open =0;
        pthread_cond_signal(&florist[i].cond);
        pthread_mutex_unlock(&florist[i].mutex);
        pthread_join(flowerShops[i],(void**)&statistics[i]);
        pthread_mutex_destroy(&florist[i].mutex);
        pthread_cond_destroy(&florist[i].cond);
    }
    
    printf("-------------------------------------------------\n");  
    printf("Florist%10c# of sales%10cTotal time\n",' ',' ');
    printf("-------------------------------------------------\n");
    for(i=0;i<size;++i){
    
        printf("%-10s%10c%-10d%10c%d ms\n",statistics[i]->name,' ',statistics[i]->count,' ',statistics[i]->time);
    }
    
    free(florist);
    free(flowerShops);
    for(i=0;i<size;++i)
        free(statistics[i]);
    free(statistics);
    free(file);
    close(fd);

    return (EXIT_SUCCESS);
}

void *flowerShop(void *flor){
    struct Florist *florist =(struct Florist*)flor;
    struct Client *client;
    struct SaleStatistic *statistic =malloc(sizeof(struct SaleStatistic));
    int time;
    statistic->time =0;
    statistic->count =0;
    sscanf(florist->name,"%s",statistic->name);
    while(1){
        pthread_mutex_lock(&florist->mutex);
        while(isEmpty(&florist->client)){
            if(!florist->open){
                pthread_mutex_unlock(&florist->mutex);
                pthread_exit((void*)statistic);
            }
            pthread_cond_wait(&florist->cond,&florist->mutex);
        }
        client =dequeue(&florist->client);
        pthread_mutex_unlock(&florist->mutex);
        printf("Florist %s has got the request %s %s\n",florist->name,client->name,client->request);
        time =rand()%41+10 +distance(florist,client)/florist->speed;
        printf("Florist %s has delivered a %s to %s in %d ms \n",florist->name,client->request,client->name,time);
        statistic->time +=time;
        statistic->count++;
    }
}

char *readFlorist(struct Florist **florist,int *size,char *file){
    char *line, *ptr;
    int where =strchr(file,'\n')-file;
    
    line =strtok(file,"\n");
    
    *size =0;
    (*florist) =NULL;
    while(strncmp(line,"client",6) != 0 && line !=NULL){
        (*florist) =realloc((*florist),(*size+1)*sizeof(struct Florist));
        sscanf(line,"%s (%d,%d; %lf)",(*florist)[*size].name,
                &(*florist)[*size].cordX,&(*florist)[*size].cordY,
                        &(*florist)[*size].speed);
        
        (*florist)[*size].typeSize =0;
        line =strchr(line,':');
        ++line;
        while(line !=NULL){
            ++line;
            sscanf(line,"%s",(*florist)[*size].type[(*florist)[*size].typeSize]);
            ptr =strchr((*florist)[*size].type[(*florist)[*size].typeSize],',');
            if(ptr !=NULL){
                *ptr='\0';
            }
            (*florist)[*size].typeSize++;
            line =strchr(line,' ');
        }
        (*florist)[*size].client.capacity =BUFSIZE;
        (*florist)[*size].client.front =0;
        (*florist)[*size].client.rear =(*florist)[*size].client.capacity-1;
        (*florist)[*size].client.size =0;
        (*florist)[*size].open =1;
        pthread_cond_init((&(*florist)[*size].cond),NULL);
        pthread_mutex_init((&(*florist)[*size].mutex),NULL);
        *size +=1;
        
        line=strtok(NULL,"\n");
    }
    return line;
}

int isThere(struct Florist *florist,struct Client *client){
    int i;
    for(i=0;i<florist->typeSize;++i){
        if(strcmp(florist->type[i],client->request) == 0){
            return 1;
        }
    }
    return 0;
}

double distance(struct Florist *florist,struct Client *client){

    double x =client->cordX-florist->cordX;
    double y =client->cordY-florist->cordY;
    return sqrt(x*x+y*y);
}

 
// Queue is full when size becomes equal to the capacity 
int isFull(struct Queue* queue){ 
    return (queue->size == queue->capacity);  
}
 
// Queue is empty when size is 0
int isEmpty(struct Queue* queue){
    return (queue->size == 0); 
}
 
// Function to add an item to the queue.  
// It changes rear and size
void enqueue(struct Queue* queue, struct Client *item){
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1)%queue->capacity;
    queue->array[queue->rear] =*item;
    queue->size = queue->size + 1;
}
 
// Function to remove an item from queue. 
// It changes front and size
struct Client* dequeue(struct Queue* queue){
    if (isEmpty(queue))
        return NULL;
    struct Client *item = &queue->array[queue->front];
    queue->front = (queue->front + 1)%queue->capacity;
    queue->size = queue->size - 1;
    return item;
}
