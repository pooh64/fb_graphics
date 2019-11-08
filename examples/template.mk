all: $(example)

include ../../general.mk

example_src := $(wildcard *.cpp)
example_obj := $(example_src:.cpp=.o)
example_dep := $(example_obj:.o=.d)

-include $(example_dep)

$(SELF_DIR)/%.d: $(SELF_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $< -MM -MT $(@:.d=.o) >$@

$(SELF_DIR)/%.o: $(SELF_DIR)/%.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $@

$(example): $(lib_obj) $(example_obj)
	$(CXX) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(example_obj) $(example_dep) $(example)

