HFILES=util.h table.h mitm.h lfsr.h
CFILES=main.c util.c table.c mitm.c lfsr.c
EXEC=attaque

compile : ${CFILES} ${HFILES}
	gcc -o ${EXEC} -O3 -Wall ${CFILES} -lpthread

clean :
	rm ${EXEC}