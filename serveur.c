#include "serveur.h"

void connexion(){ //mise en place connexion simulation demande de connexion tcp

	printf("Connexion process \n");

	alen= sizeof(client);

	msgSize= recvfrom(desc,recep,MSS,0,(struct sockaddr*)&client, &alen);
	printf("first message received %s\n",recep);

	if(msgSize>0){
		if(strcmp(recep,"SYN")==0){
			sprintf(sndBuf, "SYN-ACK %d", port_data);

			sendto(desc,sndBuf,sizeof(sndBuf),0, (struct sockaddr*)&client, alen);

			printf("Message sent: %s \n", sndBuf);
			memset(sndBuf,0,MSS);
			memset(recep,0,MSS);

			msgSize=recvfrom(desc,recep,MSS,0,(struct sockaddr*)&client, &alen);
			
			if(msgSize>0){
				if(strcmp(recep,"ACK")==0){
					printf("Connexion established\n");
					memset(recep,0,MSS);
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
		memset(sndBuf,0,MSS);
		memset(recep,0,MSS);
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
	count=1;
	printf("\t conversation begins\n");
		while (connected==1) {
				
			memset(recep,0,MSS);
			msgSize= recvfrom(desc_data_sock,recep,MSS,0,(struct sockaddr*)&client, &alen);

			if(msgSize > 0) {
				
				printf("received : %s\n", recep);
				if(strcmp(recep, "stop\n")==0){
					printf("Connexion ended by client\n");
					memset(recep,0,MSS);
					connected=0;
				}
				else{
					sprintf(sndBuf,"ACK_%d",count);
					sendto(desc_data_sock,sndBuf,strlen(sndBuf),0, (struct sockaddr*)&client, alen);
					printf("%s sent \n",sndBuf);
					memset(sndBuf,0,MSS);
					memset(recep,0,MSS);
					count++;
				}
				
			}
			else{
				printf("Reception error\n");
			}	

					
		}

}


void send_file(){

	//init global variables
	pthread_mutex_lock(&mutex);
	count=0;
	fragm=1;
	cwnd=3;
	flight_size=0;
	pthread_mutex_unlock(&mutex);
	
	//init local variables
	int size_data_to_send=1;
	char all_file[file_size];
	int curseur=0;
	long nb_segment;
	int total_size;
	int i=0;
	

	//read all file :
	while(feof(fin)==0){
	size_data_to_send=fread(all_file+curseur,1, MDS,fin);
	
	curseur=curseur+size_data_to_send;
	
	}
	printf("%d bytes written in all_file buffer\n",curseur);
	
	//create a thread to receive ACKs
	pthread_t listener;
	pthread_attr_t attr_listener;

	 /* For portability, explicitly create threads in a joinable state */
	pthread_attr_init(&attr_listener);
	pthread_attr_setdetachstate(&attr_listener, PTHREAD_CREATE_JOINABLE);
	
		

	//start sequencing the file
	nb_segment=(int)(file_size/MDS)+1;// need to add 1 to count the last segment with a fewer size
	printf("nb segment =%ld\n", nb_segment);

	
	//listener thread launched
	pthread_create(&listener, &attr_listener, receive_ACK, (void *)nb_segment);
	
	
	//beginning of the file sending
	printf("\t send_file begins\n");
	
	while(count<=nb_segment){
	
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&ACK_received,&mutex);

	
		while(flight_size<cwnd){	
			
				memset(sndBuf,0,MSS);

			/*update the payload */
			
				//if it's the last sequence
				if(count==nb_segment){
					fragm=0;
					printf("\tlast seq to send, reach end of file\n");

					size_data_to_send=(int)(file_size-((count-1)*MDS));//last size to send

					// copy the payload (all_file) inside the buffer to send.
					memcpy(sndBuf+H_SIZE,all_file+(count-1)*MDS,size_data_to_send); //count-1 to start at the good offset in the buffer
				}
				else if (count==0){ //write the size of the file for client buffer
			
					sprintf(sndBuf+H_SIZE,"%d",file_size);
					size_data_to_send=strlen(sndBuf+H_SIZE);
				}
				else{
					fragm=1;
					size_data_to_send=MDS;
					// copy the payload (all_file) inside the buffer to send.
					memcpy(sndBuf+H_SIZE,all_file+(count-1)*MDS,size_data_to_send); //count-1 to start at the good offset in the buffer
				}

			//fill the sndBuf with number of seg, flag, size of data and data from all_file
	
				sprintf(sndBuf,"%d",count); // write the number of sequence in the buffer
				sprintf(sndBuf+NUMSEQ_SIZE,"%d",fragm); //write the fragmentation flag
				sprintf(sndBuf+NUMSEQ_SIZE+FRAGM_FLAG_SIZE,"%d",size_data_to_send);// write size of the payload in the buffer
		
		
				//printf("size data to send : %d and size segment to send  : %d \n", size_data_to_send,size_data_to_send+H_SIZE);
		

			//send the sequence
				total_size=size_data_to_send+H_SIZE;
				sendto(desc_data_sock,sndBuf,total_size,0, (struct sockaddr*)&client, alen);

				flight_size++;
				count++;
				printf("num seq : %s\n",sndBuf);
				if(fragm==0){
					break;
				}
					
		}//end of window
	pthread_mutex_unlock(&mutex);	
		
	}//end of file

	
	  /* Wait for all threads to complete */
		  
	pthread_join(listener, NULL);
		  
	printf ("Main(): Waited on listener thread. Done.\n");

		  /* Clean up and exit */
	pthread_attr_destroy(&attr_listener);
	
	
}

void *receive_ACK(void *arg ){

	int rcvMsg_Size;
	long nb_total_segment=(long)arg;
	
	
	char* str;
	
	do{
		memset(recep,0,MSS);
		
		
		pthread_mutex_lock(&mutex);
		if(count<=nb_total_segment){
			pthread_cond_signal(&ACK_received);//signal to synchronize the sender thread
			
		}
		pthread_mutex_unlock(&mutex);
	
		//wait for ack
			rcvMsg_Size= recvfrom(desc_data_sock,recep,MSS,0,(struct sockaddr*)&client, &alen);
			while(rcvMsg_Size > 0) {
				
					pthread_mutex_lock(&mutex);
					printf("received : %s\n", recep);
				// check if it's the good ack
					str=strtok(recep, "_");
					if(strcmp(str,"ACK")==0){
						str=strtok(NULL, " ");
						
						if (atoi(str)<=count){
							flight_size--;
						}
						else{
							flight_size--;
							count=atoi(str)+1;
							printf("count=%d\n",count);
						}
						
						rcvMsg_Size=0; 
						pthread_cond_signal(&ACK_received);//signal to synchronize the sender thread
						pthread_mutex_unlock(&mutex);	
					}
					
			}
	}while(atoi(str) !=  nb_total_segment);
	
	
	pthread_exit(NULL);

}


int catch_file_size(){

	int size;
	fseek (fin, 0, SEEK_END);   // non-portable
	size=ftell (fin);
	printf("size of file : %d\n",size);         
	rewind(fin);
	return size;
	
}

void *send_thread(void *num_port){

		
		port=atoi((char* )num_port);
		port_data=port+1;

  		
  		init();
		connexion(); 
		
	
		//échange d'informations possibles sur 2eme port 

		printf("information exchange on port : %d \n",port_data); 
		
		do {
			msgSize= recvfrom(desc_data_sock,recep,MSS,0,(struct sockaddr*)&client, &alen);

			if(msgSize > 0) {
				
				printf("received : %s\n", recep);
				fin=fopen(recep, "r");
				if(fin==NULL)
					printf("Error : file not found \n");
				else{
					sprintf(sndBuf,"ACK_0");
					sendto(desc_data_sock,sndBuf,strlen(sndBuf),0, (struct sockaddr*)&client, alen);
					printf("%s sent \n",sndBuf);
					memset(sndBuf,0,MSS);
					memset(recep,0,MSS);
				}
			}
		}while(fin==NULL);
		
		close(desc);
		
		file_size=catch_file_size();
		printf("size of file : %d\n",file_size); 
		
		send_file();
		conversation();
		fclose(fin);
		
		close(desc_data_sock);
		pthread_exit(NULL);

}



int main (int argc, char *argv[]) {

	if(argc!=2){
		perror("Usage : ./serveur <no_port> \n");
		exit(0);
	}
	else{
	
		int num_thread=1;
		pthread_t sender;
		pthread_attr_t attr;

		  /* Initialize mutex and condition variable objects */
		  pthread_mutex_init(&mutex, NULL);
		  pthread_cond_init (&ACK_received, NULL);

		  /* For portability, explicitly create threads in a joinable state */
		  pthread_attr_init(&attr);
		  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		  pthread_create(&sender, &attr, send_thread, (void *)argv[1]);
		

		  /* Wait for all threads to complete */
		  
		    pthread_join(sender, NULL);
		  
		  printf ("Main(): Waited on 1 thread. Done.\n");

		  /* Clean up and exit */
		pthread_attr_destroy(&attr);
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&ACK_received);
		pthread_exit(NULL);
		
		
	}
	return 0;
}
