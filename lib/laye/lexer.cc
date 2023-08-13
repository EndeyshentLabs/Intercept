#include <laye/lexer.hh>

namespace {
using Tk = lcc::laye::TokenKind;
static struct {
    Tk kind;
    std::string image;
} keyword_infos[] = {
    {Tk::Bool, "bool"},
    {Tk::Int, "int"},
    {Tk::UInt, "uint"},
    {Tk::Float, "float"},
    {Tk::True, "true"},
    {Tk::False, "false"},
    {Tk::Nil, "nil"},
    {Tk::Global, "global"},
    {Tk::If, "if"},
    {Tk::Then, "then"},
    {Tk::Else, "else"},
    {Tk::For, "for"},
    {Tk::Do, "do"},
    {Tk::Switch, "switch"},
    {Tk::Case, "case"},
    {Tk::Default, "default"},
    {Tk::Return, "return"},
    {Tk::Break, "break"},
    {Tk::Continue, "continue"},
    {Tk::Goto, "goto"},
    {Tk::Struct, "struct"},
    {Tk::Variant, "variant"},
    {Tk::Enum, "enum"},
    {Tk::Alias, "alias"},
    //{Tk::Test, "test"},
    {Tk::Import, "import"},
    {Tk::Export, "export"},
    {Tk::From, "from"},
    {Tk::As, "as"},
    {Tk::Operator, "operator"},
    {Tk::Readonly, "readonly"},
    {Tk::Writeonly, "writeonly"},
    {Tk::New, "new"},
    {Tk::Delete, "delete"},
    {Tk::Cast, "cast"},
    {Tk::Try, "try"},
    {Tk::Catch, "catch"},
    //{Tk::Discard, "discard"},
    {Tk::Sizeof, "sizeof"},
    {Tk::Alignof, "alignof"},
    {Tk::Offsetof, "offsetof"},
    {Tk::Not, "not"},
    {Tk::And, "and"},
    {Tk::Or, "or"},
    {Tk::Xor, "xor"},
    {Tk::Varargs, "varargs"},
    {Tk::Const, "const"},
    {Tk::Foreign, "foreign"},
    {Tk::Inline, "inline"},
    {Tk::Callconv, "callconv"},
    //{Tk::Impure, "impure"},
    {Tk::Void, "void"},
    {Tk::Var, "var"},
    {Tk::Noreturn, "noreturn"},
    {Tk::Rawptr, "rawptr"},
    {Tk::String, "string"},
    {Tk::CChar, "c_char"},
    {Tk::CSChar, "c_schar"},
    {Tk::CUChar, "c_uchar"},
    {Tk::CString, "c_string"},
    {Tk::CShort, "c_short"},
    {Tk::CUShort, "c_ushort"},
    {Tk::CInt, "c_int"},
    {Tk::CUInt, "c_uint"},
    {Tk::CLong, "c_long"},
    {Tk::CULong, "c_ulong"},
    {Tk::CLongLong, "c_longlong"},
    {Tk::CULongLong, "c_ulonglong"},
    {Tk::CSizeT, "c_size_t"},
    {Tk::CISizeT, "c_isize_t"},
    {Tk::CPtrDiffT, "c_ptrdiff_t"},
    {Tk::CFloat, "c_float"},
    {Tk::CDouble, "c_double"},
    {Tk::CLongDouble, "c_longdouble"},
    {Tk::CBool, "c_bool"},

    {Tk::Invalid, {}},
};
}

