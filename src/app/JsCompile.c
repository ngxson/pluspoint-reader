// Copyright (c) 2013-2022 Cesanta Software Limited
// JS source code compilation to dense binary format

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "JsEngine.h"

// Token definitions (copied from JsEngine.c for compilation)
enum {
  TOK_ERR, TOK_EOF, TOK_IDENTIFIER, TOK_NUMBER, TOK_STRING, TOK_SEMICOLON,
  TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
  TOK_BREAK = 50, TOK_CASE, TOK_CATCH, TOK_CLASS, TOK_CONST, TOK_CONTINUE,
  TOK_DEFAULT, TOK_DELETE, TOK_DO, TOK_ELSE, TOK_FINALLY, TOK_FOR, TOK_FUNC,
  TOK_IF, TOK_IN, TOK_INSTANCEOF, TOK_LET, TOK_NEW, TOK_RETURN, TOK_SWITCH,
  TOK_THIS, TOK_THROW, TOK_TRY, TOK_VAR, TOK_VOID, TOK_WHILE, TOK_WITH,
  TOK_YIELD, TOK_UNDEF, TOK_NULL, TOK_TRUE, TOK_FALSE,
  TOK_DOT = 100, TOK_CALL, TOK_POSTINC, TOK_POSTDEC, TOK_NOT, TOK_TILDA,
  TOK_TYPEOF, TOK_UPLUS, TOK_UMINUS, TOK_EXP, TOK_MUL, TOK_DIV, TOK_REM,
  TOK_PLUS, TOK_MINUS, TOK_SHL, TOK_SHR, TOK_ZSHR, TOK_LT, TOK_LE, TOK_GT,
  TOK_GE, TOK_EQ, TOK_NE, TOK_AND, TOK_XOR, TOK_OR, TOK_LAND, TOK_LOR,
  TOK_COLON, TOK_Q, TOK_ASSIGN, TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN,
  TOK_MUL_ASSIGN, TOK_DIV_ASSIGN, TOK_REM_ASSIGN, TOK_SHL_ASSIGN,
  TOK_SHR_ASSIGN, TOK_ZSHR_ASSIGN, TOK_AND_ASSIGN, TOK_XOR_ASSIGN,
  TOK_OR_ASSIGN, TOK_COMMA,
};

// Compiled format IDs
#define COMPILED_STR  0x01  // String literal: ID + len + bytes
#define COMPILED_NUM  0x02  // Number literal: ID + len + bytes (as string)
#define COMPILED_KW   0x03  // Keyword: ID + token_id
#define COMPILED_IDENT 0x04 // Identifier: ID + len + bytes
#define COMPILED_END  0xFF  // End marker

