MAINTAINERCLEANFILES = Makefile.in

AM_CXXFLAGS = -std=c++0x -Wall -Wextra -Wdelete-non-virtual-dtor-g -fvisibility=hidden -I$(top_srcdir)/src
AM_CFLAGS = -std=c11 -Wall -pedantic -g
AM_LDFLAGS = -fvisibility=hidden

lib_LTLIBRARIES = hostname.la

hostname_la_LDFLAGS = -module -avoid-version -shared
hostname_la_SOURCES = \
    hostname_plugin.cc


