#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>

#define MSS 1500 //max segment size
#define MDS 1494 // max data size

#define NUMSEQ_SIZE 6 //nb bytes for num segment in header

#define MAX_SIZE_BUFCIRCULAIRE 1000980
#define DUPLICATE 3
#define RWND 30

#define TRUE 1
#define FALSE 0

struct sockaddr_in serveur, client, data;
int port, port_data, connected,valid,count, nbClient;

int desc,desc_data_sock;
int msgSize;
int file_size, size_to_read, readFile;
int nb_segment_total;
char recep[MSS], sndBuf[MSS];
socklen_t alen;
FILE* fin;

struct timeval timeout, start, end, t_elapsed, RTT; 
long cste = 0.8;    
    
double cwnd;
double flight_size;
int okFile;
int debug;
double ssthresh;

pthread_mutex_t mutex;
pthread_t listener,sender;//2 thread one to send the file the other to receive ACKs
pthread_attr_t attr_listener,attr_sender;

void connexion();
int checkACK();
char *str_sub ( int start,  int end);
void init();
void conversation();
double min(double, double);
void *send_file(void *arg);
void *receive_ACK(void *);
int catch_file_size();
struct timeval estimateRTT(struct timeval, long, struct timeval);
