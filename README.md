# PRS-Client1

scenario 1:

serveur for client 1 :

client 1 : send ACK in disorder, there is loss some times he sends lots of duplicate ack, he can loose the first ack need timeout and RTT.

to remove printf use only 2 arguments when launching , otherwise use a 3rd argument 1 preferentely

circular buffer implanted to send eavier files. 

Variables : RWND=40, alpha=0.8, X_RTT=6, MIN_RTT=7ms
SLow start and congestion avoidance are implanted, we decided to keep congestion avoidance because throughput were more stabled within rather than with only slow start and greater RWND. 

We have a better throughput with slow start and rwnd=50 only for small file (1Mo) but with bigger file a congestion avoidance is more appropriate.

