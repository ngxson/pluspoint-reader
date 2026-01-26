CC ?= cc
CXX ?= c++
CFLAGS ?= -O2 -I.
CXXFLAGS ?= -O2 -std=c++17 -I.

BUILD_DIR ?= build
JS_COMPILE ?= $(BUILD_DIR)/js-compile

JS_ENGINE_C := src/app/JsEngine.c
JS_COMPILE_C := src/app/JsCompile.c
JS_COMPILE_CPP := js-compile.cpp

.PHONY: all clean js-compile help

all: js-compile

help:
	@echo "Targets:"
	@echo "  js-compile  Build js-compile CLI"
	@echo "  clean       Remove build artifacts"

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/JsEngine.o: $(JS_ENGINE_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(JS_ENGINE_C) -o $@

$(BUILD_DIR)/JsCompile.o: $(JS_COMPILE_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(JS_COMPILE_C) -o $@

$(JS_COMPILE): $(JS_COMPILE_CPP) $(BUILD_DIR)/JsEngine.o $(BUILD_DIR)/JsCompile.o | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(JS_COMPILE_CPP) $(BUILD_DIR)/JsEngine.o $(BUILD_DIR)/JsCompile.o -o $@

js-compile: $(JS_COMPILE)

clean:
	@rm -rf $(BUILD_DIR)
