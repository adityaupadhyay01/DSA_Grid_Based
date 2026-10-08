CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude

# -O2 matters for honest measurement. Comparing an unoptimised build against
# an optimised one produces a meaningless result, so the flags are recorded
# here and quoted in the report.

.PHONY: all test bench clean

all: planner tests mission scaling

planner: src/main.cpp include/*.hpp
	$(CXX) $(CXXFLAGS) src/main.cpp -o planner

tests: src/test.cpp include/*.hpp
	$(CXX) $(CXXFLAGS) src/test.cpp -o tests

test: tests
	./tests

bench: planner
	./planner 0.20 10 > results.csv
	@echo "results written to results.csv"

# results.csv is committed evidence, so clean does not touch it.
# Regenerate it deliberately with `make bench`.
clean:
	rm -f planner tests mission scaling

mission: src/mission_demo.cpp include/*.hpp
	$(CXX) $(CXXFLAGS) src/mission_demo.cpp -o mission

scaling: src/scaling.cpp include/*.hpp
	$(CXX) $(CXXFLAGS) src/scaling.cpp -o scaling
