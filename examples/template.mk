include ../../general.mk

.DEFAULT_GOAL := all
all: $(example)

example_src := $(wildcard *.cpp)
example_obj := $(example_src:.cpp=.o)
example_dep := $(example_obj:.o=.d)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(DEPFLAGS) $*.d -c $< -o $@

$(example): $(lib_obj) $(example_obj)
	$(CXX) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(example_obj) $(example_dep) $(example)

-include $(lib_dep)
-include $(example_dep)
