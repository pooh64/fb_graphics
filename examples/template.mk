all: $(example)

include ../../general.mk

example_src := $(wildcard *.cpp)
example_obj := $(example_src:.cpp=.o)
example_dep := $(example_obj:.o=.d)

-include $(example_dep)

%.d: %.cpp
	$(CXX) $(CPPFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $@

$(example): $(example_obj) $(lib_obj)
	$(CXX) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(example_obj) $(example_dep) $(example)
