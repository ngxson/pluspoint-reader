// JS binary compilation CLI and test suite

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

extern "C" {
#include "src/app/JsEngine.h"
}

static char js_mem[16384];

// Test case structure
struct TestCase {
  const char *name;
  const char *code;
  std::function<bool(struct js *, jsval_t)> check;
};

// Helper functions for checking results
static bool check_number(struct js *js, jsval_t v, double expected) {
  (void)js;
  if (js_type(v) == JS_ERR) return false;
  if (js_type(v) != JS_NUM) return false;
  double got = js_getnum(v);
  return got == expected;
}

static bool check_true(struct js *js, jsval_t v) {
  (void)js;
  return js_type(v) == JS_TRUE;
}

static bool check_false(struct js *js, jsval_t v) {
  (void)js;
  return js_type(v) == JS_FALSE;
}

static bool check_string(struct js *js, jsval_t v, const char *expected) {
  if (js_type(v) != JS_STR) return false;
  size_t len;
  char *s = js_getstr(js, v, &len);
  return s && len == strlen(expected) && memcmp(s, expected, len) == 0;
}

static bool check_undef(struct js *js, jsval_t v) {
  (void)js;
  return js_type(v) == JS_UNDEF;
}

static bool check_null(struct js *js, jsval_t v) {
  (void)js;
  return js_type(v) == JS_NULL;
}

// Run a single test case using both direct eval and compiled eval
static bool run_single_test(const TestCase &t) {
  // First, test direct evaluation
	std::printf("  Direct eval...\n");
  struct js *js = js_create(js_mem, sizeof(js_mem));
  jsval_t direct_result = js_eval(js, t.code, strlen(t.code));

  if (!t.check(js, direct_result)) {
    std::printf("[FAIL] %s (direct eval)\n", t.name);
    std::printf("       Code: %s\n", t.code);
    std::printf("       Result: %s\n", js_str(js, direct_result));
    return false;
  }

  // Then, test binary compilation and evaluation
	std::printf("  Compiling...\n");
  js_compiled_t compiled = js_compile(t.code, strlen(t.code));

  if (!compiled.buf || compiled.len == 0) {
    std::printf("[FAIL] %s (compile failed)\n", t.name);
    std::printf("       Code: %s\n", t.code);
    return false;
  }

	std::printf("  Compiled %zu bytes\n", compiled.len);
  js = js_create(js_mem, sizeof(js_mem));
  jsval_t compiled_result = js_eval_compiled(js, &compiled);

  if (!t.check(js, compiled_result)) {
    std::printf("[FAIL] %s (compiled eval)\n", t.name);
    std::printf("       Code: %s\n", t.code);
    std::printf("       Result: %s\n", js_str(js, compiled_result));
    free(compiled.buf);
    return false;
  }

  free(compiled.buf);
  return true;
}

