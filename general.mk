SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

OBJDIR := $(SELF_DIR)/obj
LIBDIR := $(SELF_DIR)/lib

$(OBJDIR):
	mkdir -p $(OBJDIR)

CXX = g++
LDFLAGS = -pthread
DEPFLAGS = -MT $@ -MMD -MP -MF

CPPFLAGS = -std=c++17 -I$(SELF_DIR)
# CPPFLAGS += -O0
 CPPFLAGS += -mavx -Ofast -march=native -mtune=native -ftree-vectorize
# CPPFLAGS += -frename-registers -funroll-loops -ffast-math -fno-signed-zeros -fno-trapping-math

#CPPFLAGS += -Wall
CPPFLAGS += -g
#CPPFLAGS += -fsanitize=address
#LDFLAGS  += -fsanitize=address
#CPPFLAGS += -fsanitize=thread
#LDFLAGS  += -fsanitize=thread


$(OBJDIR)/%.o: $(LIBDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CPPFLAGS) $(DEPFLAGS) $(OBJDIR)/$*.d -c $< -o $@

lib_src := $(wildcard $(LIBDIR)/*.cpp)
lib_obj := $(lib_src:$(LIBDIR)/%.cpp=$(OBJDIR)/%.o)
lib_dep := $(lib_obj:$(OBJDIR)/%.o=$(OBJDIR)/%.d)
