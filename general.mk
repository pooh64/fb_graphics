SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CXX := g++
CPPFLAGS := -g -O2 -I$(SELF_DIR)
LDFLAGS :=

OBJDIR := $(SELF_DIR)/obj
LIBDIR := $(SELF_DIR)/lib

$(OBJDIR)/%.o: $(LIBDIR)/%.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $@

$(OBJDIR)/%.d: $(LIBDIR)/%.cpp
	$(CXX) $(CPPFLAGS) $< -MM -MT $(@:.d=.o) >$@

lib_src := $(wildcard $(LIBDIR)/*.cpp)
lib_obj := $(lib_src:$(LIBDIR)/%.cpp=$(OBJDIR)/%.o)
lib_dep := $(lib_obj:$(OBJDIR)/%.o=$(OBJDIR)/%.d)

-include $(lib_dep)
