all: klient serwer

klient: klient.c
	gcc -o klient klient.c

serwer: serwer.c
	g++ -o serwer serwer.c
