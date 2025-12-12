.PHONY: all build clean test run configure rebuild install docs

BUILD_DIR := build

all: build

configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..

build: configure
	@cmake --build $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)

rebuild: clean build

test: build
	@$(BUILD_DIR)/nog test tests/

run: build
	@$(BUILD_DIR)/nog $(ARGS)

install: build
	@mkdir -p ~/.local/bin
	@mkdir -p ~/.local/lib/nog
	@mkdir -p ~/.local/include/nog
	@cp $(BUILD_DIR)/nog ~/.local/bin/
	@rm -f ~/.local/lib/nog/libnog_http.a
	@cp $(BUILD_DIR)/lib/libllhttp.a ~/.local/lib/nog/
	@cp $(BUILD_DIR)/include/nog/http.hpp ~/.local/include/nog/
	@cp $(BUILD_DIR)/include/llhttp.h ~/.local/include/
	@rm -f ~/.local/include/nog/llhttp.h
	@echo "Installed nog to ~/.local/bin/"
	@echo "Installed runtime libraries to ~/.local/lib/nog/"
	@echo "Installed headers to ~/.local/include/"

docs: build
	@$(BUILD_DIR)/docgen src/ docs/reference/
