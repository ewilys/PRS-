#include "client.h"


int checkFin(){
	
	char end[3]="FIN";
	int i;
	for(i=0;i<3;i++){
		if(end[i]!=msgRcv[i]){
			return 0;
		}
	}
	return 1;
}


void connexion(int desc){
	printf("Initialisation \n");

	sendto(desc,"SYN",strlen("SYN"),0, (struct sockaddr *)&addr_serveur, alen);
	printf(" SYN sent\n");

	int msgSize= recvfrom(desc,msgRcv,MSS,0,(struct sockaddr*)&addr_serveur, &alen);

	if(msgSize>0){
		printf("Message SYN-ACK received\n");
		char* str=strtok(msgRcv, " ");
		if(strcmp(str,"SYN-ACK")==0){
			
			str=strtok(NULL, " ");	
			port_data=atoi(str);

			sendto(desc,"ACK",strlen("ACK"),0, (struct sockaddr *)&addr_serveur, alen );
			
			printf("Connexion established\n");
			memset(msgRcv,0,MSS);
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
	memset(msgRcv,0,MSS);
	memset(msgSnd,0,MSS);
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

void desencapsulation(){
	
	memset(msgRcv,0,MSS);
	memset(msgSnd,0,MSS);
		
	size_data_recv=recvfrom(desc, msgRcv, MSS,0,(struct sockaddr *)&addr_data, &alen);
		//printf("size data received : %d\n",size_data_recv);
	if(size_data_recv <0 ){
			printf("Error : nothing received\n");
			exit(0);
		}	
	
	
	if(size_data_recv==3){
		
		if(checkFin()==1){	
			okay_file=0;
		}
	}
	else{
		//catch the sequence numero
			
		memcpy(num_seq, msgRcv, NUMSEQ_SIZE);
			
		
		
		if(msgRcv[NUMSEQ_SIZE]=='0'){
			//okay_file=0; //end of file
			printf("\t last seq num \n");
		}
		
		//catch size of payload	
		memcpy(nb_data_rcv, msgRcv+NUMSEQ_SIZE+FRAGM_FLAG_SIZE, DATA_SIZE);
	}

}



void file_reception(){

	printf("\t reception file begins\n");
	count=1;
	drop_count=0;
	int position_in_file;
	
	okay_file=1; //check if the file is received : 1 not completely received 0 full received
	char all_file_received[file_size];
	printf("reception buffer create\n");
	
	while(okay_file==1){

		desencapsulation();
		
		if (okay_file==0){
			break;
		}
		if(atoi(num_seq)==count && drop_count<DROP){ // if it's the good sequence received

			position_in_file=(count-1)*MDS;
			memcpy(all_file_received+position_in_file,msgRcv+H_SIZE,size_data_recv-H_SIZE);
			//printf("size payload received : %s \n",nb_data_rcv);
			
			sprintf(msgSnd,"ACK_%d", count);
			count++;
			drop_count++;
			
		}
		else{
			drop_count=0;
			printf("Warning : Segment missing. Need segment num %d, actually recieved %s but we lost it\n", count, num_seq);
			sprintf(msgSnd,"ACK_%d", count-1);//warning we need the previous segment
		}
	
		sendto(desc,msgSnd,strlen(msgSnd),0, (struct sockaddr *)&addr_data, alen );
		printf("...Message  : %s sent...\n", msgSnd);
	}

	//write all bytes from the all_file_received to file	
	size_data_recv=fwrite(all_file_received,1,file_size,f_out);
			if(size_data_recv==0){
				printf("Error, impossible to write in the output file\n");
				exit(-1);
			}
			else if (size_data_recv==file_size){
				printf("succes\n");
			}
}
	
void catch_file_size(){
	desencapsulation();
	
	if(atoi(num_seq)==0){
		file_size=atoi(msgRcv+H_SIZE);
		printf("file size :%d ou %s\n",file_size, msgRcv+H_SIZE);
		sprintf(msgSnd,"ACK_0");
		sendto(desc,msgSnd,strlen(msgSnd),0, (struct sockaddr *)&addr_data, alen );
		printf("...Message  : %s sent...\n", msgSnd);
		
		file_reception();
	}
	else{
		printf("error : impossible to catch file size\n");
	}

}

void conversation(){

	printf("\t conversation begins, you can write stop at any time to close the connexion\n");
	
	while (connected==1) {
		
			memset(msgSnd,0,MSS);
			fgets(msgSnd, MSS, stdin);
			sendto(desc,msgSnd,strlen(msgSnd),0, (struct sockaddr *)&addr_data, alen );
			
			
			if (strcmp(msgSnd,"stop\n") == 0) {
				connected= 0;
			}
			else{
				recvfrom(desc, msgRcv, MSS,0,(struct sockaddr *)&addr_data, &alen);
				printf("Answer : %s\n",msgRcv);
				memset(msgRcv,0,MSS);
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
		sprintf(name_output_file,"sortie_%d_%s",getpid(),argv[3]);
		printf("name output file : %s \n",name_output_file);
		
		char* str;
		do{
			//send the name of file to receive
			sendto(desc,argv[3],strlen(argv[3]),0, (struct sockaddr *)&addr_data, alen );
	
			recvfrom(desc, msgRcv, MSS,0,(struct sockaddr *)&addr_data, &alen);
			printf("received : %s\n", msgRcv);
			// check if it's the good ack
				str=strtok(msgRcv, "_");
				if(strcmp(str,"ACK")==0){
					str=strtok(NULL, " ");	
				}
		}while(atoi(str)!=0);

		memset(msgRcv,0,MSS);

		f_out=fopen(name_output_file, "a");
		if(f_out==NULL)
			printf("Error file\n");

		else{
			catch_file_size();
			
			conversation();
		}
		
		close(desc);
		fclose(f_out);
	}
	return 0;
}
