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
	@cp $(BUILD_DIR)/lib/libnog_http_runtime.a ~/.local/lib/nog/
	@cp $(BUILD_DIR)/lib/libllhttp.a ~/.local/lib/nog/
	@cp $(BUILD_DIR)/include/nog/std.hpp ~/.local/include/nog/
	@cp $(BUILD_DIR)/include/nog/std.hpp.gch ~/.local/include/nog/
	@cp $(BUILD_DIR)/include/nog/http.hpp ~/.local/include/nog/
	@cp $(BUILD_DIR)/include/nog/http.hpp.gch ~/.local/include/nog/
	@cp $(BUILD_DIR)/include/nog/fs.hpp ~/.local/include/nog/
	@cp $(BUILD_DIR)/include/llhttp.h ~/.local/include/
	@echo "Installed nog to ~/.local/bin/"
	@echo "Installed runtime libraries to ~/.local/lib/nog/"
	@echo "Installed headers to ~/.local/include/"
	@echo "Installed precompiled headers to ~/.local/include/nog/"

docs: build
	@$(BUILD_DIR)/docgen src/ docs/reference/