void lcc::laye::Lexer::ReadToken(LayeToken& token) {
    token.kind = TokenKind::Invalid;
    token.text.clear();
    token.integer_value = 0;
    token.location.pos = CurrentOffset();
    token.location.len = 0;
    token.location.file_id = (u16) FileId();
    token.artificial = false;

    char currc = lastc;
    switch (lastc) {
        case '+': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::PlusEqual;
            } else token.kind = TokenKind::Plus;
        } break;

        case '-': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::MinusEqual;
            } else token.kind = TokenKind::Minus;
        } break;

        case '*': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::StarEqual;
            } else token.kind = TokenKind::Star;
        } break;

        case '/': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::SlashEqual;
            } else if (lastc == '/') {
                NextChar();
            finish_line_comment:
                while (lastc != '\n' && lastc != 0) {
                    NextChar();
                }

                ReadToken(token);
                return;
            } else if (lastc == '*') {
                int delimiter_count = 1;
                NextChar();

                char lastlastc = lastc;
                while (lastc != 0 && delimiter_count > 0) {
                    NextChar();

                    if (lastc == '/' && lastlastc == '*')
                        delimiter_count--;

                    lastlastc = lastc;
                }

                if (delimiter_count > 0) {
                    Error("Unfinished delimited comment in Laye source file ({} open delimiter(s) went unclosed.)", delimiter_count);
                }

                ReadToken(token);
                return;
            } else token.kind = TokenKind::Slash;
        } break;

        case '%': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::PercentEqual;
            } else token.kind = TokenKind::Percent;
        } break;

        case '&': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::AmpersandEqual;
            } else token.kind = TokenKind::Ampersand;
        } break;

        case '|': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::PipeEqual;
            } else token.kind = TokenKind::Pipe;
        } break;

        case '~': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::TildeEqual;
            } else token.kind = TokenKind::Tilde;
        } break;

        case '=': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::EqualEqual;
            } else if (lastc == '>') {
                NextChar();
                token.kind = TokenKind::EqualGreater;
            } else token.kind = TokenKind::Equal;
        } break;

        case '!': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::BangEqual;
            } else token.kind = TokenKind::Bang;
        } break;

        case '<': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::LessEqual;
            } else if (lastc == '<') {
                NextChar();
                if (lastc == '=') {
                    NextChar();
                    token.kind = TokenKind::LessLessEqual;
                } else token.kind = TokenKind::LessLess;
            } else token.kind = TokenKind::Less;
        } break;

        case '>': {
            NextChar();
            if (lastc == '=') {
                NextChar();
                token.kind = TokenKind::GreaterEqual;
            } else if (lastc == '>') {
                NextChar();
                if (lastc == '=') {
                    NextChar();
                    token.kind = TokenKind::GreaterGreaterEqual;
                } else token.kind = TokenKind::GreaterGreater;
            } else token.kind = TokenKind::Greater;
        } break;

        case ':': {
            NextChar();
            if (lastc == ':') {
                NextChar();
                token.kind = TokenKind::ColonColon;
            } else token.kind = TokenKind::Colon;
        } break;

        case ';': {
            NextChar();
            token.kind = TokenKind::SemiColon;
        } break;

        case ',': {
            NextChar();
            token.kind = TokenKind::Comma;
        } break;

        case '.': {
            NextChar();
            token.kind = TokenKind::Dot;
        } break;

        case '?': {
            NextChar();
            token.kind = TokenKind::Question;
        } break;

        case '(': {
            NextChar();
            token.kind = TokenKind::OpenParen;
        } break;

        case ')': {
            NextChar();
            token.kind = TokenKind::CloseParen;
        } break;

        case '[': {
            NextChar();
            token.kind = TokenKind::OpenBracket;
        } break;

        case ']': {
            NextChar();
            token.kind = TokenKind::CloseBracket;
        } break;

        case '{': {
            NextChar();
            token.kind = TokenKind::OpenBrace;
        } break;

        case '}': {
            NextChar();
            token.kind = TokenKind::CloseBrace;
        } break;

        case '"': {
            ReadString(token);
        } break;

        case '\'': {
            ReadRune(token);
        } break;

        case '#': {
            NextChar();
            if (lastc == '!') {
                NextChar();
                goto finish_line_comment;
            } else goto kind_invalid;
        } break;

        default: {
            if (IsIdentStart(lastc) /* or IsDigit(lastc) */) {
                ReadIdentifierOrNumber(token);
            } else {
                NextChar();
            kind_invalid:
                token.kind = TokenKind::Invalid;
                Error("Unknown character in Laye source '{}'", currc);
            }
        } break;
    }

    token.location.len = (u16) (CurrentOffset() - token.location.pos);
}

