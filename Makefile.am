# Process this file with automake to create Makefile.in

bin_PROGRAMS = gtimer

gtimer_SOURCES = \
	annotate.c confirm.c main.c task.h config.c edit.c \
	report.c unhide.c config.h changelog.c changelog.h \
	gtimer.h task.c xextras.c project.h project.c \
	http.c http.h tcpt.c tcpt.h custom-list.c custom-list.h \
	icons/splash.xpm icons/gtimer.xpm icons/gtimer2.xpm \
	icons/clock1.xpm icons/clock2.xpm icons/clock3.xpm \
	icons/clock4.xpm icons/clock5.xpm icons/clock6.xpm \
	icons/clock7.xpm icons/clock8.xpm icons/blank.xpm \
	icons/start.xpm icons/stop.xpm icons/stop_all.xpm \
	icons/annotate.xpm icons/new.xpm icons/edit.xpm

AM_CPPFLAGS = \
	-DLOCALEDIR=\"$(localedir)\" @UIFLAGS@ @CPPFLAGS@ @APP_INDICATOR_CFLAGS@

gtimer_LDADD = @UILIBS@ @APP_INDICATOR_LIBS@

EXTRA_DIST = gtimer.1 icons/gtimer.gif
