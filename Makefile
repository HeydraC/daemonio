all: fileList

fileList: fileList.o txttt.o
	gcc -o fileList fileList.o txttt.o
	rm fileList.o txttt.o

%.o: %.c $(wildcard %.h)
	gcc -c $<