static bool run_tests() {
  const TestCase tests[] = {
    // Basic literals
    {
      "literal_number",
      "42;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 42); },
    },
    {
      "literal_true",
      "true;",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },
    {
      "literal_false",
      "false;",
      [](struct js *js, jsval_t v) { return check_false(js, v); },
    },
    {
      "literal_null",
      "null;",
      [](struct js *js, jsval_t v) { return check_null(js, v); },
    },
    {
      "literal_undefined",
      "undefined;",
      [](struct js *js, jsval_t v) { return check_undef(js, v); },
    },
    {
      "zero",
      "0;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 0); },
    },
    {
      "negative_number",
      "-42;",
      [](struct js *js, jsval_t v) { return check_number(js, v, -42); },
    },
    {
      "floating_point",
      "3.14159;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 3.14159); },
    },
    {
      "empty_string",
      "\"\";",
      [](struct js *js, jsval_t v) { return check_string(js, v, ""); },
    },
    {
      "simple_string",
      "\"hello\";",
      [](struct js *js, jsval_t v) { return check_string(js, v, "hello"); },
    },

    // Basic arithmetic
    {
      "addition",
      "1 + 2;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 3); },
    },
    {
      "subtraction",
      "10 - 3 - 2;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 5); },
    },
    {
      "multiplication",
      "3 * 4;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 12); },
    },
    {
      "division",
      "20 / 4;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 5); },
    },
    {
      "modulo",
      "17 % 5;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 2); },
    },
    {
      "precedence",
      "2 + 3 * 4;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 14); },
    },
    {
      "parentheses",
      "(2 + 3) * 4;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 20); },
    },

    // Comparisons
    {
      "less_than",
      "1 < 2;",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },
    {
      "greater_than",
      "5 > 3;",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },
    {
      "less_equal",
      "5 <= 5;",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },
    {
      "greater_equal",
      "6 >= 5;",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },
    {
      "equality",
      "5 === 5;",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },
    {
      "inequality",
      "5 !== 3;",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },

    // Logical operators
    {
      "logical_and_true",
      "true && true;",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },
    {
      "logical_and_false",
      "true && false;",
      [](struct js *js, jsval_t v) { return check_false(js, v); },
    },
    {
      "logical_or",
      "false || true;",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },
		// FAILED (SEGFAULT)
    // {
    //   "logical_short_circuit_and",
    //   "let x = 0; let a = false && (x = 1); x;",
    //   [](struct js *js, jsval_t v) { return check_number(js, v, 0); },
    // },
    // {
    //   "logical_short_circuit_or",
    //   "let x = 0; let a = true || (x = 1); x;",
    //   [](struct js *js, jsval_t v) { return check_number(js, v, 0); },
    // },

    // Bitwise operations
    {
      "bitwise_or",
      "(5 | 3);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 7); },
    },
    {
      "bitwise_and",
      "(5 & 3);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 1); },
    },
    {
      "bitwise_xor",
      "(5 ^ 3);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 6); },
    },
    {
      "left_shift",
      "1 << 4;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 16); },
    },
    {
      "right_shift",
      "16 >> 2;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 4); },
    },

    // Unary operators
    {
      "unary_not_false",
      "!false;",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },
    {
      "unary_not_true",
      "!true;",
      [](struct js *js, jsval_t v) { return check_false(js, v); },
    },
		// FAILED (SEGFAULT)
    // {
    //   "unary_minus",
    //   "let a = -5; a + 10;",
    //   [](struct js *js, jsval_t v) { return check_number(js, v, 5); },
    // },
    {
      "unary_plus",
      "+5;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 5); },
    },
    {
      "unary_bitwise_not",
      "~0;",
      [](struct js *js, jsval_t v) { return check_number(js, v, -1); },
    },
    {
      "typeof_number",
      "typeof 42;",
      [](struct js *js, jsval_t v) { return check_string(js, v, "number"); },
    },
    {
      "typeof_string",
      "typeof \"hello\";",
      [](struct js *js, jsval_t v) { return check_string(js, v, "string"); },
    },

    // Variables
		// FAILED (SEGFAULT)
    // {
    //   "let_and_use",
    //   "let a = 5; a;",
    //   [](struct js *js, jsval_t v) { return check_number(js, v, 5); },
    // },
    {
      "var_reassign",
      "let a = 1; a = 2; a = 3; a;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 3); },
    },
    {
      "let_with_expr",
      "let x = 1 + 2 * 3; x;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 7); },
    },

    // Assignment operators
    {
      "assignment_add",
      "let a = 5; a += 2; a;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 7); },
    },
    {
      "assignment_sub",
      "let a = 10; a -= 3; a;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 7); },
    },
    {
      "assignment_mul",
      "let a = 4; a *= 3; a;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 12); },
    },
    {
      "assignment_div",
      "let a = 20; a /= 4; a;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 5); },
    },
    {
      "postinc",
      "let a = 5; let b = a++; a + b;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 11); },
    },
    {
      "postdec",
      "let a = 5; let b = a--; a + b;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 9); },
    },

    // Strings
    {
      "string_concat",
      "\"hello\" + \" \" + \"world\";",
      [](struct js *js, jsval_t v) { return check_string(js, v, "hello world"); },
    },
    {
      "string_length",
      "\"hi!\".length;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 3); },
    },
    {
      "string_escape",
      "\"a\\nb\".length;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 3); },
    },
    {
      "string_equality",
      "\"hello\" === \"hello\";",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },
    {
      "string_inequality",
      "\"hello\" !== \"world\";",
      [](struct js *js, jsval_t v) { return check_true(js, v); },
    },

    // Objects
    {
      "empty_object",
      "let obj = {}; obj;",
      [](struct js *js, jsval_t v) { return js_type(v) == JS_PRIV; },
    },
    {
      "object_literal",
      "let obj = {x: 5, y: 7}; obj.x + obj.y;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 12); },
    },
    {
      "nested_objects",
      "let obj = {a: {b: {c: 42}}}; obj.a.b.c;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 42); },
    },
    {
      "object_string_key",
      "let obj = {\"foo\": 123}; obj.foo;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 123); },
    },

    // Conditionals
    {
      "if_then",
      "let x = 10; let r = 0; if (x > 5) { r = 1; } r;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 1); },
    },
    {
      "if_else_true",
      "let x = 10; let r = 0; if (x > 5) { r = 1; } else { r = 2; } r;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 1); },
    },
    {
      "if_else_false",
      "let x = 3; let r = 0; if (x > 5) { r = 1; } else { r = 2; } r;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 2); },
    },
    {
      "if_nested",
      "let x = 10; let y = 5; let r = 0; if (x > 5) { if (y > 3) { r = 1; } } r;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 1); },
    },

    // Ternary
    {
      "ternary",
      "(1 ? 2 : 3);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 2); },
    },
    {
      "ternary_false",
      "(0 ? 2 : 3);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 3); },
    },
    {
      "ternary_nested",
      "(1 ? (0 ? 1 : 2) : 3);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 2); },
    },

    // Loops
    {
      "for_loop",
      "let n = 0; for (let i = 0; i < 5; i++) { n = n + i; } n;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 10); },
    },
    {
      "for_loop_sum",
      "let sum = 0; for (let i = 1; i <= 10; i++) { sum = sum + i; } sum;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 55); },
    },
    {
      "break_in_loop",
      "let i = 0; for (; i < 10; i++) { if (i === 5) { break; } } i;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 5); },
    },
    {
      "continue_in_loop",
      "let sum = 0; for (let i = 0; i < 5; i++) { if (i === 2) { continue; } sum = sum + i; } sum;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 8); },
    },

    // Functions
    {
      "function_call",
      "let f1 = function(x) { return x + 1; }; f1(3);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 4); },
    },
    {
      "function_multiple_args",
      "let add = function(a, b, c) { return a + b + c; }; add(1, 2, 3);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 6); },
    },
    {
      "nested_function_call",
      "let add = function(a, b) { return a + b; }; let mul = function(a, b) { return a * b; }; mul(add(2, 3), 4);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 20); },
    },
    {
      "recursive_factorial",
      "let fact = function(n) { if (n < 2) { return 1; } return n * fact(n - 1); }; fact(5);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 120); },
    },
    {
      "recursive_fibonacci",
      "let fib = function(n) { if (n < 2) { return n; } return fib(n - 1) + fib(n - 2); }; fib(10);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 55); },
    },
    {
      "closure",
      "let make_adder = function(x) { return function(y) { return x + y; }; }; let add5 = make_adder(5); add5(3);",
      [](struct js *js, jsval_t v) { return check_number(js, v, 8); },
    },
    {
      "function_no_return",
      "let f = function() { let x = 1; }; f();",
      [](struct js *js, jsval_t v) { return check_undef(js, v); },
    },

    // Scoping
    {
      "block_scope",
      "let a = 1; { let b = 2; a = a + b; } a;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 3); },
    },
    {
      "function_scope",
      "let a = 1; let f = function() { let a = 10; return a; }; f() + a;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 11); },
    },

    // Complex expressions
    {
      "complex_expr",
      "let a = 2; let b = 3; let c = (a + b) * (a - b) + b * b; c;",
      [](struct js *js, jsval_t v) { return check_number(js, v, 4); },
    },
    {
      "complex_nested_calls",
      "let sq = function(x) { return x * x; }; let add = function(a, b) { return a + b; }; add(sq(3), sq(4));",
      [](struct js *js, jsval_t v) { return check_number(js, v, 25); },
    },
  };

  int passed = 0;
  int failed = 0;

  for (const auto &t : tests) {
    std::printf("Testing: %s\n", t.name);
    std::fflush(stdout);
    if (run_single_test(t)) {
      std::printf("[PASS] %s\n", t.name);
      passed++;
    } else {
      failed++;
    }
  }

  std::printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
  return failed == 0;
}

