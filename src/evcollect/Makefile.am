## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES =   \
  Makefile.in

# we'll fill those later
bin_PROGRAMS =
noinst_LIBRARIES =
dist_noinst_DATA =
BUILT_SOURCES =
TESTS =
check_PROGRAMS =


################# DEPENDENCIES ####################
if HAVE_ZLIB
ZLIB_DEF = -DHAVE_ZLIB=1
else
ZLIB_DEF =
endif

if HAVE_PTHREAD
PTHREAD_DEF = -DHAVE_PTHREAD=1
else
PTHREAD_DEF =
endif

if HAVE_SYSLOG_H
SYSLOG_DEF = -DHAVE_SYSLOG_H=1
else
SYSLOG_DEF =
endif

if HAVE_GETHOSTBYNAME_R
GETHOSTBYNAME_R_DEF = -DHAVE_GETHOSTBYNAME_R=1
else
GETHOSTBYNAME_R_DEF =
endif

PCRE_LDADD=-lpcre

AM_CXXFLAGS = -DEVCOLLECT_VERSION=\"v@PACKAGE_VERSION@\" $(PTHREAD_CFLAGS) $(PTHREAD_DEF) $(SYSLOG_DEF) $(ZLIB_DEF) $(GETHOSTBYNAME_R_DEF) -std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor -Wno-predefined-identifier-outside-function -Wno-invalid-offsetof -g -I$(top_srcdir)/src -I$(top_builddir)/src
AM_CFLAGS =  $(PTHREAD_CFLAGS) $(PTHREAD_DEF) $(SYSLOG_DEF) $(ZLIB_DEF) $(GETHOSTBYNAME_R_DEF) -std=c11 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -pedantic -g
AM_LDFLAGS = $(PTHREAD_CFLAGS)

################# BIN: evcollectd ####################
evcollectd_SOURCES = \
    util/logging.h \
    util/logging.cc \
    util/flagparser.h \
    util/flagparser.cc \
    util/return_code.h \
    util/stringutil.h \
    util/stringutil_impl.h \
    util/stringutil.cc \
    util/time.h \
    util/time_impl.h \
    util/time.cc \
    util/sha1.h \
    util/sha1.cc \
    plugins/eventql/eventql_plugin.h \
    plugins/eventql/eventql_plugin.cc \
    plugins/hostname/hostname_plugin.h \
    plugins/hostname/hostname_plugin.cc \
    plugins/logfile/logfile_plugin.h \
    config.h \
    config.cc \
    plugins/logfile/logfile_plugin.cc \
    plugins/unix_stats/unix_stats_plugin.h \
    plugins/unix_stats/unix_stats_plugin.cc \
    plugin.h \
    plugin.cc \
    plugin_map.h \
    plugin_map.cc \
    dispatch.h \
    dispatch.cc \
    evcollect.h \
    evcollect.cc \
    evcollectd.cc

evcollectd_LDADD = $(PCRE_LDADD)
bin_PROGRAMS += evcollectd

