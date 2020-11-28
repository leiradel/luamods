// Line comment

/* Block comment */

/* Block comment
spawning multiple
lines */

#line test_cpp.ddl 9

#ifndef DEBUG
  #define assert(x) do { x; } while (0)
#endif

[{
  Free
  form
  block

  [{ Nested freeform block }]
}]

id
Id
iD
ID
_id
id_
_

0
0b01010101
0B10101010
0xabcd
0xABCD
0123
123
1.
.1
1.1
1.f
.1f
1.1f
1.e1
.1e1
1.1e1
1.e1f
.1e1f
1.1e1f
1.l
1u
1ul
1ull
1l
1lu
1llu
1ULL
1_km
1_

""
"\""
"\a\b\f\n\r\t\v\\\'\"\?\xab\xAB\uabcd\uABCD\U0123abcd\U0123ABCD\033"
L"wchar_t string"
u8"UTF-8 encoded string"
u"char16_t string"
U"char32_t string"
R"raw(raw (!) string)raw"
LR"raw(raw (!) wchar_t string)raw"
RL"raw(raw (!) wchar_t string)raw"
uR"raw(raw (!) char16_t string)raw"
Ru"raw(raw (!) char16_t string)raw"
UR"raw(raw (!) char32_t string)raw"
RU"raw(raw (!) char32_t string)raw"
'char character'
L'wchar_t character'
u8'UTF-8 encoded character'
u'char16_t character'
U'char32_t character'
"press"_zx81
"press"_
R"raw(press)raw"_ebcdic
