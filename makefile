include general.mk

all: $(lib_obj)

.PHONY: clean
clean:
	rm -f $(lib_obj)
	rm -f $(lib_dep)
