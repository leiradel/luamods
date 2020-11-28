static void bas_format_char(char* const buffer, size_t const size, int const k) {
    if (isprint(k) && k != '\'') {
        snprintf(buffer, size, "'%c'", (char)k);
    }
    else if (k != -1) {
        snprintf(buffer, size, "Chr(%d)", k);
    }
    else {
        strncpy(buffer, "eof", size);
    }
}

static int bas_get_id(lua_State* const L, Lexer* const self) {
    char const* const lexeme = self->source;
    self->source += strspn(self->source, ALNUM);
    size_t const length = self->source - lexeme;

    if (length == 3 && tolower(lexeme[0]) == 'r' && tolower(lexeme[1]) == 'e' && tolower(lexeme[2]) == 'm') {
        self->source -= 3;

        Block block;
        block.type = LINE_COMMENT;
        strcpy(block.begin, "REM");

        return line_comment(L, self, &block, 0);
    }

    return push(L, self, "<id>", lexeme, length);
}

static int bas_get_digits(lua_State* const L, Lexer* const self, char const* const base_name, char const* const valid_digits) {
    size_t const length = strspn(self->source, valid_digits);

    if (length == 0) {
        char c[16];
        bas_format_char(c, sizeof(c), *self->source);
        return error(L, self, "invalid digit %s in %s constant", c, base_name);
    }

    self->source += length;
    return 0;
}

static int bas_get_hexadecimal(lua_State* const L, Lexer* const self) {
    return bas_get_digits(L, self, "hexadecimal", XDIGIT);
}

static int bas_get_decimal(lua_State* const L, Lexer* const self) {
    return bas_get_digits(L, self, "decimal", DIGIT);
}

static int bas_get_octal(lua_State* const L, Lexer* const self) {
    return bas_get_digits(L, self, "octal", ODIGIT);
}

static int bas_get_binary(lua_State* const L, Lexer* const self) {
    return bas_get_digits(L, self, "binary", BDIGIT);
}

static int bas_get_integer_suffix(lua_State* const L, Lexer* const self) {
    static char const* const integer_suffixes = "%,&,s,us,i,ui,l,ul,";
    return get_suffix(L, self, integer_suffixes);
}

static int bas_get_float_suffix(lua_State* const L, Lexer* const self) {
    static char const* const suffixes = "@,!,#,f,r,d,";
    return get_suffix(L, self, suffixes);
}

static int bas_get_number(lua_State* const L, Lexer* const self) {
    char const* const lexeme = self->source;
  
    if (*self->source == '&') {
        self->source++;
        char const* token = NULL;

        if (*self->source == 'h' || *self->source == 'H') {
            self->source++;
            token = "<hexadecimal>";
            bas_get_hexadecimal(L, self);
        }
        else if (*self->source == 'o' || *self->source == 'O') {
            self->source++;
            token = "<octal>";
            bas_get_octal(L, self);
        }
        else if (*self->source == 'b' || *self->source == 'B') {
            self->source++;
            token = "<binary>";
            bas_get_binary(L, self);
        }
        else {
            char c[16];
            bas_format_char(c, sizeof(c), *self->source);
            return error(L, self, "invalid integer prefix \"&%s\"", c);
        }

        bas_get_integer_suffix(L, self);
        return push(L, self, token, lexeme, self->source - lexeme);
    }

    if (*self->source != '.') {
        bas_get_decimal(L, self);

        if (*self->source != '.' && *self->source != 'e' && *self->source != 'E') {
            char const* token = "<decimal>";

            if (bas_get_integer_suffix(L, self) != 0 && bas_get_float_suffix(L, self) == 0) {
                token = "<float>";
            }

            return push(L, self, token, lexeme, self->source - lexeme);
        }
    }

    if (*self->source == '.') {
        self->source++;
        self->source += strspn(self->source, DIGIT);
    }

    if ((*self->source == 'e' || *self->source == 'E')) {
        self->source++;

        if (*self->source == '+' || *self->source == '-') {
            self->source++;
        }

        bas_get_decimal(L, self);
    }

    bas_get_float_suffix(L, self);
    return push(L, self, "<float>", lexeme, self->source - lexeme);
}

static int bas_get_string(lua_State* const L, Lexer* const self) {
    char const* const lexeme = self->source++;

    for (;;) {
        self->source += strcspn(self->source, "\"\n");

        if (*self->source == '"') {
            if (self->source[1] != '"') {
                self->source++;
                break;
            }

            self->source += 2;
        }
        else {
            return error(L, self, "unterminated string");
        }
    }

    return push(L, self, "<string>", lexeme, self->source - lexeme);
}

static int bas_next_lua(lua_State* const L, Lexer* const self) {
    if (strspn(self->source, ALPHA) != 0) {
        return bas_get_id(L, self);
    }

    if (strspn(self->source, DIGIT ".") != 0) {
        return bas_get_number(L, self);
    }

    if (*self->source == '&') {
        char const k = self->source[1];

        if (k == 'h' || k == 'H' || k == 'o' || k == 'O' || k == 'b' || k == 'B') {
            return bas_get_number(L, self);
        }
    }

    if (*self->source == '"') {
        return bas_get_string(L, self);
    }

    char c[16];
    bas_format_char(c, sizeof(c), *self->source);
    return error(L, self, "invalid character in input: %s", c);
}

static void bas_setup_lexer(Lexer* const self) {
    self->blocks[0].type = LINE_COMMENT;
    strcpy(self->blocks[0].begin, "'");

    self->num_blocks = 1;
    self->next = bas_next_lua;
}
