hw6:
	g++ -c -g -Wall TableCheck.c
	g++ -c -g -Wall ModProd.c
	g++ -c -g -Wall KeyExpand.c
	g++ -c -g -Wall AES.c
	g++ -c -g -Wall Inverse.c
	g++ -c -g -Wall main.c
	g++ -g -o hw6 -Wall main.o Inverse.o AES.o KeyExpand.o ModProd.o TableCheck.o
clean:
	rm -f hw6
	rm -f main.o
	rm -f Inverse.o
	rm -f AES.o
	rm -f KeyExpand.o
	rm -f ModProd.o
	rm -f TableCheck.o
