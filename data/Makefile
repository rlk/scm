# This Makefile does nothing on its own and expects to be called recursively.

%-vert.h : %.vert
	xxd -i $^ > $@

%-frag.h : %.frag
	xxd -i $^ > $@