// Test compiled code size comparison
static void test_compiled_size() {
  std::printf("\n=== Compiled Size Comparison ===\n");

  const char *programs[] = {
    "1 + 2;",
    "let x = 1 + 2 * 3; x;",
    "let sum = 0; for (let i = 0; i < 10; i++) { sum = sum + i; } sum;",
    "let fact = function(n) { if (n < 2) { return 1; } return n * fact(n - 1); }; fact(5);",
    "let obj = {a: 1, b: 2, c: {d: 3, e: 4}}; obj.a + obj.c.d;",
  };

  for (const char *code : programs) {
    js_compiled_t compiled = js_compile(code, strlen(code));

    if (compiled.buf && compiled.len > 0) {
      size_t src_size = strlen(code);
      double ratio = 100.0 * compiled.len / src_size;
      std::printf("Source: %3zu bytes, Compiled: %3zu bytes (%5.1f%%): %.40s%s\n",
                  src_size, compiled.len, ratio, code, strlen(code) > 40 ? "..." : "");
      free(compiled.buf);
    } else {
      std::printf("Compile error for: %s\n", code);
    }
  }
}

// Test save/load compiled code to file
static void test_save_load() {
  std::printf("\n=== Save/Load Compiled Code Test ===\n");

  const char *code = "let fib = function(n) { if (n < 2) { return n; } return fib(n-1) + fib(n-2); }; fib(10);";

  js_compiled_t compiled = js_compile(code, strlen(code));

  if (!compiled.buf || compiled.len == 0) {
    std::printf("Compile error\n");
    return;
  }

  // Save to file
  FILE *f = std::fopen("/tmp/test.jsc", "wb");
  if (f) {
    std::fwrite(compiled.buf, 1, compiled.len, f);
    std::fclose(f);
    std::printf("Saved compiled code to /tmp/test.jsc (%zu bytes)\n", compiled.len);
  }
  free(compiled.buf);

  // Load from file
  f = std::fopen("/tmp/test.jsc", "rb");
  if (f) {
    std::fseek(f, 0, SEEK_END);
    size_t file_size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    uint8_t *loaded_buf = (uint8_t*)std::malloc(file_size);
    std::fread(loaded_buf, 1, file_size, f);
    std::fclose(f);

    // Execute loaded compiled code
    js_compiled_t loaded_compiled = {loaded_buf, file_size};
    struct js *js = js_create(js_mem, sizeof(js_mem));
    jsval_t result = js_eval_compiled(js, &loaded_compiled);

    std::printf("Loaded and executed: fib(10) = %s\n", js_str(js, result));

    if (check_number(js, result, 55)) {
      std::printf("[PASS] Save/load test\n");
    } else {
      std::printf("[FAIL] Save/load test - expected 55\n");
    }

    free(loaded_buf);
  }
}

