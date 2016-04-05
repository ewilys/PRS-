CC = gcc
LD = gcc

all : clean serveur client

serveur : serveur.c
	${CC} serveur.c -o serveur
	
client : client.c
	${CC} client.c -o client

clean :
	\rm -f *.o *~ serveur client sortie*
