all: klient serwer

klient: klient.c
	gcc -o klient klient.c

serwer: serwer.c
	gcc -o serwer serwer.c -lm
