all: klient serwer klient_output

klient: klient.c
	gcc -o klient klient.c

serwer: serwer.c
	gcc -o serwer serwer.c -lm
klient_output: klient_output.c
	gcc -o klient_output klient_output.c
