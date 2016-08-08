MAINTAINERCLEANFILES = Makefile.in

AM_CXXFLAGS = -std=c++0x -Wall -Wextra -Wdelete-non-virtual-dtor -g -fvisibility=hidden -I$(top_srcdir)/src
AM_CFLAGS = -std=c11 -Wall -pedantic -g
AM_LDFLAGS = -fvisibility=hidden -module -avoid-version -shared -export-dynamic -rpath $(libdir)

noinst_LTLIBRARIES = plugin_unix_stats.la

plugin_unix_stats_la_SOURCES = \
    unix_stats_plugin.cc

PLUGINDIR=$(DESTDIR)$(libdir)/evcollect/plugins

install-data-hook: $(noinst_LTLIBRARIES)
	@for soname in `echo | $(EGREP) "^dlname=" $^ | $(SED) -e "s|^dlname='\(.*\)'|\1|"`; do  \
		mkdir -p ${PLUGINDIR};                                                                 \
		echo Installing $$soname to ${PLUGINDIR};                                              \
		cp $(abs_builddir)/.libs/$$soname ${PLUGINDIR};                                        \
	done

