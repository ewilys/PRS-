#########################################################
# Makefile pour les programmes du serveur PRS
# Aubert Soline, Martini Lisa, 3TC 3
#########################################################

CC = gcc
FLAGS = -lpthread

all : clean Serveur3-AubertMartini

Serveur3-AubertMartini : Serveur3-AubertMartini.c Serveur3-AubertMartini.h
	${CC} Serveur3-AubertMartini.c -o Serveur3-AubertMartini ${FLAGS}
	


#Pour nettoyer (enlever tous les fichiers générés
clean :
	\rm -f *.o *~ Serveur3 copy_*
