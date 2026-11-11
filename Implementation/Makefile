# One command builds the backend. The frontend needs Node, but only to build:
# nothing in the running system is JavaScript except the browser code.
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iengine/include -Iserver/include
LDFLAGS  := -pthread
BIN      := bin

# -O2 matters for honest measurement. Comparing an unoptimised build against an
# optimised one produces a meaningless result, so the flag is fixed here.

.PHONY: all server engine test test-api bench web dev serve clean

all: $(BIN)/server $(BIN)/tests $(BIN)/json_test $(BIN)/planner $(BIN)/mission $(BIN)/scaling

$(BIN):
	@mkdir -p $(BIN)

$(BIN)/server: server/src/main.cpp server/include/*.hpp engine/include/*.hpp | $(BIN)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(BIN)/planner: engine/src/cli/main.cpp engine/include/*.hpp | $(BIN)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN)/tests: engine/src/cli/test.cpp engine/include/*.hpp | $(BIN)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN)/json_test: server/src/json_test.cpp server/include/json.hpp | $(BIN)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN)/mission: engine/src/cli/mission_demo.cpp engine/include/*.hpp | $(BIN)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN)/scaling: engine/src/cli/scaling.cpp engine/include/*.hpp | $(BIN)
	$(CXX) $(CXXFLAGS) $< -o $@

server: $(BIN)/server
engine: $(BIN)/tests $(BIN)/planner $(BIN)/mission $(BIN)/scaling

# The engine suite and the JSON reader suite. Neither needs Node or a network.
test: $(BIN)/tests $(BIN)/json_test
	./$(BIN)/tests
	./$(BIN)/json_test

# Starts the server, exercises every endpoint, stops it.
test-api: $(BIN)/server
	node scripts/api-smoke.mjs

bench: $(BIN)/planner
	./$(BIN)/planner 0.20 10 > engine/results.csv
	@echo "results written to engine/results.csv"

web:
	cd web && npm install && npm run build

# Production: one process, one port, API and frontend together.
serve: $(BIN)/server
	./$(BIN)/server --web web/dist

# Development: Vite on :5173 proxying to the C++ server on :8080.
dev: $(BIN)/server
	@echo "run in two terminals:"
	@echo "  ./bin/server"
	@echo "  cd web && npm run dev"

# results.csv is committed benchmark evidence, not a build artifact.
clean:
	rm -rf $(BIN)
