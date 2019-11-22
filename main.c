#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <memory.h>
#include <cygwin/wait.h>
#include <errno.h>

#define PORT "2244"
#define BACKLOG 10
#define MAXDATASIZE 256
#define BUFFERSIZE 1024

//Devices Connected
typedef struct devices_connected{
    char *device;
    void *dhead;
    void *dstart;
    void *thead;
    void *tstart;
    void *durhead;
    void *durstart;
    int count;
}devices_connected;

void add_new_device(devices_connected *d,char *device, char *t){//need a char type of time
    d->dhead = malloc(strlen(device));//Make space for device name
    d->thead = malloc(strlen(t));
    memcpy(d->dhead,device,strlen(device));//Saved name
    memcpy(d->thead,t,strlen(t));//Saved time
    if(d->count == 0){
        d->dstart = malloc(strlen(device));
        d->dstart = d->dhead;//Have pointer to start of array
        d->tstart = malloc(strlen(t));
        d->tstart = d->thead;
    }
    d->thead = d->thead + strlen(t);
    d->dhead = d->dhead + strlen(device);
    d->count++;
}
void device_session(devices_connected *d,char *device, char *t_end){
    char *c_start;
    memcpy(c_start,d->thead,strlen(d->thead));
    char *c_dur;
    c_dur = malloc(strlen(t_end)-strlen(c_start));
    int i_start = &c_start - '0';//Make string to int
    int i_end = &t_end - '0';
    int dur = i_end - i_start;//find duration
    sprintf(c_dur,"%d",dur);//int to string
    d->durhead = malloc(strlen(c_dur));//Make space
    memcpy(d->durhead,c_dur,strlen(c_dur));
    if(d->count==0){
        d->durstart = malloc(strlen(t_end));
        d->durstart = d->durhead;
    }
    d->durhead = d->durhead + strlen(c_dur);
}

//Circular_Buffer
typedef struct circular_buffer//Create Class
{
    void *buffer;     // data buffer
    void *buffer_end; // end of data buffer
    size_t capacity;  // maximum number of items in the buffer
    size_t count;     // number of items in the buffer
    size_t sz;        // size of each item in the buffer
    void *head;       // pointer to head
    void *tail;       // pointer to tail

} circular_buffer;
//Initialize
void cb_init(circular_buffer *cb, size_t capacity, size_t sz)//cb_init(name,how many, size of messages)
{
    cb->buffer = malloc(capacity * sz);//Make space
    if(cb->buffer == NULL){
        // handle error
    }
    cb->buffer_end = (char *)cb->buffer + capacity * sz;
    cb->capacity = capacity;
    cb->count = 0;
    cb->sz = sz;
    cb->head = cb->buffer;
    cb->tail = cb->buffer;
}
//Delete Circular_Buffer
void cb_free(circular_buffer *cb)
{
    free(cb->buffer);
    // clear out other fields too, just to be safe
}
//Delete the first one
void cb_pop_front(circular_buffer *cb, void *item)
{
    if(cb->count == 0){
        printf("Circular Buffer empty");
        return;
    }
    memcpy(item, cb->tail, cb->sz);
    cb->tail = (char*)cb->tail + cb->sz;
    if(cb->tail == cb->buffer_end)
        cb->tail = cb->buffer;
    cb->count--;
}
//Add to the buffer
int cb_check_duplicates(circular_buffer *cb, const char *item) {
    char *message;
    void *start = cb->head;
    while (cb->head != cb->buffer_end) {
        message = malloc(cb->sz);
        memcpy(message, cb->head, cb->sz);
        if (message == item) {
            return 1;
        }
        cb->head = (char *) cb->head + cb->sz;
    }
    cb->head = cb->buffer;
    while (cb->head != start) {
        message = malloc(cb->sz);
        memcpy(message, cb->head, cb->sz);
        if (message == item) {
            return 1;
        }
        cb->head = (char *) cb->head + cb->sz;
    }
    return 0;
}
void cb_add_to_buffer(circular_buffer *cb, const char *item)
{
    if (cb_check_duplicates(cb,&item)==1){
        return;
    }
    memcpy(cb->head, item, cb->sz);
    cb->head = (char*)cb->head + cb->sz;
    if(cb->head == cb->buffer_end)
        cb->head = cb->buffer;
    cb->count++;
}
void cb_print(circular_buffer *cb){
    char *message;
    void *start = cb->head;
    while (cb->head!=cb->buffer_end){
        message = malloc(cb->sz);
        memcpy(message,cb->head,cb->sz);
        printf("%d\n",*message);
        cb->head= (char *) cb->head+cb->sz;
    }
    cb->head = cb->buffer;
    while (cb->head!=start){
        message = malloc(cb->sz);
        memcpy(message,cb->head,cb->sz);
        printf("%d\n",*message);
        cb->head= (char *) cb->head+cb->sz;
    }
}

