#include "Serveur3-AubertMartini.h"

void new_connexion(){
	nbClient++;
}

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

long estimateRTT(double cste, long oldRTT, long mes){
	//printf("%ld oldrtt, %ld mes\n",oldRTT,mes);
	return (cste*oldRTT + (1-cste)*mes);
}

struct timespec estimateTimeout(long RTT){
	struct timespec newTimeout;
	long temp=RTT*X_RTT;
	newTimeout.tv_sec=(long)temp/NANO;
	newTimeout.tv_nsec=(long) temp % NANO;
	if(newTimeout.tv_nsec<MIN_RTT){
		newTimeout.tv_nsec=MIN_RTT;
	}
	//newTimeout.tv_nsec=5000000;
	//printf("temp: %ld ms, sec : %ld s + %ld ms\n",temp,newTimeout.tv_sec,newTimeout.tv_usec);
	
	return newTimeout;
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

			gettimeofday(&start, NULL);
			sendto(desc,sndBuf,sizeof(sndBuf),0, (struct sockaddr*)&client, alen);
			
			if(debug==TRUE){
				printf("Message sent: %s \n", sndBuf);
			}
			
			memset(sndBuf,0,MSS);
			memset(recep,0,MSS);
			
			
			//RTT
			gettimeofday(&end,NULL);
			mesure=((end.tv_sec-start.tv_sec)*NANO)+(end.tv_nsec-start.tv_nsec);
			RTT=estimateRTT(alpha, RTT, mesure);
			//printf(" Mesure 0: %ld ms %ld ms\n",mesure, RTT);
							
			timeout=estimateTimeout(RTT);
			save_timeout[0]=timeout;


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
		cwnd=8;
		flight_size=0;
		ssthresh=512;
		
		nb_seg_lost=0;
		
		RTT=1000000000;//1sec
		alpha=0.8;
		
		size_to_read=MAX_SIZE_BUFCIRCULAIRE;
		
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
		printf("process %d NbClients : %d\n", getpid(), nbClient);
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
				
				//printf("received : %s\n", recep);
				fin=fopen(recep, "r");
					
