static void cpp_format_char(char* const buffer, size_t const size, int const k) {
    if (isprint(k) && k != '\'') {
        snprintf(buffer, size, "'%c'", k);
    }
    else if (k != -1) {
        snprintf(buffer, size, "'\\%c%c%c'", (k >> 6) + '0', ((k >> 3) & 7) + '0', (k & 7) + '0');
    }
    else {
        strncpy(buffer, "eof", size);
    }
}

static int cpp_get_id(lua_State* const L, Lexer* const self) {
    char const* const lexeme = self->source;
    self->source += strspn(self->source, ALNUM);
    return push(L, self, "<id>", lexeme, self->source - lexeme);
}

static int cpp_get_digits(lua_State* const L, Lexer* const self, char const* const base_name, char const* const valid_digits) {
    if (*self->source == '\'') {
        return error(L, self, "digit separator before any digits");
    }

    size_t const length = strspn(self->source, valid_digits);

    if (length == 0) {
        char c[8];
        cpp_format_char(c, sizeof(c), *self->source);
        return error(L, self, "invalid digit %s in %s constant", c, base_name);
    }

    if (self->source[length - 1] != '\'') {
        self->source += length;
    }
    else {
        self->source += length - 1;
    }

    return 0;
}

static int cpp_get_hexadecimal(lua_State* const L, Lexer* const self) {
    return cpp_get_digits(L, self, "hexadecimal", XDIGIT "'");
}

static int cpp_get_decimal(lua_State* const L, Lexer* const self) {
    return cpp_get_digits(L, self, "decimal", DIGIT "'");
}

static int cpp_get_octal(lua_State* const L, Lexer* const self) {
    return cpp_get_digits(L, self, "octal", ODIGIT "'");
}

static int cpp_get_binary(lua_State* const L, Lexer* const self) {
    return cpp_get_digits(L, self, "binary", BDIGIT "'");
}

static int cpp_get_user_defined_suffix(lua_State* const L, Lexer* const self) {
    if (*self->source != '_') {
        return error(L, self, "user defined suffixes must begin with '_'");
    }

    self->source += strspn(self->source, ALNUM);
    return 0;
}

static int cpp_get_integer_suffix(lua_State* const L, Lexer* const self) {
    static char const* const suffixes = "u,ul,ull,l,lu,ll,llu,f,";

    if (*self->source == '_') {
        return cpp_get_user_defined_suffix(L, self);
    }

    return get_suffix(L, self, suffixes);
}

static int cpp_get_float_suffix(lua_State* const L, Lexer* const self) {
    static char const* const suffixes = "f,l,";

    if (*self->source == '_') {
        return cpp_get_user_defined_suffix(L, self);
    }

    return get_suffix(L, self, suffixes);
}

static int cpp_get_number(lua_State* const L, Lexer* const self) {
    char const* const lexeme = self->source;
  
    if (*self->source == '0') {
        char const* token = NULL;

        if (self->source[1] == 'x' || self->source[1] == 'X') {
            self->source += 2;
            token = "<hexadecimal>";
            cpp_get_hexadecimal(L, self);
        }
        else if (self->source[1] == 'b' || self->source[1] == 'B') {
            self->source += 2;
            token = "<binary>";
            cpp_get_binary(L, self);
        }
        else {
            size_t const length = strspn(self->source, DIGIT);

            if (self->source[length] == '.' || self->source[length] == 'e' || self->source[length] == 'E') {
                goto is_float;
            }

            token = "<octal>";
            cpp_get_octal(L, self);

            if (self->source - lexeme == 1) {
                token = "<decimal>";
            }
        }

        cpp_get_integer_suffix(L, self);
        return push(L, self, token, lexeme, self->source - lexeme);
    }

is_float:
    if (*self->source != '.') {
        cpp_get_decimal(L, self);

        if (*self->source != '.' && *self->source != 'e' && *self->source != 'E') {
            cpp_get_integer_suffix(L, self);
            return push(L, self, "<decimal>", lexeme, self->source - lexeme);
        }
    }

    if (*self->source == '.') {
        self->source++;

        if (strspn(self->source, DIGIT) != 0) {
            cpp_get_decimal(L, self);
        }
    }

    if (*self->source == 'e' || *self->source == 'E') {
        self->source++;

        if (*self->source == '+' || *self->source == '-') {
            self->source++;
        }

        cpp_get_decimal(L, self);
    }

    cpp_get_float_suffix(L, self);
    return push(L, self, "<float>", lexeme, self->source - lexeme);
}

