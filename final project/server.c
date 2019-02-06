#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <pthread.h>

#define BUFSIZE 50

struct Client{
    char name[BUFSIZE];
    char priority;
    int request;
};
struct Answer{
    double result;
    int found;
    char providerName[BUFSIZE];
    int cost;
    double time;
    char errMsg[BUFSIZE];
};

struct Question{
    struct Client client;
    struct Answer answer;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};

struct Queue{
    int front;
    int rear;
    int size;
    int capacity;
    struct Question *array[2];
};

struct Provider{
    char name[BUFSIZE];
    int performance;
    int price;
    int duration;
    struct Queue client;
    int isOpen;
    int served;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    pthread_t thread;
};

volatile sig_atomic_t isServerOpen =1;
struct Provider **priceProvider, **timeProvider, **performanceProvider;
int numOfProvider =0;
FILE *logFile =NULL;

void readProvider(struct Provider **providers,int *size,char *file);
double factF(double n);
double cosF(int degree);
void *providerF(void * provide);
void *serverF(void * server);
void *wakerF(void* wake);
int isFull(struct Queue* queue);
int isEmpty(struct Queue* queue);
void enqueue(struct Queue* queue, struct Question *item);
struct Question* dequeue(struct Queue* queue);
void sigHandler(int sigNo);
void sortPrice(struct Provider **provider);
void sortPerformance(struct Provider **provider);
void sortTime(struct Provider **provider);
void print(FILE *file1,FILE *file2, char const *list, ...);

