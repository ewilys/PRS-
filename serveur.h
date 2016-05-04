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

#define MSS 1036 //max segment size
#define MDS 1024 // max data size
#define H_SIZE 12 //header size
#define NUMSEQ_SIZE 6 //nb bytes for num segment in header
#define FRAGM_FLAG_SIZE 1 //nb bytes for fragm flag in header
#define DATA_SIZE 5 // nb bytes for size in header

#define DUPLICATE 2

#define MAX_RTT 50
#define X_RTT 3
#define MICROS 1000000

#define TRUE 1
#define FALSE 0

struct sockaddr_in serveur, client, data;
int port, port_data, connected,valid,count, nbClient;
int fragm; //fragm : 1 if the seq is not the last seq ; 0 if it's the last seq
int desc,desc_data_sock;
int msgSize;
int file_size;
int nb_segment_total;
char recep[MSS], sndBuf[MSS];
struct timeval save_start[MAX_RTT];
struct timeval save_timeout[MAX_RTT];
socklen_t alen;
FILE* fin;

int cwnd;
int flight_size;
int okFile;



//Variables linked to RTT estimation
long RTT = 10000, mesure; //RTT =1s=1 000 000 ms
double alpha = 0.6;
struct timeval start, end;
struct timeval timeout;

pthread_mutex_t mutex;
pthread_t listener,sender;//2 thread one to send the file the other to receive ACKs
pthread_attr_t attr_listener,attr_sender;



void connexion();
void new_connexion();
struct timeval estimateTimeout(long);
void init();
void conversation();
void *send_file(void *arg);
void *receive_ACK(void *);
int catch_file_size();
long estimateRTT(double, long, long);
