#define IDSTART ALPHA "@?"
#define IDCONT IDSTART DIGIT

static void asm_format_char(char* const buffer, size_t const size, int const k) {
    if (isprint(k) && k != '\'') {
        snprintf(buffer, size, "'%c'", k);
    }
    else if (k != -1) {
        snprintf(buffer, size, "%s%02xh", k >= 0xa0 ? "0" : "", k);
    }
    else {
        strncpy(buffer, "eof", size);
    }
}

static int asm_get_id(lua_State* const L, Lexer* const self) {
    char const* const lexeme = self->source;
    self->source += strspn(self->source, IDCONT);
    size_t const length = self->source - lexeme;

    if (length == 7 && tolower(lexeme[0]) == 'c' && tolower(lexeme[1]) == 'o' && tolower(lexeme[2]) == 'm') {
        if (tolower(lexeme[3]) == 'm' && tolower(lexeme[4]) == 'e' && tolower(lexeme[5]) == 'n') {
            if (tolower(lexeme[6]) == 't' && isspace(lexeme[7])) {
                self->source += strspn(self->source, SPACE);
                char const delim = *self->source++;

                if (!isprint(delim)) {
                    char c[16];
                    asm_format_char(c, sizeof(c), delim);
                    return error(L, self, "invalid comment delimiter: %s", c);
                }

                // can't use block_comment as it will stop in the first delimiter
                char reject[3];
                reject[0] = '\n';
                reject[1] = delim;
                reject[2] = 0;

                for (;;) {
                    self->source += strcspn(self->source, reject);

                    if (*self->source == '\n') {
                        self->source++;
                        self->line++;
                    }
                    else if (*self->source == delim) {
                        self->source++;
                        return push(L, self, "<blockcomment>", lexeme, self->source - lexeme);
                    }
                    else {
                        return error(L, self, "unterminated comment");
                    }
                }
            }
        }
    }

    return push(L, self, "<id>", lexeme, self->source - lexeme);
}

static int asm_get_number(lua_State* const L, Lexer* const self) {
    char const* const lexeme = self->source;
    char const* token = NULL;
    int base = 2;

    while (isxdigit(*self->source)) {
        switch (*self->source++) {
            case '0':
            case '1':
                break;

            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                base = base < 8 ? 8 : base;
                break;

            case '8':
            case '9':
                base = base < 10 ? 10 : base;
                break;
            
            case 'b':
            case 'B':
                if (!isxdigit(*self->source)) {
                    self->source--;
                    goto done; // should be a binary literal
                }

                // fallthrough
            case 'a':
            case 'A':
            case 'c':
            case 'C':
            case 'd':
            case 'D':
            case 'e':
            case 'E':
            case 'f':
            case 'F':
                base = 16;
                break;
        }
    }

done:
    if (*self->source == 'h' || *self->source == 'H') {
        self->source++;
        token = "<hexadecimal>";
    }
    else if (*self->source == 'o' || *self->source == 'O') {
        self->source++;

        if (base > 8) {
            return error(L, self, "invalid hexadecimal digit");
        }

        token = "<octal>";
    }
    else if (*self->source == 'b' || *self->source == 'B') {
        self->source++;

        if (base > 2) {
            return error(L, self, "invalid binary digit");
        }

        token = "<binary>";
    }
    else {
        token = "<decimal>";

        if (base > 10) {
            return error(L, self, "invalid binary digit");
        }
    }

    return push(L, self, token, lexeme, self->source - lexeme);
}

static int asm_get_char(lua_State* const L, Lexer* const self) {
    char const* const lexeme = ++self->source;

    if (!isprint(*self->source)) {
        return error(L, self, "invalid character literal");
    }

    if (*++self->source != '\'') {
        return error(L, self, "multibyte character literals are not supported");
    }

    return push(L, self, "<char>", lexeme, 1);
}

static int asm_next_lua(lua_State* const L, Lexer* const self) {
    if (strspn(self->source, IDSTART) != 0) {
        return asm_get_id(L, self);
    }

    if (strspn(self->source, DIGIT) != 0) {
        return asm_get_number(L, self);
    }

    if (*self->source == '"') {
        return bas_get_string(L, self); // same as in BASIC
    }

    if (*self->source == '\'') {
        return asm_get_char(L, self);
    }

    char c[8];
    asm_format_char(c, sizeof(c), *self->source);
    return error(L, self, "invalid character in input: %s", c);
}

static void asm_setup_lexer(Lexer* const self) {
    self->blocks[0].type = LINE_COMMENT;
    strcpy(self->blocks[0].begin, ";");

    self->num_blocks = 1;
    self->next = asm_next_lua;
}
