SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CXX := g++

LDFLAGS :=

# CPPFLAGS := -std=gnu++17 -g -O3 -mavx -I$(SELF_DIR)
CPPFLAGS := -g -I$(SELF_DIR) -std=gnu++17 -Ofast -march=native -mtune=native -mavx -ftree-vectorize
# CPPFLAGS += -frename-registers -funroll-loops -ffast-math -fno-signed-zeros -fno-trapping-math

# CPPFLAGS += -fsanitize=sanitize -g
# LDFLAGS +=  -fsanitize=sanitize

OBJDIR := $(SELF_DIR)/obj
LIBDIR := $(SELF_DIR)/lib

$(OBJDIR)/%.o: $(LIBDIR)/%.cpp
	mkdir -p $(OBJDIR)
	$(CXX) -c $(CPPFLAGS) $< -o $@

$(OBJDIR)/%.d: $(LIBDIR)/%.cpp
	mkdir -p $(OBJDIR)
	$(CXX) $(CPPFLAGS) $< -MM -MT $(@:.d=.o) >$@

lib_src := $(wildcard $(LIBDIR)/*.cpp)
lib_obj := $(lib_src:$(LIBDIR)/%.cpp=$(OBJDIR)/%.o)
lib_dep := $(lib_obj:$(OBJDIR)/%.o=$(OBJDIR)/%.d)

-include $(lib_dep)
