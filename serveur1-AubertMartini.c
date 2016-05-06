#include "serveur1-AubertMartini.h"

int checkACK(){
	
	char begin[3]="ACK";
	int i;
	for(i=0;i<3;i++){
		if(begin[i]!=recep[i]){
			return 0;
		}
	}
	return 1;
}


char *str_sub ( int start,  int end)
{
   char *new_s = NULL;

   if (recep != NULL && start < end)
   {

      new_s = malloc (sizeof (*new_s) * (end - start + 2));
      if (new_s != NULL)
      {
         int i;
         for (i = start; i <= end; i++)
         {
            new_s[i-start] = recep[i];
         }
         new_s[i-start] = '\0';
      }
      else
      {
         fprintf (stderr, "Memoire insuffisante\n");
         exit (EXIT_FAILURE);
      }
   }
   return new_s;
}


void connexion(){ //mise en place connexion simulation demande de connexion tcp

	if(debug==TRUE){
		printf("Connexion process \n");
	}
	
	alen= sizeof(client);

	msgSize= recvfrom(desc,recep,MSS,0,(struct sockaddr*)&client, &alen);
	if(debug==TRUE){
		printf("first message received %s\n",recep);
	}
	
	if(msgSize>0){
		if(strcmp(recep,"SYN")==0){
			sprintf(sndBuf, "SYN-ACK%d", port_data);

			sendto(desc,sndBuf,sizeof(sndBuf),0, (struct sockaddr*)&client, alen);
			
			if(debug==TRUE){
				printf("Message sent: %s \n", sndBuf);
			}
			
			memset(sndBuf,0,MSS);
			memset(recep,0,MSS);

			msgSize=recvfrom(desc,recep,MSS,0,(struct sockaddr*)&client, &alen);
			
			if(msgSize>0){
				if(strcmp(recep,"ACK")==0){
					if(debug==TRUE){
						printf("Connexion established\n");
					}
					
					memset(recep,0,MSS);
					connected++;
				}
				else{
					if(debug==TRUE){
						printf("Connexion failed at last ACK point\n");
					}
					
					exit(-1);
				}
			}
			else{
				if(debug==TRUE){
					printf("Connexion failed at SYN/SYN-ACK point\n");
				}
				exit(-1);
			}		
		}
		else{
			if(debug==TRUE){
				printf("Connexion failed : reception error\n");
			}
			exit(-1);
		}
	}
}
				
		
void init( ){ //initialisation des variables et création/lien socket
	
		if(debug==TRUE){printf("Initialisation \n");}

		memset((char*)&serveur,0,sizeof(serveur));
		memset((char*)&client,0,sizeof(client));
		memset(sndBuf,0,MSS);
		memset(recep,0,MSS);
		
		
	//init global variables

		count=1;
		cwnd=1;
		flight_size=0;
		
		okFile=FALSE;

		valid= 1;
		connected=0;
		
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
		
	//initialisation of serveur and data addr	
		serveur.sin_family= AF_INET;
		data.sin_family= AF_INET;
		
		serveur.sin_port= htons(port);
		data.sin_port= htons(port_data);
		
		serveur.sin_addr.s_addr= htonl(INADDR_ANY);
		data.sin_addr.s_addr= htonl(INADDR_ANY);		

	//bind sockets and their addr
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
		
	
	 /* For portability, explicitly create threads in a joinable state */
	pthread_attr_init(&attr_listener);
	pthread_attr_setdetachstate(&attr_listener, PTHREAD_CREATE_JOINABLE);
	pthread_attr_init(&attr_sender);
	pthread_attr_setdetachstate(&attr_sender, PTHREAD_CREATE_JOINABLE);
	
	/* Initialize mutex */
	pthread_mutex_init(&mutex, NULL);
	
	
}	


void conversation(){

	
	if(debug==TRUE){printf("\t conversation begins\n");}
	
				
			memset(recep,0,MSS);
			msgSize= recvfrom(desc_data_sock,recep,MSS,0,(struct sockaddr*)&client, &alen);

			if(msgSize > 0) {
				
				if(debug==TRUE){printf("received : %s\n", recep);}
				fin=fopen(recep, "r");
					
				if(fin==NULL){
					perror("Error : file not found \n");
				}
				else{
					file_size=catch_file_size();
					if(debug==TRUE){printf("size of file : %d\n",file_size); }
						
					
						// create 2 thread one to send the file the other to receive ACKs
						pthread_create(&sender, &attr_sender, send_file, (void *)1);
						pthread_create(&listener, &attr_listener, receive_ACK, (void *)2);

		 				 /* Wait for all threads to complete */
		  
		    				pthread_join(sender, NULL);
		    				pthread_join(listener, NULL);
		  				if(debug==TRUE){printf ("conversation(): Waited on 2 thread. Done.\n");}
				}
					
					
				memset(sndBuf,0,MSS);
				memset(recep,0,MSS);
					
			}

}