void cb_send(circular_buffer *cb, int new_fd){
    char *message;
    void *start = cb->head;
    while (cb->head!=cb->buffer_end){
        message = malloc(cb->sz);
        memcpy(message,cb->head,cb->sz);
        send(new_fd,message,strlen(message),0);
        cb->head= (char *) cb->head+cb->sz;
    }
    cb->head = cb->buffer;
    while (cb->head!=start){
        message = malloc(cb->sz);
        memcpy(message,cb->head,cb->sz);
        send(new_fd,message,strlen(message),0);
        cb->head= (char *) cb->head+cb->sz;
    }
}

//returns message AEM_1_AEM_2_t_message
char *create_message(uint32_t AEM_1,uint32_t AEM_2,time_t t,char *message){//Create string to send
    char *C_AEM_1;
    C_AEM_1 = malloc(5);
    char *C_AEM_2;
    C_AEM_2 = malloc(5);
    char *C_t;
    C_t = malloc(11);
    //Convert int to char
    sprintf(C_AEM_1, "%d", AEM_1);
    sprintf(C_AEM_2, "%d", AEM_2);
    sprintf(C_t,"%d",t);

    //Combine into packet to send
    char *packet;
    packet=malloc(strlen(C_AEM_1)+strlen(C_AEM_2)+strlen(C_t)+strlen(message));
    strcpy(packet,C_AEM_1);
    strcat(packet,"_");
    strcat(packet,C_AEM_2);
    strcat(packet,"_");
    strcat(packet,C_t);
    strcat(packet,"_");
    strcat(packet,message);

    printf("Packet: %s\n",packet);
    return packet;
}

//Returns 10.0.xx.yy xx: first two digits of AEM_2  yy: last two digits
char *create_IP(uint32_t AEM_2){
    char *C_AEM_2;
    C_AEM_2 = malloc(5);
    sprintf(C_AEM_2, "%d", AEM_2);
    int ip_length = strlen("10.0.")+strlen(".")+strlen(C_AEM_2)+1;
    char *send_IP;
    send_IP = malloc("10.0.99.99");
    char *temp;
    strcpy(send_IP,"10.0.");
    temp = C_AEM_2[0];
    strcat(send_IP,&temp);
    temp = C_AEM_2[1];
    strcat(send_IP,&temp);
    strcat(send_IP,".");
    temp = C_AEM_2[2];
    strcat(send_IP,&temp);
    temp = C_AEM_2[3];
    strcat(send_IP,&temp);

    printf("Send_IP: %s\n",send_IP);
    return send_IP;
}
//gets sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa){
    if (sa-> sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)-> sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
