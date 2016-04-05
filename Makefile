#########################################################
# Makefile pour les programmes du serveur et client PRS
# Aubert Soline, Martini Lisa, 3TC 3
#########################################################

CC = gcc
LD = gcc

all : clean serveur client

serveur : serveur.c serveur.h
	${CC} serveur.c -o serveur
	
client : client.c client.h
	${CC} client.c  -o client

#Pour nettoyer (enlever tous les fichiers générés
clean :
	\rm -f *.o *~ serveur client sortie*
