#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

#define MSS 1500 //max segment size
#define MDS 1494 // max data size

#define NUMSEQ_SIZE 6 //nb bytes for num segment in header

#define MAX_SIZE_BUFCIRCULAIRE 10159200 //memory size for the file : 10,16Mo
#define DUPLICATE 2
#define RWND 100

#define TRUE 1
#define FALSE 0

struct sockaddr_in serveur, client, data;
int port, port_data, connected,valid,count, nbClient;

int desc,desc_data_sock;
int msgSize;
int file_size;
int nb_segment_total;
char recep[MSS], sndBuf[MSS];
socklen_t alen;
FILE* fin;

double cwnd;
int flight_size;
int okFile;
int debug;
int ssthresh;

int readFile;
int size_to_read;

pthread_mutex_t mutex;
pthread_t listener,sender;//2 thread one to send the file the other to receive ACKs
pthread_attr_t attr_listener,attr_sender;

void connexion();
int checkACK();
char *str_sub ( int start,  int end);
void init();
void conversation();
int min(int, int);
void *send_file(void *arg);
void *receive_ACK(void *);
int catch_file_size();
