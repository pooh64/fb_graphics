all: $(example)

include ../../general.mk

example_src := $(wildcard *.c)
example_obj := $(example_src:.c=.o)
example_dep := $(example_obj:.o=.d)

-include $(example_dep)

%.d: %.c
	$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(example): $(example_obj) $(lib_obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(example_obj) $(example_dep) $(example)