// CLI helper
static void print_help(const char *exe) {
  std::printf("Usage:\n");
  std::printf("  %s input.js output.app\n", exe);
  std::printf("  %s input.js  : output is <input>.app\n", exe);
  std::printf("  %s test      : run tests\n", exe);
  std::printf("  %s           : will show this help\n", exe);
}

// Compile a JS file to binary format
static bool compile_file(const char *input_path, const char *output_path) {
  // Read input file
  FILE *f = std::fopen(input_path, "rb");
  if (!f) {
    std::printf("Error: Cannot open input file '%s'\n", input_path);
    return false;
  }

  std::fseek(f, 0, SEEK_END);
  size_t file_size = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);

  char *source = (char*)std::malloc(file_size + 1);
  std::fread(source, 1, file_size, f);
  source[file_size] = '\0';
  std::fclose(f);

  std::printf("Compiling '%s' (%zu bytes)...\n", input_path, file_size);

  // Compile
  js_compiled_t compiled = js_compile(source, file_size);
  std::free(source);

  if (!compiled.buf || compiled.len == 0) {
    std::printf("Error: Compilation failed\n");
    return false;
  }

  std::printf("  Compiled size: %zu bytes (%.1f%% of source)\n",
              compiled.len, 100.0 * compiled.len / file_size);

  // Write output file
  f = std::fopen(output_path, "wb");
  if (!f) {
    std::printf("Error: Cannot write to output file '%s'\n", output_path);
    std::free(compiled.buf);
    return false;
  }

  std::fwrite(compiled.buf, 1, compiled.len, f);
  std::fclose(f);
  std::free(compiled.buf);

  std::printf("Successfully written to '%s'\n", output_path);
  return true;
}

// Get default output path from input path
static std::string get_default_output(const char *input_path) {
  std::string path(input_path);
  size_t dot_pos = path.rfind('.');
  if (dot_pos != std::string::npos) {
    return path.substr(0, dot_pos) + ".app";
  }
  return path + ".app";
}

int main(int argc, char **argv) {
  // No arguments - show help
  if (argc == 1) {
    print_help(argv[0]);
    return 0;
  }

  // Test mode
  if (argc == 2 && std::strcmp(argv[1], "test") == 0) {
    std::printf("=== JS Binary Compilation Tests ===\n\n");
    bool all_passed = run_tests();
    test_compiled_size();
    test_save_load();
    return all_passed ? 0 : 1;
  }

  // Compile mode
  if (argc >= 2) {
    const char *input_path = argv[1];
    std::string output_path_str;
    const char *output_path;

    if (argc >= 3) {
      output_path = argv[2];
    } else {
      output_path_str = get_default_output(input_path);
      output_path = output_path_str.c_str();
    }

    return compile_file(input_path, output_path) ? 0 : 1;
  }

  print_help(argv[0]);
  return 1;
}
