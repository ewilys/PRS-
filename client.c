#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define RCVSIZE 1024
#define H_SIZE 24
#define NUMSEQ_SIZE 13 //warning num_seq is 13 and not 14 because 1 bytes is left for the flag fragm
#define FRAGM_FLAG_SIZE 1
#define DATA_SIZE 10

struct sockaddr_in addr_serveur, addr_data;
int port, port_data, connected, okay_file;
socklen_t alen;
char msgSnd[RCVSIZE], msgRcv[RCVSIZE];
int desc, compteur,size_data_recv;
char num_seq[NUMSEQ_SIZE],nb_data_rcv[DATA_SIZE] ;
FILE* f_out;

void connexion(int desc){
	printf("Initialisation \n");

	sendto(desc,"SYN",strlen("SYN"),0, (struct sockaddr *)&addr_serveur, alen);
	printf(" SYN sent\n");

	int msgSize= recvfrom(desc,msgRcv,RCVSIZE,0,(struct sockaddr*)&addr_serveur, &alen);

	if(msgSize>0){
		printf("Message SYN-ACK received\n");
		char* str=strtok(msgRcv, " ");
		if(strcmp(str,"SYN-ACK")==0){
			
			str=strtok(NULL, " ");	
			port_data=atoi(str);

			sendto(desc,"ACK",strlen("ACK"),0, (struct sockaddr *)&addr_serveur, alen );
			
			printf("Connexion established\n");
			memset(msgRcv,0,RCVSIZE);
			connected=1;
		}
		else{
			printf("Connexion failed at port_data recv\n");
			exit(-1);
		}
		
	}
	else{
		printf("Connexion failed at SYN-ACK rcv\n");
	}
}
	
int init(char* port, char* ip_addr){

	int valid= 1;
	

	memset((char*)&addr_serveur,0,sizeof(addr_serveur));
	memset((char*)&addr_data,0,sizeof(addr_data));
	alen = sizeof(addr_serveur);
	memset(msgRcv,0,RCVSIZE);
	memset(msgSnd,0,RCVSIZE);
	int desc= socket(AF_INET, SOCK_DGRAM, 0);
	if (desc < 0) {
			perror("cannot create socket\n");
			return -1;
	}

	setsockopt(desc, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

	addr_serveur.sin_family= AF_INET;
	addr_serveur.sin_port= htons(atoi(port));
	inet_aton(ip_addr , &addr_serveur.sin_addr);
	
	connexion(desc);
	
	addr_data.sin_family= AF_INET;
	addr_data.sin_port= htons(port_data);
	inet_aton(ip_addr , &addr_data.sin_addr);
	
	return desc;	
}

void file_reception(){

	printf("\t reception file begins\n");
	compteur=1;
	okay_file=1; //check if the file is received 1 not completely received 0 full received
	while(okay_file==1){

		memset(msgRcv,0,RCVSIZE);
		size_data_recv=recvfrom(desc, msgRcv, RCVSIZE,0,(struct sockaddr *)&addr_data, &alen);
		if(size_data_recv <0 ){
			printf("Error : nothing received\n");
			exit(0);
		}			

	//catch the sequence numero
		memcpy(num_seq, msgRcv, NUMSEQ_SIZE);
		printf("Segment num : %s received \n", num_seq);
	//catch the flag fragm
		printf("flag fragm :%c\n",msgRcv[NUMSEQ_SIZE]);
		if(msgRcv[NUMSEQ_SIZE]=='0'){
			okay_file=0; //end of file
			printf("\t last seq num \n");
		}
		
	//catch size of payload	
		memcpy(nb_data_rcv, msgRcv+NUMSEQ_SIZE+FRAGM_FLAG_SIZE, DATA_SIZE);
		

		if(atoi(num_seq)==compteur){ // if it's the good sequence received
			size_data_recv=fwrite(msgRcv+H_SIZE,1,atoi(nb_data_rcv),f_out);
			if(size_data_recv==0){
				printf("Error, impossible to write in the output file\n");
				exit(-1);
			}
			printf("size payload received : %s ,size data written in the output : %d \n \n",nb_data_rcv, size_data_recv);
			sprintf(msgSnd,"ACK_%d", compteur);
			compteur++;
		}
		else{
			printf("Warning : Segment missing. Need segment num %d, actually recieved %s\n", compteur, num_seq);
		}
	
		sendto(desc,msgSnd,strlen(msgSnd),0, (struct sockaddr *)&addr_data, alen );
		printf("...Message  : %s sent...\n", msgSnd);
	}
}
	

void conversation(){

	printf("\t conversation begins, you can write stop at any time to close the connexion\n");
	
	while (connected==1) {
		
			memset(msgSnd,0,RCVSIZE);
			fgets(msgSnd, RCVSIZE, stdin);
			sendto(desc,msgSnd,strlen(msgSnd),0, (struct sockaddr *)&addr_data, alen );
			
			
			if (strcmp(msgSnd,"stop\n") == 0) {
				connected= 0;
			}
			else{
				recvfrom(desc, msgRcv, RCVSIZE,0,(struct sockaddr *)&addr_data, &alen);
				printf("Answer : %s\n",msgRcv);
				memset(msgRcv,0,RCVSIZE);
			}
		}

}	 
int main (int argc, char *argv[]) {

	if(argc<4 || argc>4){
		perror("Usage : ./client <no_port> <addr_serveur> <inputfile>\n");
		exit(-1);
	}
	else{
		
		desc=init(argv[1], argv[2]);
		
		printf("information exchange on port: %d \n", port_data);
		
		char name_output_file[100]="";
		sprintf(name_output_file,"sortie_%s",argv[3]);
		printf("name output file : %s \n",name_output_file);
		
		char* str;
		do{
			//send the name of file to receive
			sendto(desc,argv[3],strlen(argv[3]),0, (struct sockaddr *)&addr_data, alen );
	
			recvfrom(desc, msgRcv, RCVSIZE,0,(struct sockaddr *)&addr_data, &alen);
			printf("received : %s\n", msgRcv);
			// check if it's the good ack
				str=strtok(msgRcv, "_");
				if(strcmp(str,"ACK")==0){
					str=strtok(NULL, " ");	
				}
		}while(atoi(str)!=0);

		memset(msgRcv,0,RCVSIZE);

		f_out=fopen(name_output_file, "a");
		if(f_out==NULL)
			printf("Error file\n");

		else{
			file_reception();
			conversation();
		}
		
		close(desc);
		fclose(f_out);
	}
	return 0;
}