int main(int argc, char** argv) {
    
    if(argc != 4 ){
        print(stderr,NULL,"Wrong line argument enter port number, file name and log file name respectively\n");
        print(stderr,NULL,"Example: ./server 5555 final.dat log.dat\n");
        exit(EXIT_FAILURE);
    }
    int fd =open(argv[2],O_RDONLY);
    logFile =fopen(argv[3],"w");
    
    if(fd == -1){
        print(stderr,NULL,"File %s does not exist or can not be read\n",argv[2]);
        exit (EXIT_FAILURE);
    }
    if(logFile == NULL){
        print(stderr,NULL,"File %s does not exist or can not be created\n",argv[3]);
        exit (EXIT_FAILURE);
    }
    // resources 
    char *providerFile;
    struct stat fileInfo;
    
    if(stat(argv[2],&fileInfo) < 0){
        print(stderr,logFile,"Program can't read file information\n");
        exit (EXIT_FAILURE);
    }

    providerFile =(char*)malloc(fileInfo.st_size+1);
    
    int fileSize =read(fd,providerFile,fileInfo.st_size);
    if(fileSize != fileInfo.st_size){
        print(stderr,NULL,"The program could not read the whole file\n");
        exit(EXIT_FAILURE);
    }
    else{
        close(fd);
    }
    providerFile[fileSize] ='\0';
    srand(time(NULL)); // for rand()
    setbuf(stdout,NULL);
    setbuf(logFile,NULL);
    int i, sockfd, dataSocket, port, serverCount, optVal;
    socklen_t optLen;
    struct Provider *providers;
    pthread_t providerThread;
    pthread_t wakerThread;
    pthread_t *serverThread;
    struct sockaddr_in addr;
    
    port =atoi(argv[1]);
    readProvider(&providers,&numOfProvider,providerFile);
    free(providerFile);
    print(stdout,logFile,"%d provider threads created\n",numOfProvider);
    print(stdout,logFile,"Name  Performance  Price  Duration\n");
    for(i=0;i<numOfProvider;++i){
    
        print(stdout,logFile,"%-10s  %-5d  %3d  %5d\n",providers[i].name,providers[i].performance,
                providers[i].price,providers[i].duration);
    }
    priceProvider =malloc(sizeof(struct Provider*)*numOfProvider);
    timeProvider =malloc(sizeof(struct Provider*)*numOfProvider);
    performanceProvider =malloc(sizeof(struct Provider*)*numOfProvider);
    for(i=0;i<numOfProvider;++i){
        priceProvider[i] =&providers[i];
        performanceProvider[i] =&providers[i];
        timeProvider[i] =&providers[i];
    }
    sortPrice(priceProvider);
    sortPerformance(performanceProvider);
    sortTime(timeProvider);
    for(i=0;i<numOfProvider;++i){
        if(pthread_create(&providerThread,NULL,providerF,&providers[i]) !=0){
            print(stderr,logFile,"Thread couldn't be created \n");
            exit (EXIT_FAILURE);
        }
        providers[i].thread =providerThread;
    }

    if(pthread_create(&wakerThread,NULL,wakerF,NULL) !=0){
        print(stderr,logFile,"Thread couldn't be created \n");
        exit (EXIT_FAILURE);
    }
    
    sockfd =socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        print(stderr,logFile,"Socket couldn't be opened \n");
        exit (EXIT_FAILURE);           
    }
    optVal =1;
    optLen =sizeof(int);
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(void*)&optVal,optLen);
    memset(&addr,0,sizeof(struct sockaddr_in));
    addr.sin_family =AF_INET;
    addr.sin_port =htons(port);
    addr.sin_addr.s_addr =htonl(INADDR_ANY);
    if(bind(sockfd,(struct sockaddr*)&addr,sizeof(struct sockaddr_in)) == -1){
        print(stderr,logFile,"Server couldn't bind socket\n");
        exit (EXIT_FAILURE);       
    }
    if(listen(sockfd,numOfProvider) == -1){
        print(stderr,logFile,"Server couldn't listen socket\n");
        exit (EXIT_FAILURE);
    }
    print(stdout,logFile,"Server is waiting for client connections at port %d\n",port);
    
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    struct sigaction sa;
    memset(&sa,0,sizeof(struct sigaction));
    sa.sa_handler =sigHandler;
    sigaction(SIGINT,&sa,NULL);
    int *socketP;
    serverCount =0; serverThread =NULL;
    while(isServerOpen){ 
        dataSocket =accept(sockfd,NULL,NULL);
        if(dataSocket != -1){
            pthread_sigmask(SIG_BLOCK,&set,NULL);
            serverThread =realloc(serverThread,sizeof(pthread_t)*(serverCount+1));
            socketP =malloc(sizeof(int));
            *socketP =dataSocket;
            pthread_create(&serverThread[serverCount],NULL,serverF,socketP);
            ++serverCount;
            pthread_sigmask(SIG_UNBLOCK,&set,NULL); 
        }
    }
    print(stdout,logFile,"Termination signal received\n");
    print(stdout,logFile,"Terminating all provider\n");
    pthread_kill(wakerThread,SIGUSR1);
    pthread_join(wakerThread,NULL);
    
    for(i=0;i<serverCount;++i){
        pthread_join(serverThread[i],NULL);
    }

    close(sockfd);
    
    for(i=0;i<numOfProvider;++i){
        if(providers[i].isOpen){
            pthread_mutex_lock(&providers[i].mutex);
            providers[i].isOpen =0;
            pthread_cond_signal(&providers[i].cond);
            pthread_mutex_unlock(&providers[i].mutex);
        }
        pthread_join(providers[i].thread,NULL);
    }

    print(stdout,logFile,"Statistics\n");
    print(stdout,logFile,"Name   Number of clients served\n");
    for(i=0;i<numOfProvider;++i){
        print(stdout,logFile,"%-10s  %d\n",providers[i].name,providers[i].served);
    }

    free(providers);
    free(serverThread);
    free(timeProvider);
    free(priceProvider);
    free(performanceProvider);
    fclose(logFile);
    
    return (EXIT_SUCCESS);
}