void lcc::laye::Lexer::ReadIdentifierOrNumber(LayeToken& token) {
    LCC_ASSERT(IsIdentStart(lastc));

    if (IsDigit(lastc)) {
        u64 integer_value = (u64) (lastc - '0');
        bool is_int_too_large = false;

        bool is_lastc_underscore = false;
        do {
            is_lastc_underscore = lastc == '_';
            token.text += lastc;

            if (not is_lastc_underscore) {
                u64 digit_value = (u64) (lastc - '0');
                if ((std::numeric_limits<u64>::max() - digit_value) / 10 < integer_value)
                    is_int_too_large = true;

                if (!is_int_too_large)
                    integer_value = integer_value * 10 + digit_value;
            }

            NextChar();
        } while (IsDigit(lastc) or lastc == '_');

        if (IsAlpha(lastc)) {
            goto continue_identifier;
        }

        if (is_lastc_underscore) {
            Error("The '_' digit separator cannot end a number literal");
        }

        token.location.len = (u16) (CurrentOffset() - token.location.pos);
        if (lastc == '#') {
            NextChar();
            u64 radix = integer_value;
            if (radix < 2 or radix > 36 or is_int_too_large) {
                if (radix < 2) radix = 2;
                else radix = 36;
                Error(token.location, "Number base value must be in the rage [2, 36]");
            }
            ReadIntegerInBase(token, (int) radix);
        } else if (lastc == '.') {
            token.integer_value = integer_value;
            ReadFloatInBase(token, 10);
        } else {
            if (is_int_too_large) {
                Error(token.location, "Integer literal does not fit within an unsigned 64-bit value");
            }
            token.integer_value = integer_value;
            token.kind = TokenKind::LitInt;
        }

        return;
    }

continue_identifier:
    do {
        token.text += lastc;
        NextChar();
    } while (IsIdentContinue(lastc));

    for (int i = 0; keyword_infos[i].kind != TokenKind::Invalid; i++) {
        if (token.text == keyword_infos[i].image) {
            token.kind = keyword_infos[i].kind;
            return;
        }
    }

    char c = token.text[0];
    if (c == 'b' or c == 'i' or c == 'u' or c == 'f') {
        u64 integer_value = 0;
        bool is_int_too_large = false;

        bool are_rest_digits = true;
        for (usz i = 1; are_rest_digits and i < token.text.size(); i++) {
            if (not IsDigit(token.text[i])) {
                are_rest_digits = false;
            } else {
                u64 digit_value = (u64) (lastc - '0');
                if ((std::numeric_limits<u64>::max() - digit_value) / 10 < integer_value)
                    is_int_too_large = true;

                if (!is_int_too_large)
                    integer_value = integer_value * 10 + digit_value;
            }
        }

        if (are_rest_digits) {
            if (integer_value == 0 or integer_value > 65535 or is_int_too_large) {
                Error(token.location, "Sized primitive bit width must be in the range [1, 65535]");
            }
            // clang-format off
            token.integer_value = integer_value;
            token.kind = c == 'b' ? TokenKind::Bool :
                         c == 'i' ? TokenKind::Int  :
                         c == 'u' ? TokenKind::UInt :
                                    TokenKind::Float;
            return;
        } // clang-format on
    }

    token.kind = TokenKind::Ident;
}

