
all: librm.a  app

librm.a:  rm.c
	gcc -Wall -c rm.c
	ar -cvq librm.a rm.o
	ranlib librm.a

app: app.c
	gcc -Wall -o app app.c -L. -lrm -lpthread

clean: 
	rm -fr *.o *.a *~ a.out  app rm.o rm.a librm.a
