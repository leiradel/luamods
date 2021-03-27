static void pas_format_char(char* const buffer, size_t const size, int const k) {
    if (isprint(k) && k != '\'') {
        snprintf(buffer, size, "'%c'", k);
    }
    else if (k != -1) {
        snprintf(buffer, size, "#%d", k);
    }
    else {
        strncpy(buffer, "eof", size);
    }
}

static int pas_get_id(lua_State* const L, Lexer* const self) {
    char const* const lexeme = self->source;
    self->source += strspn(self->source, ALNUM);
    return push(L, self, "<id>", lexeme, self->source - lexeme);
}

static int pas_get_number(lua_State* const L, Lexer* const self) {
    char const* const lexeme = self->source;
    char const* token = NULL;
  
    if (*self->source == '$' || *self->source == '&' || *self->source == '%') {
        if (*self->source == '$') {
            self->source++;
            token = "<hexadecimal>";
            bas_get_hexadecimal(L, self); // same as in BASIC
        }
        else if (*self->source == '&') {
            self->source++;
            token = "<octal>";
            bas_get_octal(L, self); // same as in BASIC
        }
        else { // '%'
            self->source++;
            token = "<binary>";
            bas_get_binary(L, self); // same as in BASIC
        }

        return push(L, self, token, lexeme, self->source - lexeme);
    }

    token = "<decimal>";
    bas_get_decimal(L, self); // same as in BASIC

    if (*self->source == '.' && (self->source[1] != '.' || !self->has_range_symbol)) {
        self->source++;
        token = "<float>";
        bas_get_decimal(L, self);
    }

    if ((*self->source == 'e' || *self->source == 'E')) {
        self->source++;
        token = "<float>";

        if (*self->source == '+' || *self->source == '-') {
            self->source++;
        }

        bas_get_decimal(L, self);
    }

    return push(L, self, token, lexeme, self->source - lexeme);
}

static int pas_get_string(lua_State* const L, Lexer* const self) {
    char const* const lexeme = self->source++;

    if (*lexeme == '#') {
        goto control;
    }

    for (;;) {
        self->source += strcspn(self->source, "'\n");

        if (*self->source == '\'') {
            self->source++;

            if (*self->source == '\'') {
                self->source++;
                continue;
            }
            else if (*self->source != '#') {
                break;
            }

            self->source++;

control:;
            size_t const length = strspn(self->source, DIGIT);

            if (length == 0) {
                return error(L, self, "control string used with no following digits");
            }
      
            self->source += length;

            if (*self->source == '#') {
                self->source++;
                goto control;
            }
            else if (*self->source != '\'') {
                break;
            }

            self->source++;
        }
        else {
            return error(L, self, "unterminated string");
        }
    }

    return push(L, self, "<string>", lexeme, self->source - lexeme);
}

static int pas_next_lua(lua_State* const L, Lexer* const self) {
    if (strspn(self->source, ALPHA) != 0) {
        return pas_get_id(L, self);
    }

    if (strspn(self->source, DIGIT "$&%") != 0) {
        return pas_get_number(L, self);
    }

    if (*self->source == '\'' || *self->source == '#') {
        return pas_get_string(L, self);
    }

    char c[8];
    pas_format_char(c, sizeof(c), *self->source);
    return error(L, self, "invalid character in input: %s", c);
}

static void pas_setup_lexer(Lexer* const self) {
    self->blocks[0].type = LINE_COMMENT;
    strcpy(self->blocks[0].begin, "//");

    self->blocks[1].type = BLOCK_DIRECTIVE;
    strcpy(self->blocks[1].begin, "(*$");
    strcpy(self->blocks[1].block_directive.end, "*)");

    self->blocks[2].type = BLOCK_DIRECTIVE;
    strcpy(self->blocks[2].begin, "{$");
    strcpy(self->blocks[2].block_directive.end, "}");

    self->blocks[3].type = BLOCK_COMMENT;
    strcpy(self->blocks[3].begin, "(*");
    strcpy(self->blocks[3].block_comment.end, "*)");

    self->blocks[4].type = BLOCK_COMMENT;
    strcpy(self->blocks[4].begin, "{");
    strcpy(self->blocks[4].block_comment.end, "}");

    self->num_blocks = 5;
    self->next = pas_next_lua;
}
