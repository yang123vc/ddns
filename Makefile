# $Id: Makefile,v 1.15 2000/07/14 21:16:48 drt Exp $

DOWNLOADER = "wget"

CFLAGS=-g -Wall -Idnscache -Ilibtai

defaut: client

all: client daemon

daemon: libs ddnsd ddnsd-data ddns-cleand filedns

client: libs ddns-clientd

ddnsd: ddns.h ddnsd.o dnscache.a libtai.a drtlib.a djblib.a
	gcc -o $@ ddnsd.o dnscache.a libtai.a drtlib.a djblib.a

ddns-cleand: ddns.h ddns-cleand.o dnscache.a libtai.a drtlib.a djblib.a
	gcc -o $@ ddns-cleand.o dnscache.a libtai.a drtlib.a djblib.a

ddns-clientd: ddns.h ddns-clientd.o ddnsc.o dnscache.a libtai.a \
drtlib.a djblib.a dnscache/ndelay_off.o
	gcc -o $@ ddns-clientd.o ddnsc.o dnscache.a libtai.a drtlib.a \
	djblib.a dnscache/ndelay_off.o

ddnsd-data: ddnsd-data.o buffer_0.o rijndael.o pad.o txtparse.o dnscache.a libtai.a
	gcc -o $@ ddnsd-data.o buffer_0.o rijndael.o pad.o txtparse.o dnscache.a libtai.a

filedns: ddns.h filedns.o server.o txtparse.o dnscache.a libtai.a drtlib.a djblib.a
	gcc -o $@ filedns.o server.o txtparse.o dnscache.a libtai.a drtlib.a djblib.a

sig_block.o: sig_block.c hassgprm.h

sig_catch.o: sig_catch.c hassgact.h

hassgprm.h: dnscache.a
	make trysgprm
	./dnscache/choose cl trysgprm hassgprm.h1 hassgprm.h2 > hassgprm.h

hassgact.h: dnscache.a
	make trysgact
	./dnscache/choose cl trysgact hassgact.h1 hassgact.h2 > hassgact.h

trysgprm: trysgprm.o

trysgact: trysgact.o

libs: dnscache.a libtai.a drtlib.a djblib.a

dnscache.a:
	if [ ! -d dnscache ]; then \
		$(DOWNLOADER) http://cr.yp.to/dnscache/dnscache-1.00.tar.gz; \
		$(DOWNLOADER) http://www.fefe.de/dns/dnscache-1.00-ipv6.diff5; \
		tar xzvf dnscache-1.00.tar.gz; rm dnscache-1.00.tar.gz; \
		mv dnscache-1.00 dnscache; \
		cd dnscache; patch < ../dnscache-1.00-ipv6.diff5 ; cd ..; \
		rm dnscache-1.00-ipv6.diff5; \
        fi;	
	cd dnscache; \
	make; \
	grep -l ^main *.c | perl -npe 's/^(.*).c/\1.o/;' | xargs rm -fv; \
	ar cr ../dnscache.a *.o;

libtai.a:
	if [ ! -d libtai ]; then \
		$(DOWNLOADER) http://cr.yp.to/libtai/libtai-0.60.tar.gz; \
		tar xzvf libtai-0.60.tar.gz; rm libtai-0.60.tar.gz; \
		mv libtai-0.60 libtai;\
	fi	
	cd libtai; \
	make; \
	grep -l ^main *.c | perl -npe 's/^(.*).c/\1.o/;' | xargs rm -fv; \
	ar cr ../libtai.a *.o; 	

djblib.a: buffer_0.o fd.h fd_copy.o fd_move.o fmt_xint.o fmt_xlong.o \
now.o open_excl.o scan_xlong.o sig.o sig_alarm.o sig_block.o sig_catch.o \
sig_int.o sig_term.o timeoutconn.o
	ar cr djblib.a buffer_0.o fd_copy.o fd_move.o fmt_xint.o \
	fmt_xlong.o now.o open_excl.o scan_xlong.o sig_alarm.o sig_block.o \
	sig_catch.o timeoutconn.o sig.o sig_int.o sig_term.o

drtlib.a: iso2txt.o loc.o mt19937.o pad.o rijndael.o txtparse.o
	ar cr drtlib.a iso2txt.o loc.o mt19937.o pad.o rijndael.o txtparse.o

clean:
	rm -f *.o ddnsd ddnsd-data ddns-clientd filedns hassgprm.h hassgact.h *.a

distclean: clean
	rm -f *~ *.cdb core
	rm -Rf dnscache libtai 
