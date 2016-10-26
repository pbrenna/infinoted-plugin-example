plugin_LTLIBRARIES = \
	libinfinoted-plugin-replacer.la

plugindir = ${libdir}/infinoted-0.6/plugins

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	$(infinoted_plugin_replacer_CFLAGS)

AM_LDFLAGS = \
	-avoid-version -module -no-undefined

libinfinoted_plugin_replacer_la_LIBADD = \
	$(infinoted_plugin_replacer_LIBS)

libinfinoted_plugin_replacer_la_SOURCES = \
        infinoted-plugin-replacer.c
