#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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

int main(int argc, char** argv) {
     
    if(argc != 6){
        fprintf(stderr,"Wrong line argument enter your name, priority, question, server address and server port respectively\n");
        fprintf(stderr,"Example : ferhat C 45 127.0.0.1 5555\n");
        exit(EXIT_FAILURE);
    }
    
    char *name =argv[1];
    char priority =argv[2][0];
    int request =atoi(argv[3]);
    char *address =argv[4];
    int port =atoi(argv[5]);
    
    if(priority != 'c' && priority != 'C' && priority !='q' &&
            priority !='Q' && priority !='t' && priority !='T'){
    
        fprintf(stderr,"Priority %c is invalid! \n",priority);
        fprintf(stderr,"Priority can only be Quality (Q), Time (T) or Cost (C)\n");
        exit (EXIT_FAILURE);
    }
    setbuf(stdout,NULL);
    int sockfd =socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        fprintf(stderr,"Socket couldn't be opened for client %s\n",name);
        exit (EXIT_FAILURE);           
    }
    struct sockaddr_in addr;
    addr.sin_family =AF_INET;
    addr.sin_port =htons(port);
    addr.sin_addr.s_addr =inet_addr(address);
    if(connect(sockfd,(struct sockaddr*)&addr,sizeof(struct sockaddr_in)) == -1){
        fprintf(stderr,"Connection error! Client %s couldn't connect to server\n",name);
        exit (EXIT_FAILURE);
    }
    double totalTime;
    struct timespec start,end;
    struct Client client;
    struct Answer answer;
    strncpy(client.name,name,BUFSIZE);
    client.priority =priority;
    client.request =request;
    printf("Client %s is requesting %c %d from server %s:%d\n",name,priority,request,address,port);
    clock_gettime(CLOCK_MONOTONIC,&start);
    if(write(sockfd,&client,sizeof(struct Client)) == -1){
        fprintf(stderr,"Write Error! Client %s couldn't write to socket\n",name);
        exit (EXIT_FAILURE);
    }
    if(read(sockfd,&answer,sizeof(struct Answer)) ==-1){
        fprintf(stderr,"Read Error! Client %s couldn't read socket\n",name);
        exit(EXIT_FAILURE);
    }
    clock_gettime(CLOCK_MONOTONIC,&end);
    totalTime =((double)end.tv_sec+1.0e-9*end.tv_nsec)-((double)start.tv_sec+1.0e-9*start.tv_nsec);
    if(answer.found){
        printf("%s’s task completed by %s in %lf seconds, cos(%d)=%.3lf, cost is %d TL, total time spent %lf \n",name,answer.providerName,answer.time,
                request,answer.result,answer.cost,totalTime);
    }
    else{
        printf("Client %s's request couldn't be satisfied : %s\n",name,answer.errMsg);
    }
    
    return (EXIT_SUCCESS);
}


