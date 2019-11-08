include general.mk

.PHONY: clean
clean:
	rm -f $(lib_obj)
	rm -f $(lib_dep)

.DEFAULT_GOAL := all
all: $(lib_obj)

-include $(lib_dep)
