#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define RCVSIZE 1024
#define DATAFILE 1000
#define H_SIZE 24
#define NUMSEQ_SIZE 13 //warning num_seq is 13 and not 14 because 1 bytes is left for the flag fragm
#define FRAGM_FLAG_SIZE 1
#define DATA_SIZE 10
struct sockaddr_in serveur, client, data;
int port, port_data, connected,valid,compt;
int fragm; //fragm : 1 if the seq is not the last seq ; 0 if it's the last seq
int desc,desc_data_sock;
int msgSize;
char recep[RCVSIZE], sndBuf[RCVSIZE], data_file[DATAFILE];
socklen_t alen;
FILE* fin;



void connexion(){ //mise en place connexion simulation demande de connexion tcp

	printf("Connexion process \n");

	alen= sizeof(client);

	msgSize= recvfrom(desc,recep,RCVSIZE,0,(struct sockaddr*)&client, &alen);
	printf("first message received %s\n",recep);

	if(msgSize>0){
		if(strcmp(recep,"SYN")==0){
			sprintf(sndBuf, "SYN-ACK %d", port_data);

			sendto(desc,sndBuf,sizeof(sndBuf),0, (struct sockaddr*)&client, alen);

			printf("Message sent: %s \n", sndBuf);
			memset(sndBuf,0,RCVSIZE);
			memset(recep,0,RCVSIZE);

			msgSize=recvfrom(desc,recep,RCVSIZE,0,(struct sockaddr*)&client, &alen);
			
			if(msgSize>0){
				if(strcmp(recep,"ACK")==0){
					printf("Connexion established\n");
					memset(recep,0,RCVSIZE);
					connected++;
				}
				else{
					printf("Connexion failed at last ACK point\n");
					exit(-1);
				}
			}
			else{
				printf("Connexion failed at SYN/SYN-ACK point\n");
				exit(-1);
			}		
		}
		else{
			printf("Connexion failed : reception error\n");
			exit(-1);
		}
	}
}
				
		
void init( ){ //initialisation des variables et création/lien socket
	
		printf("Initialisation \n");

		memset((char*)&serveur,0,sizeof(serveur));
		memset((char*)&client,0,sizeof(client));
		
		
		valid= 1;
		connected=0;
		memset(sndBuf,0,RCVSIZE);
		memset(recep,0,RCVSIZE);
		alen= sizeof(client);

	//create socket
		desc= socket(AF_INET, SOCK_DGRAM, 0);
		desc_data_sock=socket(AF_INET, SOCK_DGRAM, 0);

  	// handle error
		if (desc < 0 || desc_data_sock<0) {
			perror("cannot create socket\n");
			exit(-1);
		}
	
	//allow reuse of sockets
		setsockopt(desc, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
		setsockopt(desc_data_sock, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
		
		
		serveur.sin_family= AF_INET;
		data.sin_family= AF_INET;
		serveur.sin_port= htons(port);
		data.sin_port= htons(port_data);
		serveur.sin_addr.s_addr= htonl(INADDR_ANY);
		data.sin_addr.s_addr= htonl(INADDR_ANY);		

		if (bind(desc, (struct sockaddr*) &serveur, sizeof(serveur)) == -1) {
			perror("Bind fail\n");
			close(desc);
			exit(-1);
		}
		
		if (bind(desc_data_sock, (struct sockaddr*) &data, sizeof(data)) == -1) {
			perror("Bind fail\n");
			close(desc_data_sock);
			exit(-1);
		}

}	


void conversation(){

	connected=1;
	compt=1;
	printf("\t conversation begins\n");
		while (connected==1) {
				
			memset(recep,0,RCVSIZE);
			msgSize= recvfrom(desc_data_sock,recep,RCVSIZE,0,(struct sockaddr*)&client, &alen);

			if(msgSize > 0) {
				
				printf("received : %s\n", recep);
				if(strcmp(recep, "stop\n")==0){
					printf("Connexion ended by client\n");
					memset(recep,0,RCVSIZE);
					connected=0;
				}
				else{
					sprintf(sndBuf,"ACK_%d",compt);
					sendto(desc_data_sock,sndBuf,strlen(sndBuf),0, (struct sockaddr*)&client, alen);
					printf("%s sent \n",sndBuf);
					memset(sndBuf,0,RCVSIZE);
					memset(recep,0,RCVSIZE);
					compt++;
				}
				
			}
			else{
				printf("Reception error\n");
			}	

					
		}

}


void send_file(){

	compt=1;
	fragm=1;
	int size_data_read=1;
	printf("\t send_file begins\n");
	while(feof(fin)==0){
		
		memset(sndBuf,0,RCVSIZE);

	//read maximum DATAFILE=1024 bytes from the input file
		size_data_read=fread(data_file, 1, DATAFILE, fin);
		
		sprintf(sndBuf,"%d",compt); // write the number of sequence in the buffer
	//if endoffile then it's the last sequence
		if(feof(fin)!=0){
			fragm=0;
			printf("\tlast seq to send, reach end of file\n");
		}
		sprintf(sndBuf+NUMSEQ_SIZE,"%d",fragm);
		sprintf(sndBuf+NUMSEQ_SIZE+FRAGM_FLAG_SIZE,"%d",size_data_read);// write size of the payload in the buffer
		memcpy(sndBuf+H_SIZE,data_file,size_data_read); // copy the payload (data_file) inside the buffer to send.
		
		printf("size data read from the input file : %d and size data sent  : %d \n", size_data_read,size_data_read+H_SIZE);

	//send the sequence
		int total_size=size_data_read+H_SIZE;
		sendto(desc_data_sock,sndBuf,total_size,0, (struct sockaddr*)&client, alen);

		printf("num seq : %s\n",sndBuf);
		
		memset(recep,0,RCVSIZE);
		
	//wait for ack
		msgSize= recvfrom(desc_data_sock,recep,RCVSIZE,0,(struct sockaddr*)&client, &alen);
		while(msgSize > 0) {
				
				printf("received : %s\n", recep);
			// check if it's the good ack
				char* str=strtok(recep, "_");
				if(strcmp(str,"ACK")==0){
					str=strtok(NULL, " ");	
					if (atoi(str)==compt){
						compt++;
						msgSize=0; //if the good ack is received ,quit this while section
					}
					else{ //if not send back the same sequence only once
						sendto(desc_data_sock,sndBuf,total_size,0, (struct sockaddr*)&client, alen);
						printf("sequence %s sent back\n",sndBuf);
						memset(recep,0,RCVSIZE);
						//catch the ack of this seq sent back
						msgSize= recvfrom(desc_data_sock,recep,RCVSIZE,0,(struct sockaddr*)&client, &alen);
					}
				}
		}
	}

	
	
	
}

int main (int argc, char *argv[]) {

	if(argc!=2){
		perror("Usage : ./serveur <no_port> \n");
		exit(0);
	}
	else{
		port=atoi(argv[1]);
		port_data=port+1;

  		init();
		connexion(); 

		

		
		
		//échange d'informations possibles sur 2eme port 

		printf("information exchange on port : %d \n",port_data); 
		
		do {
			msgSize= recvfrom(desc_data_sock,recep,RCVSIZE,0,(struct sockaddr*)&client, &alen);

			if(msgSize > 0) {
				
				printf("received : %s\n", recep);
				fin=fopen(recep, "r");
				if(fin==NULL)
					printf("Error : file not found \n");
				else{
					sprintf(sndBuf,"ACK_0");
					sendto(desc_data_sock,sndBuf,strlen(sndBuf),0, (struct sockaddr*)&client, alen);
					printf("%s sent \n",sndBuf);
					memset(sndBuf,0,RCVSIZE);
					memset(recep,0,RCVSIZE);
				}
			}
		}while(fin==NULL);

		send_file();
		conversation();
		fclose(fin);
		close(desc);
		close(desc_data_sock);
	}
	return 0;
}