void *providerF(void * provide){
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    pthread_sigmask(SIG_BLOCK,&set,NULL);   

    struct Provider *provider =(struct Provider*)provide;
    struct Question *question; 
    struct timespec start,end;
    pthread_mutex_lock(&provider->mutex);
    provider->isOpen =1;
    pthread_mutex_unlock(&provider->mutex);
    while(1){
        pthread_mutex_lock(&provider->mutex);
        while(isEmpty(&provider->client)){
            if(!provider->isOpen){
                print(stdout,logFile,"Provider %s's time is over and leaving the server\n",provider->name);
                pthread_mutex_unlock(&provider->mutex);
                pthread_cond_destroy(&provider->cond);
                pthread_mutex_destroy(&provider->mutex);
                pthread_exit(NULL);
            }
            else{
                print(stdout,logFile,"Provider %s is waiting for a task\n",provider->name);
                pthread_cond_wait(&provider->cond,&provider->mutex);
            }
        }
        start.tv_nsec =0; start.tv_sec =0;
        clock_gettime(CLOCK_MONOTONIC,&start);
        question =dequeue(&provider->client);
        provider->served +=1;
        pthread_mutex_unlock(&provider->mutex);
        pthread_mutex_lock(&question->mutex);
        print(stdout,logFile,"Provider %s is processing task number %d: %d\n",provider->name,provider->served,question->client.request);
        question->answer.result =cosF(question->client.request);
        question->answer.found =1;
        question->answer.cost =provider->price;
        strncpy(question->answer.providerName,provider->name,BUFSIZE);
        sleep(rand()%15+5);
        clock_gettime(CLOCK_MONOTONIC,&end);
        question->answer.time =((double)end.tv_sec+1.0e-9*end.tv_nsec)-((double)start.tv_sec+1.0e-9*start.tv_nsec);
        print(stdout,logFile,"Provider %s completed task number %d: cos(%d) =%.3lf in %lf seconds\n",provider->name,provider->served,
                question->client.request,question->answer.result,question->answer.time);
        pthread_cond_signal(&question->cond);
        pthread_mutex_unlock(&question->mutex);
        
    }
    return NULL;    
}

void *serverF(void *serve){
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    pthread_sigmask(SIG_BLOCK,&set,NULL);
    
    int *dataSocket =(int *)serve;
    struct Question question;
    int i,found; struct Provider **provider;
    
    if(read(*dataSocket,&question.client,sizeof(struct Client)) == -1){
        print(stderr,logFile,"Server read error!\n");
        close(*dataSocket);
        free(dataSocket);
        pthread_exit(NULL);
    }

    memset(&question.answer,0,sizeof(struct Answer));
    if(question.client.priority == 'C' || question.client.priority == 'c'){
        provider =priceProvider;
    }
    else if(question.client.priority == 'Q' || question.client.priority == 'q'){
        provider =performanceProvider;
    }
     
    else if(question.client.priority == 'T' || question.client.priority == 't'){
        provider =timeProvider;
    }
    found =0;
    for(i=0;i<numOfProvider && !found;++i){
        pthread_mutex_lock(&provider[i]->mutex);
        if(provider[i]->isOpen && isEmpty(&provider[i]->client)){
            print(stdout,logFile,"Client %s (%c %d) connected, forwarded to provider %s\n",question.client.name,question.client.priority,question.client.request,provider[i]->name);
            question.answer.found =0;
            pthread_cond_init(&question.cond,NULL);
            pthread_mutex_init(&question.mutex,NULL);
            enqueue(&provider[i]->client,&question);
            pthread_cond_signal(&provider[i]->cond);
            pthread_mutex_unlock(&provider[i]->mutex);
            found =1;
        }
        else{
            pthread_mutex_unlock(&provider[i]->mutex);
        }
    }
    if(found){
        pthread_mutex_lock(&question.mutex);
        while(!question.answer.found){
            pthread_cond_wait(&question.cond,&question.mutex);
        }
        pthread_mutex_unlock(&question.mutex);
        pthread_cond_destroy(&question.cond);
        pthread_mutex_destroy(&question.mutex);
    }
    else{
        question.answer.found =0;
        strncpy(question.answer.errMsg,"NO PROVIDER IS AVAILABLE",BUFSIZE);
        print(stdout,logFile,"No provider is available for client %s (%c %d)\n",question.client.name,question.client.priority,question.client.request);
    }
    
    if(write(*dataSocket,&question.answer,sizeof(struct Answer)) == -1){
        print(stderr,logFile,"Server write error! Server couldn't write socket for client %s\n",question.client.name);
    }

    close(*dataSocket);
    free(dataSocket);
      
    return NULL;
}

