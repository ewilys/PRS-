#########################################################
# Makefile pour les programmes du serveur PRS
# Aubert Soline, Martini Lisa, 3TC 3
#########################################################

CC = gcc
FLAGS = -lpthread

all : clean serveur2-AubertMartini

serveur1-AubertMartini : serveur2-AubertMartini.c serveur2-AubertMartini.h
	${CC} serveur2-AubertMartini.c -o serveur2-AubertMartini ${FLAGS}
	


#Pour nettoyer (enlever tous les fichiers générés
clean :
	\rm -f *.o *~ serveur2 copy_*
