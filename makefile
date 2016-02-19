all: klient serwer stan

klient: klient.c
	gcc -Wall klient.c -o klient

serwer: serwer.c
	gcc -Wall serwer.c -lm -o serwer
stan: stan.c
	gcc -Wall stan.c -o stan