//In a nutshell it connects to a port and waits to receive a packet from a server
int client(char *my_IP,circular_buffer buffer,char *C_AEM_1,devices_connected *d ){
    int sockfd;
    int numbytes;
    char received_message[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET_ADDRSTRLEN];

    memset(&hints,0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if((rv=getaddrinfo(my_IP,PORT,&hints,&servinfo)!=0)){
        fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));
    }

    //loop through all the results and connect to the first we can
    for (p=servinfo;p!=NULL;p=p->ai_next){
        //Socket error
        if ((sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1){
            perror("client: socket");
            continue;
        }
        //Connection error
        if(connect(sockfd,p->ai_addr,p->ai_addrlen)==-1){
            perror("client: connect");
            close(sockfd);
            continue;
        }
        break;
    }
    if (p==NULL){
        fprintf(stderr, "cliend: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
    printf("client: connectiing to %s\n",s);
    freeaddrinfo(servinfo);//Done
    char *device_name;
    while (recv(sockfd,received_message,MAXDATASIZE-1,0)!=0){
        if((numbytes = recv(sockfd,received_message,MAXDATASIZE-1,0)==-1)){
            perror("recv");
            exit(1);
        }
        if (numbytes!=strlen(C_AEM_1)){
            received_message[numbytes]='\0';
            cb_add_to_buffer(&buffer, received_message);
            printf("client: received %s\n",received_message);
        }else{
            device_name = received_message;
            time_t t = time(NULL);//Take time of connection
            add_new_device(d,device_name,(char*)t);//Add device
        }
    }
    time_t t_end = time(NULL);//Take time of connection
    device_session(d,device_name,(char*)t_end);
    close(sockfd);
    return 0;
}

void sigchld_handler(int s){
    (void)s; //quiet unused variable warning
    //waitpid() might overwrite errno, so we save and restore it
    int saved_errno = errno;

    while(waitpid(-1,NULL,WNOHANG)>0);
    errno = saved_errno;
}

int server(circular_buffer buffer, char *C_AEM_1){
    int sockfd, new_fd;//listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo,*p;
    struct sockaddr_storage their_addr;//Connectors adress information
    socklen_t sin_size;
    struct sigaction sa;
    int yes =1;
    char s[INET_ADDRSTRLEN];
    int rv;
    int send_status;

    //Set up hints
    memset(&hints,0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; //Use my IP

    if ((rv =getaddrinfo(NULL,PORT,&hints,&servinfo))!=0){
        fprintf(stderr,"getaddrinfo %s\n",gai_strerror(rv));
        return 1;
    }

    //loop through all the results and bind to the first we can
    for(p=servinfo;p!=NULL;p->ai_next){
        if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol)==-1)){
            perror("serrverL socket");
            continue;
        }
        if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes, sizeof(int))==-1){
            perror("setsockopt");
//            exit(1);
        }
        if(bind(sockfd,p->ai_addr,p->ai_addrlen)==-1){
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);//Done

    if(p==NULL){
        fprintf(stderr,"server: failed to bind \n");
        exit(1);
    }
    if (listen(sockfd,BACKLOG)==-1){
        perror("listen");
        exit(1);
    }
    sa.sa_handler = sigchld_handler; //reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags= SA_RESTART;
    if(sigaction(SIGCHLD,&sa,NULL)==-1){
        perror("sigaction");
        exit(1);
    }
    printf("server: waiting for connections... \n");

    while(1){//Main accept() loop
        sin_size = sizeof(their_addr);
        printf("their_addr: %s\n",their_addr);
        if (&their_addr == "10.0.99.99")
            break;
        new_fd = accept(sockfd,(struct sockaddr *)&their_addr,&sin_size);
        if(new_fd==-1){
            perror("Accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof(s));
        printf("server: got connection from: %s\n",s);

        if(!fork()){//child process
            close(sockfd);//Dont need the listener
            send_status = send(new_fd,&C_AEM_1,13,0);
            if (send_status==-1)
                perror("send");
            //Send all the messages from the circular buffer
            cb_send(&buffer, &new_fd);
            close(new_fd);
            exit(0);
        }
        close(new_fd);//Parent dont need this
    }
}

int main() {
    //uint32: 32 bit integer
// AEM_1
    uint32_t AEM_1 = 8967;
    char *C_AEM_1;
    C_AEM_1 = malloc(5);
    sprintf(C_AEM_1, "%d", AEM_1);
    printf("AEM_1: %d\n",AEM_1);
//AEM_2
    uint32_t AEM_2[] ={8960, 8961,8962,8963,8964,8965,8966,8967,8968,8969};
    int AEM_2_Num=2;
    printf("AEM_2[%d]:%d\n",AEM_2_Num,AEM_2[AEM_2_Num]);
//Message
    char message[256] = "Hello there";
    printf("Message: %s\n",message);
//Time
    int *t;
    t = time(NULL);
    printf("Time: %d\n",t);

//Time to function as a server
    srand(time(NULL));   // Initialization, should only be called once.
    int *server_time;
    server_time = rand()%(300-60+1)+60;

//Devices connected
    devices_connected *devices;

    char *packet;
    packet = create_message(AEM_1,AEM_2[AEM_2_Num],t,message);
    printf("Packet returned from create_message: %s\n",packet);

    char *send_IP;
    send_IP = create_IP(AEM_2[AEM_2_Num]);
    printf("sendIP from create_IP %s\n", send_IP);

    char *my_IP;
    my_IP = create_IP(AEM_1);
    printf("my_IP from create_IP %s\n", my_IP);

    //Create Buffer
    circular_buffer buffer;
    //Initialize Buffer
    cb_init(&buffer,BUFFERSIZE,MAXDATASIZE);

    while (sleep(server_time)){
        server(buffer, C_AEM_1);
    }
    while (sleep(server_time)){
        client(my_IP,buffer,C_AEM_1,devices);
    }
    cb_free(&buffer);



//int getaddrinfo(const char *node(IP), const char *service(port number),const struct *hints, struct addrinfo **res )
//Give IP, Port Number, hints(list with relevant info) and returns pointer array res
    //setup for server to lisen
//    int status;
//    struct addrinfo hints;
//    struct addrinfo *servinfo;//Will point the results
//
//    memset(&hints, 0, sizeof(hints));//Set everything in hints to 0
//    hints.ai_family = AF_INET; //AF_UNSPEC - dont care, AF_INET - IPv4, AF_INET6 - IPv6
//    hints.ai_socktype = SOCK_STREAM;//TCP stream sockets
//    hints.ai_flags = AI_PASSIVE;//Fill in my IP
//    status = getaddrinfo(NULL,"2244",&hints, &servinfo);
    //Now servinfo points to a linked list to 1 or more addrinfos
    //For client side: Its the same but at getaddrinfo you put destination IP or website
    //getaddrinfo(send_IP,"2244",&hints,&servinfo); And DELETE hints.ai_flags = AI_PASSIVE
//int socket(int domain(IPv4 or IPv6), int type (stream or datagram), int protocol (TCP or UDP))
//    int sockfd;
//    sockfd = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);//Returns socket discriptor or -1 for error
//    servinfo->ai_family: Take the relevant info that was given from getaddrinfo

//int bind(int sockfd, struct sockaddr *my_addr, int addrlen)
//sockfd: socket discriptor returned by socket()
// my_addr pointer to a struct sockaddr wigth info about  the address (IP, port)
//addrlen: length in bytes of that address
//    bind(sockfd,servinfo->ai_addr,servinfo->ai_addrlen);//Retruns -1 on error
    //If i dont want to bind to a specific port skip this command
//int connect(int sockfd, struct sockaddr *serv_addr,int addrlen);
//sockdf: socket discriptor returned by socket()
//serv_addr: contains destination port and IP
//addrlen: length in bytes
//    connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);//Returns -1 if error

//int listen(int sockfd, int backlog);
//sockdf: socket discriptor returned by socket()
//backlog: number of connections allowed on the incoming queue max 20
/*For listening to incoming connections you do
 * getaddrinfo()
 * socket()
 * bind()
 * listen()
 * accept() */
//    listen(sockfd,5);//Smth like this

//int accept(int sockfd, struct sockaddr *addr,socklen_t *addrlen)
//sockdf: socket discriptor returned by socket()
//addr pointer to struct sockaddr_storage. This is where the incoming information will go
//addlen address length
//    int new_fd;//accept() receives a new socket discriptor from the sender new_fd. sockfd is still able to receive new requests
//    struct sockaddr_storage their_addr;
//    socklen_t addr_size;
//    addr_size = sizeof(their_addr);
//    new_fd = accept(sockfd,(struct sockaddr*)&their_addr,&addr_size);
//    ready to communicate
//
//int send(int sockfd, const void *msg, int len, int flags)
// *msg: Pointer to the data i want to send
//len: length of message in bytes
//flags: Set to 0
//    int bytes_send,message_len;
//    message_len = strlen(message);
//    bytes_send = send(sockfd, message,message_len,0);//returns number of bytes to send out
//
//int recv(int sockfd, void *buf, int len, int flags); Returns the number of bytes read into buffer. If 0 the connevtion closed.
//sockfd socket discritpor to read from
//buf: buffer to read the indo into
//len: maximum length of buffer
//flags: set to 0
//    recv(sockfd, buffer,buffer_size,0);


}