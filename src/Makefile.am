AM_CFLAGS = $(GCC_CFLAG) @CFLAGS@
AM_YFLAGS = -d
LEXLIB = @LEXLIB@
LDADD = libcfengine.la $(BERKELEY_DB_LDFLAGSD) $(BERKELEY_DB_LIB) $(OPENSSL_LIB_LDFLAGS) $(OPENSSL_LIB_LIB)
INCLUDES = $(BERKELEY_DB_CFLAGS) $(OPENSSL_LIB_CPPFLAGS)
AM_LDFLAGS = $(BERKELEY_DB_LDFLAGS) $(OPENSSL_LIB_LDFLAGS) @LDFLAGS@

sbin_PROGRAMS = cfconvert

#
# Fix this generically
#

lib_LTLIBRARIES = libcfengine.la

libcfengine_la_LDFLAGS = -version-info 1:0:0 -no-undefined
libcfengine_la_LIBADD = $(BERKELEY_DB_LDFLAGSD) $(BERKELEY_DB_LIB) $(OPENSSL_LIB_LDFLAGS) $(OPENSSL_LIB_LIB)
libcfengine_la_CFLAGS = $(AM_CFLAGS)

# Separate out a library

libcfengine_la_SOURCES =							\
	init.c								\
	modules.c							\
	alerts.c							\
        report.c                                                        \
	client.c							\
	ip.c								\
	ipname.c							\
	instrument.c							\
	process.c							\
	timeout.c							\
	ifconf.c							\
	image.c								\
	item.c								\
	item-ext.c							\
	item-file.c							\
	2Dlist.c							\
	globals.c							\
	classes.c							\
	misc.c								\
	parse.c								\
	functions.c							\
	edittools.c							\
	patches.c							\
	install.c							\
	link.c								\
	tidy.c								\
	filedir.c							\
	eval.c								\
	modes.c								\
	sockaddr.c							\
	chflags.c							\
	locks.c								\
	mount.c								\
	macro.c								\
	filenames.c							\
	varstring.c							\
	wildcard.c							\
	cfparse.y							\
	comparray.c							\
	read.c								\
	checksums.c							\
	proto.c								\
        filters.c                                                       \
	copy.c								\
	repository.c							\
	rotate.c							\
	errors.c							\
	cflex.l								\
	net.c								\
	df.c								\
	log.c								\
	crypto.c							\
	popen.c								\
	popen_def.c							\
	sensible.c							\
	acl.c								\
	dce_acl.c							\
	nameinfo.c							\
	strategies.c                         				\
	granules.c							\
	methods.c							\
	dtypes.c							\
	state.c								\
	checksum_db.c							\
	expand-files.c							\
	expand-tidy.c							\
	expand-image.c							\
	expand-editfiles.c						\
	package.c							\
	scli.c

#libcfengine_la_LIBADD = -L../pub -lcfpub

cfconvert_SOURCES =							\
	convert.c							\
        wrapper.c                                                       \
        do.c

#
# Pickup header files here so Automake knows about them
#
noinst_HEADERS =							\
	cfservd.h							\
	cfparse.h							\
	cf.defs.h							\
	cf.extern.h                                                     \
	prototypes.h							\
	cfetooldefs.h

#
# Some basic clean ups
#
MOSTLYCLEANFILES = *~ *.orig *.rej

#
# Get everything removed down to where rebuilding requires:
# "make; make install"
# 
CLEANFILES = cflex.c cfparse.c cfparse.h

#
# Get everything removed down to where rebuilding requires:
# "configure; make; make install"
#
DISTCLEANFILES = cflex.c cfparse.c cfparse.h

#
# Get everything removed down to where rebuilding requires:
# "aclocal; autoconf; autoheader; automake --add-missing"
# "configure; make; make install"
#
MAINTAINERCLEANFILES = Makefile.in conf.h.in stamp-h.in
