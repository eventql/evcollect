MAINTAINERCLEANFILES = Makefile.in

AM_CXXFLAGS = -std=c++0x -Wall -Wextra -Wdelete-non-virtual-dtor -g -fvisibility=hidden -I$(top_srcdir)/src
AM_CFLAGS = -std=c11 -Wall -pedantic -g
AM_LDFLAGS = -fvisibility=hidden

lib_LTLIBRARIES = plugin_hostname.la

plugin_hostname_la_LDFLAGS = -module -avoid-version -shared
plugin_hostname_la_SOURCES = \
    hostname_plugin.cc


