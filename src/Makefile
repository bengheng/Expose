SUBDIRS = plugin_enumfuncs \
	plugin_funcs \
	plugin_funcstats \
	plugin_idamnem \
	plugin_instrinfo \
	plugin_jmppatch \
	plugin_makedot \
	plugin_xref \
	distance \
	GraphMatcher

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@


clean:
	for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done	
