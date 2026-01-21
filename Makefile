.PHONY: all build test clean format lint

BUILD_DIR := build

all: build

build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. && make

test: build
	@cd $(BUILD_DIR) && make test-all

clean:
	@rm -rf $(BUILD_DIR)

format:
	@find src include tests -name '*.c' -o -name '*.h' | xargs clang-format -i

lint:
	@cppcheck --error-exitcode=1 --enable=all --suppress=missingIncludeSystem \
		--suppress=unmatchedSuppression --suppress=unusedFunction \
		--std=c99 -Iinclude src/ tests/

bench: build
	@./$(BUILD_DIR)/certifiable-bench --help

install-hooks:
	@pip install pre-commit
	@pre-commit install