void *wakerF(void* wake){
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    pthread_sigmask(SIG_BLOCK,&set,NULL);  
    
    int count;
    time_t start,current,wait;
    sigemptyset(&set);
    sigaddset(&set,SIGUSR1);
    struct sigaction sa;
    memset(&sa,0,sizeof(struct sigaction));
    sa.sa_handler =sigHandler;
    sigaction(SIGUSR1,&sa,NULL);
    count =0;
    time(&start);
    while(count < numOfProvider && isServerOpen){
        time(&current);
        wait =timeProvider[count]->duration-(current-start);
        if(0<wait){
            sleep(wait);
        }
        if(isServerOpen){
            pthread_sigmask(SIG_BLOCK,&set,NULL);
            pthread_mutex_lock(&timeProvider[count]->mutex);
            timeProvider[count]->isOpen =0;
            pthread_cond_signal(&timeProvider[count]->cond);
            pthread_mutex_unlock(&timeProvider[count]->mutex);
            ++count;
            pthread_sigmask(SIG_UNBLOCK,&set,NULL);
        }
    }
    if(isServerOpen){
        print(stdout,logFile,"All providers time is over and signal is sent to main server\n");
        kill(getpid(),SIGINT);
    }
    
    return NULL;
}

double cosF(int degree){
    double radyan =(double)degree/180.0*M_PI;
    int i; double sum =0.0; double sign =1.0;
    for(i=0;i<100;++i){
        sum +=sign/factF(2.0*i)*pow(radyan,2.0*i);        
        sign *=-1.0;
    }
    return sum;
}

double factF(double n){
    if(n < 2){
        return 1.0;
    }
    else{
        return n*factF(n-1.0);
    }
}

void readProvider(struct Provider **providers,int *size,char *file){
    char *line;
    *size =0;
    (*providers) =NULL;
    line =strtok(file,"\n");
    line =strtok(NULL,"\n");
    while(line !=NULL){
        (*providers) =realloc((*providers),(*size+1)*sizeof(struct Provider));
        sscanf(line,"%s %d %d %d)",(*providers)[*size].name,
                &(*providers)[*size].performance,&(*providers)[*size].price,
                        &(*providers)[*size].duration);

        (*providers)[*size].isOpen =0;
        (*providers)[*size].served =0;
        (*providers)[*size].client.capacity =2;
        (*providers)[*size].client.front =0;
        (*providers)[*size].client.rear =(*providers)[*size].client.capacity-1;
        (*providers)[*size].client.size =0;
        pthread_cond_init((&(*providers)[*size].cond),NULL);
        pthread_mutex_init((&(*providers)[*size].mutex),NULL);
        *size +=1;
        line=strtok(NULL,"\n");
    }
}

void print(FILE *file1,FILE *file2, char const *list, ...) { 
    va_list arg;
    if(file1 !=NULL){
        va_start(arg,list);
        vfprintf(file1,list, arg);
        va_end(arg);
    }
    if(file2 !=NULL){
        va_start(arg,list);
        vfprintf(file2,list,arg);
        va_end(arg);
    }
}

void sigHandler(int sigNo){
    isServerOpen =0;
}

void sortPrice(struct Provider **provider){
    int i,j; struct Provider *temp;
    for(i=0;i<numOfProvider-1;++i){
        for(j =i+1;j<numOfProvider;++j){
            if(provider[j]->price < provider[i]->price){
                temp =provider[i];            
                provider[i] =provider[j];
                provider[j] =temp;
            }
        }
    }
}

void sortPerformance(struct Provider **provider){
    int i,j; struct Provider *temp;
    for(i=0;i<numOfProvider-1;++i){
        for(j =i+1;j<numOfProvider;++j){
            if(provider[j]->performance > provider[i]->performance){
                temp =provider[i];            
                provider[i] =provider[j];
                provider[j] =temp;
            }
        }
    }
}
void sortTime(struct Provider **provider){     
    int i,j; struct Provider *temp;
    for(i=0;i<numOfProvider-1;++i){
        for(j =i+1;j<numOfProvider;++j){
            if(provider[j]->duration < provider[i]->duration){
                temp =provider[i];            
                provider[i] =provider[j];
                provider[j] =temp;
            }
        }
    }
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
void enqueue(struct Queue* queue, struct Question *item){
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1)%queue->capacity;
    queue->array[queue->rear] =item;
    queue->size = queue->size + 1;
}
 
// Function to remove an item from queue. 
// It changes front and size
struct Question* dequeue(struct Queue* queue){
    if (isEmpty(queue))
        return NULL;
    struct Question *item = queue->array[queue->front];
    queue->front = (queue->front + 1)%queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

