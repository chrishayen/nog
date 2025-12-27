.PHONY: all build clean test run configure rebuild install docs

BUILD_DIR := build

all: build

configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..

build: configure
	@cmake --build $(BUILD_DIR) --parallel

clean:
	@rm -rf $(BUILD_DIR)

rebuild: clean build

test: build
	@$(BUILD_DIR)/bishop test tests/

run: build
	@$(BUILD_DIR)/bishop $(ARGS)

install: build
	@mkdir -p ~/.local/bin
	@mkdir -p ~/.local/lib/bishop
	@mkdir -p ~/.local/include/bishop
	@mkdir -p ~/.local/include/bishop/fiber_asio
	@cp $(BUILD_DIR)/bishop ~/.local/bin/
	@cp $(BUILD_DIR)/lib/libbishop_std_runtime.a ~/.local/lib/bishop/
	@cp $(BUILD_DIR)/lib/libbishop_http_runtime.a ~/.local/lib/bishop/
	@cp $(BUILD_DIR)/lib/libllhttp.a ~/.local/lib/bishop/
	@cp $(BUILD_DIR)/include/bishop/std.hpp ~/.local/include/bishop/
	@cp $(BUILD_DIR)/include/bishop/std.hpp.gch ~/.local/include/bishop/
	@cp $(BUILD_DIR)/include/bishop/http.hpp ~/.local/include/bishop/
	@cp $(BUILD_DIR)/include/bishop/http.hpp.gch ~/.local/include/bishop/
	@cp $(BUILD_DIR)/include/bishop/fs.hpp ~/.local/include/bishop/
	@cp $(BUILD_DIR)/include/bishop/fiber_asio/*.hpp ~/.local/include/bishop/fiber_asio/
	@cp $(BUILD_DIR)/include/llhttp.h ~/.local/include/
	@echo "Installed bishop to ~/.local/bin/"
	@echo "Installed runtime libraries to ~/.local/lib/bishop/"
	@echo "Installed headers to ~/.local/include/"
	@echo "Installed precompiled headers to ~/.local/include/bishop/"

docs: build
	@$(BUILD_DIR)/docgen src/ docs/reference/
