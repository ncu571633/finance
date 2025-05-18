CXX=g++
CXXFLAGS:=-Wall -Werror -Wextra -Og -std=c++23

SOURCES=$(wildcard *.cpp)
OBJS=$(SOURCES:%.cpp=out/%.o)
TESTS=$(wildcard examples/*.txt)
TESTS_EXPECTED=$(TESTS:%.txt=%.expected)
TESTS_RESULT=$(TESTS:examples/%.txt=out/%.result)

.PHONY: build
build: out/exchange

.PHONY: run
run: build
	out/exchange

.PHONY: test
test: $(TESTS_RESULT)
	@bash -c 'for x in $(TESTS_EXPECTED); do echo $$x...; diff -sy $$x out/$$(basename $${x%.expected}.result); done'

.PHONY: print
print:
	@echo SOURCES=$(SOURCES)
	@echo OBJS=$(OBJS)

out:
	mkdir -p out

out/exchange: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

out/%.o: %.cpp | out
	$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<

out/%.result: examples/%.txt out/exchange
	out/exchange < $^ | tee $@

.PHONY: clean
clean:
	rm -rf out

-include $(wildcard out/*.d)