static int cpp_get_string(lua_State* const L, Lexer* const self, unsigned const skip, char const quote, char const* const prefix) {
    char const* const lexeme = self->source;
    self->source += skip + 1;

    char reject[4];
    reject[0] = quote;
    reject[1] = '\\';
    reject[2] = '\n';
    reject[3] = 0;

    for (;;) {
        self->source += strcspn(self->source, reject);

        if (*self->source == quote) {
            self->source++;
            break;
        }
        else if (*self->source == '\\') {
            self->source++;

            switch (*self->source++) {
                case 'a':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                case 'v':
                case '\\':
                case '\'':
                case '"':
                case '?':
                    continue;
      
                case 'x': {
                    size_t const length = strspn(self->source, XDIGIT);

                    if (length == 0) {
                        return error(L, self, "\\x used with no following hex digits");
                    }
    
                    self->source += length;
                    continue;
                }
      
                case 'u': {
                    size_t const length = strspn(self->source, XDIGIT);

                    if (length != 4) {
                        return error(L, self, "\\u needs 4 hexadecimal digits");
                    }
    
                    self->source += 4;
                    continue;
                }
            
                case 'U': {
                    size_t const length = strspn(self->source, XDIGIT);

                    if (length != 8) {
                        return error(L, self, "\\u needs 8 hexadecimal digits");
                    }
    
                    self->source += 8;
                    continue;
                }

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    self->source += strspn(self->source, ODIGIT);
                    continue;
            }
    
            char c[8];
            cpp_format_char(c, sizeof(c), self->source[-1]);
            return error(L, self, "unknown escape sequence: %s", c);
        }
        else {
            return error(L, self, "unterminated %s", quote == '"' ? "string" : "char");
        }
    }

    if (*self->source == '_') {
        cpp_get_user_defined_suffix(L, self);
    }

    char token[32];
    snprintf(token, sizeof(token), "<%s%s>", prefix, quote == '"' ? "string" : "char");

    return push(L, self, token, lexeme, self->source - lexeme);
}

static int cpp_get_rawstring(lua_State* const L, Lexer* const self, unsigned const skip, char const* const token) {
    char const* const lexeme = self->source;
    self->source += skip + 1;

    size_t const count = strcspn(self->source, NOTDELIM);

    if (count > 16) {
        return error(L, self, "raw string delimiter longer than 16 characters");
    }

    char delimiter[24];
    delimiter[0] = ')';
    memcpy(delimiter + 1, self->source, count);
    delimiter[count + 1] = '"';
    delimiter[count + 2] = 0;

    self->source += count;

    if (*self->source != '(') {
        char c[8];
        cpp_format_char(c, sizeof(c), *self->source);
        return error(L, self, "invalid character %s in raw string delimiter", c);
    }

    self->source = strstr(self->source + 1, delimiter);

    if (self->source == NULL) {
        return error(L, self, "missing raw string terminating delimiter %.*s", (int)count + 1, delimiter);
    }

    self->source += count + 2;

    if (*self->source == '_') {
        cpp_get_user_defined_suffix(L, self);
    }

    return push(L, self, token, lexeme, self->source - lexeme);
}

static int cpp_next_lua(lua_State* const L, Lexer* const self) {
    if (strspn(self->source, DIGIT ".") != 0) {
        return cpp_get_number(L, self);
    }

    char const k0 = *self->source;

    if (k0 == '"' || k0 == '\'') {
        return cpp_get_string(L, self, 0, k0, "");
    }

    char const k1 = k0 != 0 ? self->source[1] : 0;

    if (k1 == '"' || k1 == '\'') {
        if (k0 == 'L') {
            return cpp_get_string(L, self, 1, k1, "wide");
        }
        else if (k0 == 'u') {
            return cpp_get_string(L, self, 1, k1, "utf16");
        }
        else if (k0 == 'U') {
            return cpp_get_string(L, self, 1, k1, "utf32");
        }
        else if (k0 == 'R' && k1 == '"') {
            return cpp_get_rawstring(L, self, 1, "<rawstring>");
        }
    }

    char const k2 = k1 != 0 ? self->source[2] : 0;

    if (k2 == '"') {
        if (k0 == 'u' && k1 == '8') {
            return cpp_get_string(L, self, 2, k2, "utf8");
        }
        else if ((k0 == 'L' && k1 == 'R') || (k0 == 'R' && k1 == 'L')) {
            return cpp_get_rawstring(L, self, 2, "<rawwidestring>");
        }
        else if ((k0 == 'u' && k1 == 'R') || (k0 == 'R' && k1 == 'u')) {
            return cpp_get_rawstring(L, self, 2, "<rawutf16string>");
        }
        else if ((k0 == 'U' && k1 == 'R') || (k0 == 'R' && k1 == 'U')) {
            return cpp_get_rawstring(L, self, 2, "<rawutf32string>");
        }
    }

    if (k0 == 'u' && k1 == '8' && k2 == '\'') {
        return cpp_get_string(L, self, 2, k2, "utf8");
    }

    if (strspn(self->source, ALPHA) != 0) {
        return cpp_get_id(L, self);
    }

    char c[8];
    cpp_format_char(c, sizeof(c), *self->source);
    return error(L, self, "invalid character in input: %s", c);
}

static void cpp_setup_lexer(Lexer* self) {
    self->blocks[0].type = LINE_COMMENT;
    strcpy(self->blocks[0].begin, "//");

    self->blocks[1].type = BLOCK_COMMENT;
    strcpy(self->blocks[1].begin, "/*");
    strcpy(self->blocks[1].block_comment.end, "*/");

    self->blocks[2].type = LINE_DIRECTIVE;
    strcpy(self->blocks[2].begin, "#");
    self->blocks[2].line_directive.at_start = 0;

    self->num_blocks = 3;
    self->next = cpp_next_lua;
}
