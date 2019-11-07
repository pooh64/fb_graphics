SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CXX := g++

LDFLAGS := -pthread

CPPFLAGS := -std=c++17 -I$(SELF_DIR)
# CPPFLAGS += -Wall
CPPFLAGS += -g
CPPFLAGS += -mavx -Ofast -march=native -mtune=native -ftree-vectorize
# CPPFLAGS += -frename-registers -funroll-loops -ffast-math -fno-signed-zeros -fno-trapping-math

# CPPFLAGS += -fsanitize=address
# LDFLAGS +=  -fsanitize=address

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
