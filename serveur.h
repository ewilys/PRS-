#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MSS 1015 //max segment size
#define MDS 1000 // max data size
#define H_SIZE 15 //header size
#define NUMSEQ_SIZE 9 //nb bytes for num segment in header
#define FRAGM_FLAG_SIZE 1 //nb bytes for fragm flag in header
#define DATA_SIZE 5 // nb bytes for size in header

struct sockaddr_in serveur, client, data;
int port, port_data, connected,valid,count;
int fragm; //fragm : 1 if the seq is not the last seq ; 0 if it's the last seq
int desc,desc_data_sock;
int msgSize;
int file_size;
char recep[MSS], sndBuf[MSS];
socklen_t alen;
FILE* fin;

void connexion();
void init();
void conversation();
void send_file();
int catch_file_size();
