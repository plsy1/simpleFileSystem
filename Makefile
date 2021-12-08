OUTDIR=./out

fs:main.o shell.o str.o function.o list.o disk.o
	cc -o fs $(OUTDIR)/main.o $(OUTDIR)/shell.o $(OUTDIR)/str.o $(OUTDIR)/function.o $(OUTDIR)/list.o $(OUTDIR)/disk.o

main.o:main.c 
	cc -c main.c
	mv main.o $(OUTDIR)/main.o

shell.o:./shell/shell.c ./shell/shell.h
	cc -c ./shell/shell.c
	mv shell.o $(OUTDIR)/shell.o

str.o:./util/str.c ./util/str.h
	cc -c ./util/str.c
	mv str.o $(OUTDIR)/str.o

function.o:./function/function.c ./function/function.h
	cc -c ./function/function.c
	mv function.o $(OUTDIR)/function.o	

list.o:./util/list.c ./util/list.h
	cc -c ./util/list.c
	mv list.o $(OUTDIR)/list.o

disk.o:./util/disk.c ./util/disk.h
	cc -c ./util/disk.c
	mv disk.o $(OUTDIR)/disk.o

.PHONY:clean
clean:
	-rm $(OUTDIR)/main.o $(OUTDIR)/function.o $(OUTDIR)/disk.o $(OUTDIR)/list.o $(OUTDIR)/shell.o $(OUTDIR)/str.o