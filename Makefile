#########################################################
# Makefile pour les programmes du serveur PRS
# Aubert Soline, Martini Lisa, 3TC 3
#########################################################

CC = gcc
FLAGS = -pthread

all : clean serveur1-AubertMartini

serveur1-AubertMartini : serveur1-AubertMartini.c serveur1-AubertMartini.h
	${CC} ${FLAGS} serveur1-AubertMartini.c -o serveur1-AubertMartini
	


#Pour nettoyer (enlever tous les fichiers générés
clean :
	\rm -f *.o *~ serveur1 copy_*