void lcc::laye::Lexer::ReadIntegerInBase(LayeToken& token, int base) {
    u64 integer_value = (u64) (lastc - '0');
    bool is_int_too_large = false;

    bool is_lastc_underscore = false;
    do {
        is_lastc_underscore = lastc == '_';
        token.text += lastc;

        if (not is_lastc_underscore) {
            u64 digit_value = (u64) GetDigitValueInBase(lastc, base);
            if ((std::numeric_limits<u64>::max() - digit_value) / (u64) base < integer_value)
                is_int_too_large = true;

            if (!is_int_too_large)
                integer_value = integer_value * (u64) base + digit_value;
        }

        NextChar();
    } while (IsDigitInBase(lastc, base) or lastc == '_');

    if (IsAlpha(lastc)) {
        Error("Number literal cannot contain letter characters not within its base");
        do {
            NextChar();
        } while (IsAlphaNumeric(lastc));
    } else if (is_lastc_underscore) {
        Error("The '_' digit separator cannot end a number literal");
    }

    token.location.len = (u16) (CurrentOffset() - token.location.pos);
    token.integer_value = integer_value;
    if (lastc == '.') {
        ReadFloatInBase(token, base);
    } else {
        if (is_int_too_large) {
            Error(token.location, "Integer literal does not fit within an unsigned 64-bit value");
        }
        token.kind = TokenKind::LitInt;
    }
}

void lcc::laye::Lexer::ReadFloatInBase(LayeToken& token, int base) {
    LCC_ASSERT(lastc == '.');
    NextChar();

    // Diag::ICE(context, CurrentLocation(), "No floats! (yet)");

    bool is_lastc_underscore = false;
    do {
        is_lastc_underscore = lastc == '_';
        token.text += lastc;
        NextChar();
    } while (IsDigitInBase(lastc, base) or lastc == '_');

    if (IsAlpha(lastc)) {
        Error("Number literal cannot contain letter characters not within its base");
        do {
            NextChar();
        } while (IsAlphaNumeric(lastc));
    } else if (is_lastc_underscore) {
        Error("The '_' digit separator cannot end a number literal");
    }

    token.kind = TokenKind::LitFloat;

    Error("Float values are not currently supported beyond the lexing stage");
}

void lcc::laye::Lexer::ReadString(LayeToken& token) {
    LCC_ASSERT(lastc == '"');

    NextChar();
    while (lastc != 0 && lastc != '"') {
        if (lastc == '\\') {
            ReadEscapeSequence(token);
        } else {
            token.text += lastc;
            NextChar();
        }
    }

    if (lastc != '"') {
        Error("Unfinished string literal");
    } else NextChar();

    token.kind = TokenKind::LitString;
}

void lcc::laye::Lexer::ReadRune(LayeToken& token) {
    LCC_ASSERT(lastc == '\'');

    NextChar();
    if (lastc == '\\') {
        ReadEscapeSequence(token);
    } else {
        token.text += lastc;
        NextChar();
    }

    if (lastc != '\'') {
        Error("Unfinished rune literal");
    } else NextChar();

    token.kind = TokenKind::LitRune;
}

void lcc::laye::Lexer::ReadEscapeSequence(LayeToken& token) {
    LCC_ASSERT(lastc == '\\');

    NextChar();
    switch (lastc) {
        case 'n':
            tok.text += '\n';
            NextChar();
            break;
        case 'r':
            tok.text += '\r';
            NextChar();
            break;
        case 't':
            tok.text += '\t';
            NextChar();
            break;
        case 'f':
            tok.text += '\f';
            NextChar();
            break;
        case 'v':
            tok.text += '\v';
            NextChar();
            break;
        case 'a':
            tok.text += '\a';
            NextChar();
            break;
        case 'b':
            tok.text += '\b';
            NextChar();
            break;
        case 'e':
            tok.text += '\033';
            NextChar();
            break;
        case '0':
            tok.text += '\0';
            NextChar();
            break;
        case '\'':
            tok.text += '\'';
            NextChar();
            break;
        case '\"':
            tok.text += '\"';
            NextChar();
            break;
        case '\\':
            tok.text += '\\';
            NextChar();
            break;
        // TODO(local): unicode escape sequences
        default:
            Error("Invalid escape sequence");
            NextChar();
            break;
    }
}
