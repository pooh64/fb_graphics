SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CC := gcc
CFLAGS := -g -O2 -I$(SELF_DIR)
LDFLAGS :=

OBJDIR := $(SELF_DIR)/obj
LIBDIR := $(SELF_DIR)/lib

$(OBJDIR)/%.o: $(LIBDIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(LIBDIR)/%.c
	$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

lib_src := $(wildcard $(LIBDIR)/*.c)
lib_obj := $(lib_src:$(LIBDIR)/%.c=$(OBJDIR)/%.o)
lib_dep := $(lib_obj:$(OBJDIR)/%.o=$(OBJDIR)/%.d)

-include $(lib_dep)
