plugin_LTLIBRARIES = \
	libinfinoted-plugin-example.la

plugindir = ${libdir}/infinoted-0.6/plugins

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	$(infinoted_plugin_example_CFLAGS)

AM_LDFLAGS = \
	-avoid-version -module -no-undefined

libinfinoted_plugin_example_la_LIBADD = \
	$(infinoted_plugin_example_LIBS)

libinfinoted_plugin_example_la_SOURCES = \
        infinoted-plugin-example.c