				if(fin==NULL){
					perror("From serveur Error : file not found \n");
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
			fclose(fin);

}

int min(int cwnd, int rwnd){
	if (cwnd>=rwnd)
		return rwnd;
	else 
		return cwnd;
}

void *send_file(void *arg ){

		
	//init local variables
	int size_data_to_send=1;
	
	//sequencing the file
	nb_segment_total=(int)(file_size/MDS)+1;// need to add 1 to count the last segment with a fewer size
	if(debug==TRUE){printf("nb segment =%d\n", nb_segment_total);}	
	
	char all_file[MAX_SIZE_BUFCIRCULAIRE];
	int curseur=0;
	int i=0;
	int ptSeg_lost=0;
	int total_size;
	int window=cwnd;
	readFile=TRUE;

		
	
	//beginning of sending the file 
	if(debug==TRUE){printf("\t send_file begins\n");}
	
	while( okFile==FALSE){
	
		pthread_mutex_lock(&mutex);
		if(readFile==TRUE){
			//read all file :
			if(curseur<file_size){
				if(debug==TRUE){printf("avant ecriture %d curseur  %d size to read\n",curseur, size_to_read);}
				if ((curseur % MAX_SIZE_BUFCIRCULAIRE)+size_to_read > MAX_SIZE_BUFCIRCULAIRE){
						//when size-to-read overcome max size buf circu
					size_data_to_send=fread(all_file+(curseur % MAX_SIZE_BUFCIRCULAIRE),1,(MAX_SIZE_BUFCIRCULAIRE-(curseur % MAX_SIZE_BUFCIRCULAIRE)),fin);
					curseur=curseur+size_data_to_send;
					size_to_read-=size_data_to_send;
					if(debug==TRUE){printf(" 1ere ecriture %d curseur  %d size to read\n",curseur, size_to_read);}
				}
				size_data_to_send=fread(all_file+(curseur % MAX_SIZE_BUFCIRCULAIRE),1, size_to_read,fin);
				curseur=curseur+size_data_to_send;
				
				if(debug==TRUE){printf("%d/%d bytes written in all_file buffer (%d/%d this time) rep ferror %d\n",curseur, file_size, size_data_to_send,size_to_read,ferror(fin));}
			}
			
			readFile=FALSE;
		}	
		
		if(count<=nb_segment_total || nb_seg_lost !=0){	
			window=min((int)cwnd,RWND);
			
			while(flight_size<window ){	
			
					//if(debug==TRUE){printf("\t window : %d\n",window);}
					memset(sndBuf,0,MSS);

					/*update the payload */
					
					if(nb_seg_lost!=0){// send those who need to be retransmit only
						for(i=ptSeg_lost;i<nb_seg_lost;i++){
							memset(sndBuf,0,MSS);
							if(seg_lost[i]==nb_segment_total){
								size_data_to_send=(int)(file_size-((seg_lost[i]-1)*MDS));
							}
							else{
								size_data_to_send=MDS;
							}
							memcpy(sndBuf+NUMSEQ_SIZE,all_file+((seg_lost[i]-1)*MDS)%MAX_SIZE_BUFCIRCULAIRE,size_data_to_send); 
							sprintf(sndBuf,"%d",seg_lost[i]); // write the number of sequence in the buffer
							total_size=size_data_to_send+NUMSEQ_SIZE;
							gettimeofday( &start,NULL);
							sendto(desc_data_sock,sndBuf,total_size,0, (struct sockaddr*)&client, alen);
					
							save_start[seg_lost[i] % MAX_RTT]=start;

							flight_size++;
							cwnd=ssthresh;
							
							if (cwnd<=flight_size){
								cwnd=flight_size;
							}
							
							ptSeg_lost++;
							if(debug==TRUE){printf("num seq : %s et fs:%d\n",sndBuf,flight_size);}
							if(seg_lost[i]==count){count++;}
							if(size_data_to_send<MDS || flight_size==window){//if it's the last segment
								break;
							}
						}
						if(ptSeg_lost==nb_seg_lost){
							if(debug==TRUE){printf("remise à zero tous les seg perdus ont été retransmis\n");}
							ptSeg_lost=0;
							nb_seg_lost=0;
						}
						else{
							if(debug==TRUE){printf("il reste des seg perdus non retransmis\n");}
						}
					}	
					
					
					if(flight_size == window || count ==nb_segment_total+1){
						
						if(debug==TRUE){printf("fs=win break \n");}
						break;
					}
					
					if(count<=nb_segment_total){		
						//if it's the last sequence
						if(count==nb_segment_total){
						
							if(debug==TRUE){printf("\tlast seq to send, reach end of file\n");}
					
							size_data_to_send=(int)(file_size-((count-1)*MDS));//last size to send

							// copy the payload (all_file) inside the buffer to send.
							memcpy(sndBuf+NUMSEQ_SIZE,all_file+((count-1)*MDS)%MAX_SIZE_BUFCIRCULAIRE,size_data_to_send); //count-1 to start at the good offset in the buffer
						}
					
						else {
						
							size_data_to_send=MDS;
							// copy the payload (all_file) inside the buffer to send.
							memcpy(sndBuf+NUMSEQ_SIZE,all_file+((count-1)*MDS)%MAX_SIZE_BUFCIRCULAIRE,size_data_to_send); //count-1 to start at the good offset in the buffer
						}

					//fill the sndBuf with number of seg
	
						sprintf(sndBuf,"%d",count); // write the number of sequence in the buffer
					
					
					//send the sequence
						total_size=size_data_to_send+NUMSEQ_SIZE;
						gettimeofday( &start,NULL);
						sendto(desc_data_sock,sndBuf,total_size,0, (struct sockaddr*)&client, alen);
					
						save_start[count % MAX_RTT]=start;

						flight_size++;
						count++;
						if(debug==TRUE){printf("num seq : %s\n",sndBuf);}
					
						if(size_data_to_send<MDS){//if it's the last segment
							break;
						}
					
				}//end of window
			
			}
				
		}
		pthread_mutex_unlock(&mutex);
	}//end of file
	
	memset(sndBuf,0,MSS);
	sprintf(sndBuf,"FIN");
	if(debug==TRUE){printf("FIN send\n");}
	sendto(desc_data_sock,sndBuf,3,0, (struct sockaddr*)&client, alen);
	
	pthread_exit(NULL);	
	
}

void *receive_ACK(void *arg ){

	int rcvMsg_Size;	
	char* str="0";
	int last_ack=0;
	int new_timeout=0;
	int duplicate=0;
	int x=1;
	int rep=0;
	int retransmission=0;
	fd_set readfs;
	
	
	do{
		memset(recep,0,MSS);
		FD_ZERO(&readfs);
		FD_SET(desc_data_sock,&readfs);
		
		
		rep= pselect(desc_data_sock+1,&readfs,NULL,NULL,&save_timeout[new_timeout % MAX_RTT],NULL);
			
			
		if(rep ==0 && retransmission<2){//timeout, no ack received before time is running out : congestion
			if(debug==TRUE ){printf(" congestion, timeout :%ld  retransmission : %d\n",save_timeout[new_timeout % MAX_RTT].tv_nsec,last_ack+1);}
			retransmission++;
			pthread_mutex_lock(&mutex);
			
			
			if (retransmission<=2 && last_ack+retransmission<=nb_segment_total){
				ssthresh=min((int)cwnd,RWND)/2;
				seg_lost[nb_seg_lost]=last_ack+1;
				nb_seg_lost++;
				flight_size--;//compulsory for sending back the ack
				cwnd=flight_size+1;
			}

			
			

			pthread_mutex_unlock(&mutex);
			
		
		}
		else if (rep ==-1){
			perror("Error, select failed\n");
			exit(0);
		}
		else if(FD_ISSET(desc_data_sock,&readfs)){				
			
		//wait for ack
			rcvMsg_Size= recvfrom(desc_data_sock,recep,MSS,0,(struct sockaddr*)&client, &alen);
			if(rcvMsg_Size > 0) {			
				
				// check if it's the good ack
					
					if(checkACK()==TRUE){
						str=str_sub(3,strlen(recep));
						
						pthread_mutex_lock(&mutex);
						
						if (atoi(str)!=last_ack){
							
							gettimeofday(&end,NULL);
							mesure=((end.tv_sec-save_start[atoi(str) % MAX_RTT].tv_sec)*NANO)+(end.tv_nsec-save_start[atoi(str) % MAX_RTT].tv_nsec);
							RTT=estimateRTT(alpha, RTT, mesure);
								//if(debug== TRUE){printf(" Mesure %d: %ld ms %ld ms\n",atoi(str),mesure, RTT);}
						
							timeout=estimateTimeout(RTT);
							save_timeout[atoi(str) % MAX_RTT]=timeout;
							
							new_timeout=atoi(str);
								
						}	
											
						if( atoi(str)<=last_ack && duplicate >= DUPLICATE){ //after 3 duplicate ack , segment are ignored but transmission can keep going on thanks to flight_size
										
							if(debug==TRUE){printf("ignored (fs: %d)\n",flight_size);}
							
							//si des atoi(str)<last_ack alors on ne diminue pas le flight_size car deja pris en compte
							if(atoi(str)==last_ack){
								cwnd+= (1/cwnd); //pour eviter tous les ignored 
								
							}
							
						}
						
						else{
							
							
							/* critical section access to variable used by the other send, need mutex protection*/
							
						
							if(debug==TRUE){printf("received : %s\n", recep);}
							
							
							if (atoi(str)<=count-1){
								
								if(atoi(str)==last_ack){//if it's a duplicate
									duplicate++;
								}
																
								if (last_ack< atoi(str) && atoi(str)<=count-1){//if count > ack > last_ack
									flight_size=count-atoi(str)-1;
									retransmission=0;
									
									if (cwnd >ssthresh){//congestion avoidance
										if (debug ==TRUE){printf("congestion avoidance\n");}
										cwnd+= (1/cwnd)*(atoi(str)-last_ack);
										}
									else{
										cwnd=cwnd+(atoi(str)-last_ack);
									}
									
									last_ack=atoi(str);
									duplicate=0;
									if(debug==TRUE){printf("\t \t fs :%d  cwnd: %f \n",flight_size,cwnd);}
								}
								
								
							}
							
							if(duplicate==DUPLICATE){
								if (debug ==TRUE){printf("retransmission of %d \n",last_ack+1);}
								ssthresh=min((int)cwnd,RWND)/2;
								seg_lost[nb_seg_lost]=last_ack+1;
								nb_seg_lost++;
								flight_size--;
								cwnd=flight_size+duplicate;
								retransmission++;
							}
							
							if (last_ack >= x*((int)(MAX_SIZE_BUFCIRCULAIRE/(2*MDS)))){ 
								x++;
								size_to_read=MAX_SIZE_BUFCIRCULAIRE/2;								
								readFile=TRUE;
							}
							
							/*end of critical section */
							
						}
						pthread_mutex_unlock(&mutex);
						rcvMsg_Size=0; 
					}
						
					
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
		int pid;
		port=atoi(argv[1]);
		nbClient=1;
		
		while(1){	
			
			signal(SIGUSR1, new_connexion);
			
			pid=fork();
			if(pid == 0){//son process
				
				port_data=getpid()+nbClient;
				while(port_data > 10000){
					port_data = port_data%10000;
				}
				if(port_data <= 1024) port_data+=1024;
				printf("port_data : %d\n", port_data);
				//printf("process communicating with client %d\n",nbClient);
  				init();
				connexion();
				
				kill(getppid(), SIGUSR1); //allow father to get a new connexion
						
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
			else{
				pause();
			}
		}
			
	return 0;
	}
}