// Helper functions for tokenization
static bool is_alpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static bool is_digit(int c) { return c >= '0' && c <= '9'; }
static bool is_xdigit(int c) { return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
static bool is_space(int c) { return c == ' ' || c == '\r' || c == '\n' || c == '\t'; }
static bool is_ident_begin(int c) { return c == '_' || c == '$' || is_alpha(c); }
static bool is_ident_continue(int c) { return c == '_' || c == '$' || is_alpha(c) || is_digit(c); }

static bool streq(const char *p, size_t n, const char *buf, size_t len) {
  return n == len && memcmp(buf, p, len) == 0;
}

static size_t skiptonext(const char *code, size_t len, size_t pos) {
  while (pos < len) {
    if (is_space(code[pos])) {
      pos++;
    } else if (pos + 1 < len && code[pos] == '/' && code[pos + 1] == '/') {
      // Single line comment
      pos += 2;
      while (pos < len && code[pos] != '\n') pos++;
    } else if (pos + 1 < len && code[pos] == '/' && code[pos + 1] == '*') {
      // Multi-line comment
      pos += 2;
      while (pos + 1 < len && !(code[pos] == '*' && code[pos + 1] == '/')) pos++;
      pos += 2;
    } else {
      break;
    }
  }
  return pos;
}

static uint8_t parsekeyword(const char *buf, size_t len) {
  switch (buf[0]) {
    case 'b': if (streq("break", 5, buf, len)) return TOK_BREAK; break;
    case 'c':
      if (streq("class", 5, buf, len)) return TOK_CLASS;
      if (streq("case", 4, buf, len)) return TOK_CASE;
      if (streq("catch", 5, buf, len)) return TOK_CATCH;
      if (streq("const", 5, buf, len)) return TOK_CONST;
      if (streq("continue", 8, buf, len)) return TOK_CONTINUE;
      break;
    case 'd':
      if (streq("do", 2, buf, len)) return TOK_DO;
      if (streq("default", 7, buf, len)) return TOK_DEFAULT;
      break;
    case 'e': if (streq("else", 4, buf, len)) return TOK_ELSE; break;
    case 'f':
      if (streq("for", 3, buf, len)) return TOK_FOR;
      if (streq("function", 8, buf, len)) return TOK_FUNC;
      if (streq("finally", 7, buf, len)) return TOK_FINALLY;
      if (streq("false", 5, buf, len)) return TOK_FALSE;
      break;
    case 'i':
      if (streq("if", 2, buf, len)) return TOK_IF;
      if (streq("in", 2, buf, len)) return TOK_IN;
      if (streq("instanceof", 10, buf, len)) return TOK_INSTANCEOF;
      break;
    case 'l': if (streq("let", 3, buf, len)) return TOK_LET; break;
    case 'n':
      if (streq("new", 3, buf, len)) return TOK_NEW;
      if (streq("null", 4, buf, len)) return TOK_NULL;
      break;
    case 'r': if (streq("return", 6, buf, len)) return TOK_RETURN; break;
    case 's': if (streq("switch", 6, buf, len)) return TOK_SWITCH; break;
    case 't':
      if (streq("try", 3, buf, len)) return TOK_TRY;
      if (streq("this", 4, buf, len)) return TOK_THIS;
      if (streq("throw", 5, buf, len)) return TOK_THROW;
      if (streq("true", 4, buf, len)) return TOK_TRUE;
      if (streq("typeof", 6, buf, len)) return TOK_TYPEOF;
      break;
    case 'u': if (streq("undefined", 9, buf, len)) return TOK_UNDEF; break;
    case 'v':
      if (streq("var", 3, buf, len)) return TOK_VAR;
      if (streq("void", 4, buf, len)) return TOK_VOID;
      break;
    case 'w':
      if (streq("while", 5, buf, len)) return TOK_WHILE;
      if (streq("with", 4, buf, len)) return TOK_WITH;
      break;
    case 'y': if (streq("yield", 5, buf, len)) return TOK_YIELD; break;
  }
  return TOK_IDENTIFIER;
}

// Dynamic buffer for building compiled output
typedef struct {
  uint8_t *data;
  size_t size;
  size_t capacity;
} buffer_t;

static void buffer_init(buffer_t *buf, size_t initial_capacity) {
  buf->data = (uint8_t*)malloc(initial_capacity);
  buf->size = 0;
  buf->capacity = initial_capacity;
}

static void buffer_append(buffer_t *buf, uint8_t byte) {
  if (buf->size >= buf->capacity) {
    buf->capacity *= 2;
    buf->data = (uint8_t*)realloc(buf->data, buf->capacity);
  }
  buf->data[buf->size++] = byte;
}

static void buffer_append_bytes(buffer_t *buf, const uint8_t *bytes, size_t len) {
  while (buf->size + len > buf->capacity) {
    buf->capacity *= 2;
    buf->data = (uint8_t*)realloc(buf->data, buf->capacity);
  }
  memcpy(buf->data + buf->size, bytes, len);
  buf->size += len;
}

// Compile JS source code to dense binary format
js_compiled_t js_compile(const char *src, size_t len) {
  js_compiled_t result = {NULL, 0};
  if (!src || len == 0) return result;

  buffer_t buf;
  buffer_init(&buf, len / 2 + 256); // Estimate smaller size

  size_t pos = 0;

  while (pos < len) {
    pos = skiptonext(src, len, pos);
    if (pos >= len) break;

    const char *token_start = src + pos;
    size_t token_len = 0;
    uint8_t tok = TOK_ERR;

    // Tokenize similar to next() function in JsEngine.c
    char c = src[pos];

    // Single character tokens - map to proper token IDs
    if (c == '?') { buffer_append(&buf, TOK_Q); pos++; continue; }
    if (c == ':') { buffer_append(&buf, TOK_COLON); pos++; continue; }
    if (c == '(') { buffer_append(&buf, TOK_LPAREN); pos++; continue; }
    if (c == ')') { buffer_append(&buf, TOK_RPAREN); pos++; continue; }
    if (c == '{') { buffer_append(&buf, TOK_LBRACE); pos++; continue; }
    if (c == '}') { buffer_append(&buf, TOK_RBRACE); pos++; continue; }
    if (c == ';') { buffer_append(&buf, TOK_SEMICOLON); pos++; continue; }
    if (c == ',') { buffer_append(&buf, TOK_COMMA); pos++; continue; }
    if (c == '.') { buffer_append(&buf, TOK_DOT); pos++; continue; }
    if (c == '~') { buffer_append(&buf, TOK_TILDA); pos++; continue; }

    // Multi-character operators
    if (c == '!') {
      if (pos + 2 < len && src[pos+1] == '=' && src[pos+2] == '=') {
        buffer_append(&buf, TOK_NE);
        pos += 3;
      } else {
        buffer_append(&buf, TOK_NOT);
        pos++;
      }
      continue;
    }

    if (c == '-') {
      if (pos + 1 < len && src[pos+1] == '-') {
        buffer_append(&buf, TOK_POSTDEC);
        pos += 2;
      } else if (pos + 1 < len && src[pos+1] == '=') {
        buffer_append(&buf, TOK_MINUS_ASSIGN);
        pos += 2;
      } else {
        buffer_append(&buf, TOK_MINUS);
        pos++;
      }
      continue;
    }

    if (c == '+') {
      if (pos + 1 < len && src[pos+1] == '+') {
        buffer_append(&buf, TOK_POSTINC);
        pos += 2;
      } else if (pos + 1 < len && src[pos+1] == '=') {
        buffer_append(&buf, TOK_PLUS_ASSIGN);
        pos += 2;
      } else {
        buffer_append(&buf, TOK_PLUS);
        pos++;
      }
      continue;
    }

    if (c == '*') {
      if (pos + 1 < len && src[pos+1] == '*') {
        buffer_append(&buf, TOK_EXP);
        pos += 2;
      } else if (pos + 1 < len && src[pos+1] == '=') {
        buffer_append(&buf, TOK_MUL_ASSIGN);
        pos += 2;
      } else {
        buffer_append(&buf, TOK_MUL);
        pos++;
      }
      continue;
    }

    if (c == '/') {
      if (pos + 1 < len && src[pos+1] == '=') {
        buffer_append(&buf, TOK_DIV_ASSIGN);
        pos += 2;
      } else {
        buffer_append(&buf, TOK_DIV);
        pos++;
      }
      continue;
    }

    if (c == '%') {
      if (pos + 1 < len && src[pos+1] == '=') {
        buffer_append(&buf, TOK_REM_ASSIGN);
        pos += 2;
      } else {
        buffer_append(&buf, TOK_REM);
        pos++;
      }
      continue;
    }

    if (c == '&') {
      if (pos + 1 < len && src[pos+1] == '&') {
        buffer_append(&buf, TOK_LAND);
        pos += 2;
      } else if (pos + 1 < len && src[pos+1] == '=') {
        buffer_append(&buf, TOK_AND_ASSIGN);
        pos += 2;
      } else {
        buffer_append(&buf, TOK_AND);
        pos++;
      }
      continue;
    }

    if (c == '|') {
      if (pos + 1 < len && src[pos+1] == '|') {
        buffer_append(&buf, TOK_LOR);
        pos += 2;
      } else if (pos + 1 < len && src[pos+1] == '=') {
        buffer_append(&buf, TOK_OR_ASSIGN);
        pos += 2;
      } else {
        buffer_append(&buf, TOK_OR);
        pos++;
      }
      continue;
    }

    if (c == '=') {
      if (pos + 2 < len && src[pos+1] == '=' && src[pos+2] == '=') {
        buffer_append(&buf, TOK_EQ);
        pos += 3;
      } else {
        buffer_append(&buf, TOK_ASSIGN);
        pos++;
      }
      continue;
    }

    if (c == '<') {
      if (pos + 2 < len && src[pos+1] == '<' && src[pos+2] == '=') {
        buffer_append(&buf, TOK_SHL_ASSIGN);
        pos += 3;
      } else if (pos + 1 < len && src[pos+1] == '<') {
        buffer_append(&buf, TOK_SHL);
        pos += 2;
      } else if (pos + 1 < len && src[pos+1] == '=') {
        buffer_append(&buf, TOK_LE);
        pos += 2;
      } else {
        buffer_append(&buf, TOK_LT);
        pos++;
      }
      continue;
    }

    if (c == '>') {
      if (pos + 2 < len && src[pos+1] == '>' && src[pos+2] == '=') {
        buffer_append(&buf, TOK_SHR_ASSIGN);
        pos += 3;
      } else if (pos + 1 < len && src[pos+1] == '>') {
        buffer_append(&buf, TOK_SHR);
        pos += 2;
      } else if (pos + 1 < len && src[pos+1] == '=') {
        buffer_append(&buf, TOK_GE);
        pos += 2;
      } else {
        buffer_append(&buf, TOK_GT);
        pos++;
      }
      continue;
    }

    if (c == '^') {
      if (pos + 1 < len && src[pos+1] == '=') {
        buffer_append(&buf, TOK_XOR_ASSIGN);
        pos += 2;
      } else {
        buffer_append(&buf, TOK_XOR);
        pos++;
      }
      continue;
    }

    // String literals - process escape sequences during compilation
    if (c == '"' || c == '\'') {
      char quote = c;
      pos++; // Skip opening quote

      // Process string and escape sequences into a temporary buffer
      uint8_t str_buf[256];
      size_t str_len = 0;

      while (pos < len && src[pos] != quote && str_len < sizeof(str_buf)) {
        if (src[pos] == '\\' && pos + 1 < len) {
          pos++; // Skip backslash
          if (src[pos] == quote) {
            str_buf[str_len++] = quote;
            pos++;
          } else if (src[pos] == 'n') {
            str_buf[str_len++] = '\n';
            pos++;
          } else if (src[pos] == 't') {
            str_buf[str_len++] = '\t';
            pos++;
          } else if (src[pos] == 'r') {
            str_buf[str_len++] = '\r';
            pos++;
          } else if (src[pos] == '\\') {
            str_buf[str_len++] = '\\';
            pos++;
          } else if (src[pos] == 'x' && pos + 2 < len &&
                     is_xdigit(src[pos+1]) && is_xdigit(src[pos+2])) {
            uint8_t hi = (src[pos+1] >= '0' && src[pos+1] <= '9') ? (src[pos+1] - '0') :
                        (src[pos+1] >= 'a' && src[pos+1] <= 'f') ? (src[pos+1] - 'a' + 10) :
                        (src[pos+1] - 'A' + 10);
            uint8_t lo = (src[pos+2] >= '0' && src[pos+2] <= '9') ? (src[pos+2] - '0') :
                        (src[pos+2] >= 'a' && src[pos+2] <= 'f') ? (src[pos+2] - 'a' + 10) :
                        (src[pos+2] - 'A' + 10);
            str_buf[str_len++] = (hi << 4) | lo;
            pos += 3;
          } else {
            // Unknown escape, just copy the char
            str_buf[str_len++] = src[pos++];
          }
        } else {
          str_buf[str_len++] = src[pos++];
        }
      }

      if (pos < len) pos++; // Skip closing quote

      // Encode as: COMPILED_STR + length + processed bytes
      if (str_len <= 255) {
        buffer_append(&buf, COMPILED_STR);
        buffer_append(&buf, (uint8_t)str_len);
        buffer_append_bytes(&buf, str_buf, str_len);
      }
      continue;
    }

    // Number literals
    if (is_digit(c) || (c == '.' && pos + 1 < len && is_digit(src[pos+1]))) {
      const char *num_start = src + pos;
      token_len = 0;

      // Simple number parsing (integer or float)
      while (pos < len && (is_digit(src[pos]) || src[pos] == '.')) {
        token_len++;
        pos++;
      }

      // Scientific notation
      if (pos < len && (src[pos] == 'e' || src[pos] == 'E')) {
        token_len++;
        pos++;
        if (pos < len && (src[pos] == '+' || src[pos] == '-')) {
          token_len++;
          pos++;
        }
        while (pos < len && is_digit(src[pos])) {
          token_len++;
          pos++;
        }
      }

      // Encode as: COMPILED_NUM + length + bytes
      if (token_len <= 255) {
        buffer_append(&buf, COMPILED_NUM);
        buffer_append(&buf, (uint8_t)token_len);
        buffer_append_bytes(&buf, (const uint8_t*)num_start, token_len);
      }
      continue;
    }

    // Identifiers and keywords
    if (is_ident_begin(c)) {
      const char *ident_start = src + pos;
      token_len = 0;

      while (pos < len && is_ident_continue(src[pos])) {
        token_len++;
        pos++;
      }

      uint8_t kw = parsekeyword(ident_start, token_len);

      if (kw == TOK_IDENTIFIER) {
        // Regular identifier - encode with length
        if (token_len <= 255) {
          buffer_append(&buf, COMPILED_IDENT);
          buffer_append(&buf, (uint8_t)token_len);
          buffer_append_bytes(&buf, (const uint8_t*)ident_start, token_len);
        }
      } else {
        // Keyword - encode as token ID
        buffer_append(&buf, COMPILED_KW);
        buffer_append(&buf, kw);
      }
      continue;
    }

    // Unknown character - skip
    pos++;
  }

  // Add end marker
  buffer_append(&buf, COMPILED_END);

  result.buf = buf.data;
  result.len = buf.size;
  return result;
}
