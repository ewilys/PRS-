#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MSS 1036 //max segment size
#define MDS 1024
#define H_SIZE 12 //header size
#define NUMSEQ_SIZE 6 //nb bytes for num segment in header
#define FRAGM_FLAG_SIZE 1 //nb bytes for fragm flag in header
#define DATA_SIZE 5 // nb bytes for size in header
#define DROP 5 //the x*DROP packet is lost

struct sockaddr_in addr_serveur, addr_data;
int port, port_data, connected, okay_file;
socklen_t alen;
char msgSnd[MSS], msgRcv[MSS];
int desc, count,size_data_recv;
int file_size;
char num_seq[NUMSEQ_SIZE],nb_data_rcv[DATA_SIZE] ;
FILE* f_out;

int drop_count;//raise until the count of packet received reach the DROP

int checkFin();
void connexion(int desc);
int init(char* port, char* ip_addr);
void desencapsulation();
void file_reception();
void catch_file_ize();
void conversation();