void *send_file(void *arg ){

		
	//init local variables
	int size_data_to_send=1;
	
	//sequencing the file
	nb_segment_total=(int)(file_size/MDS)+1;// need to add 1 to count the last segment with a fewer size
	if(debug==TRUE){printf("nb segment =%d\n", nb_segment_total);}	
	
	char all_file[file_size];
	int curseur=0;
	int total_size;
	
	

	//read all file :
	while(feof(fin)==0){
	
		size_data_to_send=fread(all_file+curseur,1, MDS,fin);
	
		curseur=curseur+size_data_to_send;
	
	}
	if(debug==TRUE){printf("%d bytes written in all_file buffer\n",curseur);}
		
	
	//beginning of sending the file 
	if(debug==TRUE){printf("\t send_file begins\n");}
	
	while( okFile==FALSE){
	
		if(count<=nb_segment_total){
			
		
			pthread_mutex_lock(&mutex);
		
			while(flight_size<(int)cwnd){	
			
					memset(sndBuf,0,MSS);

				/*update the payload */
			
					//if it's the last sequence
					if(count==nb_segment_total){
						
						if(debug==TRUE){printf("\tlast seq to send, reach end of file\n");}
					
						size_data_to_send=(int)(file_size-((count-1)*MDS));//last size to send

						// copy the payload (all_file) inside the buffer to send.
						memcpy(sndBuf+NUMSEQ_SIZE,all_file+(count-1)*MDS,size_data_to_send); //count-1 to start at the good offset in the buffer
					}
					
					else{
						
						size_data_to_send=MDS;
						// copy the payload (all_file) inside the buffer to send.
						memcpy(sndBuf+NUMSEQ_SIZE,all_file+(count-1)*MDS,size_data_to_send); //count-1 to start at the good offset in the buffer
					}

				//fill the sndBuf with number of seg
	
					sprintf(sndBuf,"%d",count); // write the number of sequence in the buffer
					
					
				//send the sequence
					total_size=size_data_to_send+NUMSEQ_SIZE;
					sendto(desc_data_sock,sndBuf,total_size,0, (struct sockaddr*)&client, alen);

					flight_size++;
					count++;
					if(debug==TRUE){printf("num seq : %s\n",sndBuf);}
					
					if(size_data_to_send<MDS){//if it's the last segment
						break;
					}
					
			}//end of window
			pthread_mutex_unlock(&mutex);	
		}
	}//end of file
	
	memset(sndBuf,0,MSS);
	sprintf(sndBuf,"FIN");
	
	sendto(desc_data_sock,sndBuf,3,0, (struct sockaddr*)&client, alen);
	
	pthread_exit(NULL);	
	
}

void *receive_ACK(void *arg ){

	int rcvMsg_Size;	
	char* str;
	int last_ack=0;
	int duplicate=0;
	
	do{
		memset(recep,0,MSS);
		
		
							
		
		//wait for ack
			rcvMsg_Size= recvfrom(desc_data_sock,recep,MSS,0,(struct sockaddr*)&client, &alen);
			while(rcvMsg_Size > 0) {			
				
				// check if it's the good ack
					
					if(checkACK()==TRUE){
						str=str_sub(3,strlen(recep));						
						if( atoi(str)<=last_ack && duplicate >= DUPLICATE){ //after 3 duplicate ack , segment are ignored but transmission can keep going on thanks to flight_size
							if(flight_size>0)flight_size--;
							else flight_size=0;
							if(debug==TRUE){printf("ignored\n");}
							
						}
						else{
							/* critical section access to variable used by the other send, need mutex protection*/
							pthread_mutex_lock(&mutex);
						
							if(cwnd>ssthresh){
								if(debug==TRUE)printf("Congestion avoidance\n");	
								cwnd+=1/cwnd;
								if((int)cwnd>=RWND){
									cwnd=RWND;
								}
								
							}
							else{
								cwnd++;
								if((int)cwnd>=RWND){
									cwnd=RWND;
								}
							}
							
							if (atoi(str)<=count){
								
								if(atoi(str)==last_ack){//if it's a duplicate
									duplicate++;
								}
								
								
								
								if(atoi(str)==count-1){ 
									if(debug==TRUE){printf("received : %s\n", recep);}
									flight_size=0;
									last_ack=atoi(str);
								} 	
								else if(atoi(str)>last_ack && atoi(str)<count-1){//if count > ack > last_ack
									if(debug==TRUE){printf("received : %s\n", recep);}
									duplicate=0;
									flight_size=count-atoi(str)-1;
									last_ack=atoi(str);
								}
								else {
									//if(debug==TRUE)printf("Already acked\n");
								}
								
								
							}
							else{//after retransmission serveur can receive an ack > count
								count=atoi(str)+1;
								flight_size=0;
								last_ack=atoi(str);
							
							}
							if(duplicate==DUPLICATE){
								count=last_ack+1;//send the segment last_ack+1 again
								ssthresh=cwnd/2;
								cwnd=ssthresh;
								flight_size=0;
							}
						
							pthread_mutex_unlock(&mutex);
							/*end of critical section */
						}
						rcvMsg_Size=0; 
					}
						
					
			}
	}while(atoi(str) != nb_segment_total);
	okFile=TRUE;
	
	pthread_exit(NULL);

}


int catch_file_size(){

	int size;
	fseek (fin, 0, SEEK_END);   // non-portable
	size=ftell (fin);      
	rewind(fin);
	return size;
	
}


int main (int argc, char *argv[]) {

	if(argc<2 && argc>3){
		perror("Usage : ./serveur <no_port> \n");
		exit(0);
	}
	
	else{
		if(argc==2){
			debug=FALSE;
			}
		else{
			debug=TRUE;
		}
		port=atoi(argv[1]);
		nbClient=1;
		while(1){
			
			
			port_data=port+nbClient; 
			
				
  				init();
				connexion();
				
						
				close(desc); //closing control socket
				
				if(debug==TRUE){
					printf("information exchange on port : %d \n",port_data); 
				}
				conversation();
				
				/* Clean up and exit */
				pthread_attr_destroy(&attr_listener);
				pthread_attr_destroy(&attr_sender);
				pthread_mutex_destroy(&mutex);
				close(desc_data_sock);//closing communication socket
			
		}
	}
	
			
	return 0;
}