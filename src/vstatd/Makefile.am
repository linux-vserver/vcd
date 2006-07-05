INCLUDES = -I$(top_srcdir)/lib/lucid

LIBS = $(top_builddir)/lib/lucid/libucid.a \
       -lvserver

noinst_HEADERS = cfg.h \
                 collector.h \
                 log.h \
		 vs_rrd.h \
		 vstats.h

sbin_PROGRAMS = vstatd

vstatd_SOURCES = cfg.c \
	         collector.c \
		 log.c \
		 main.c \
		 parser.c \
		 vs_rrd.c

vstatd_LDADD = -lconfuse \
	       -lvserver \
	       -lrrd
