#include <stdio.h>
#include <string>
#include <functional>
#include <initializer_list>

#define FIX_FUNC emit("<fixme "); emit(__FUNCTION__); emit(">");

using f = std::function<void()>;

void emit(const char* s) {
  while(*s) putc(*s++, stdout);
}

void any_char() {
  char buf[2] = {0};
  buf[0] = 33 + (rand() % 94);
  // FIXME this is supposed to exclude different ranges for c_char and s_char
  emit(buf);
}

void oneof(const std::string& s) {
  putc(s.data()[rand() % s.size()], stdout);
}

void oneof(std::initializer_list<std::string> s) {
  emit(s.begin()[rand() % s.size()].c_str());
}

void opt(f pattern) {
  if (rand() & 1) pattern();
}

void oneof(const std::vector<f>& opts) {
  opts[rand() % opts.size()]();
}

void octal_digit() {
  oneof("01234567");
}

void nonzero_digit() {
  oneof("123456789");
}

void digit() {
  oneof("0123456789");
}

void hexadecimal_digit() {
  oneof("0123456789ABCDEF");
}

void nondigit() {
  oneof("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
}

void identifier_nondigit() {
  nondigit();
}

void identifier() {
  oneof({
    []() { identifier_nondigit(); },
    []() { identifier(); identifier_nondigit(); },
    []() { identifier(); digit(); },
  });
}

void decimal_constant() {
  oneof({
    [](){ nonzero_digit(); },
    [](){ decimal_constant(); digit(); },
  });
}

void octal_constant() {
  oneof({
    [](){ emit("0"); },
    [](){ octal_constant(); octal_digit(); },
  });
}

void hexadecimal_prefix() { oneof({ "0x", "0X" }); }
void unsigned_suffix()    { oneof({ "u", "U" });   }
void long_suffix()        { oneof({ "l", "L" });   }
void long_long_suffix()   { oneof({ "ll", "LL" }); }

void hexadecimal_constant() {
  oneof({
    [](){ hexadecimal_prefix(); hexadecimal_digit(); },
    [](){ hexadecimal_constant(); hexadecimal_digit(); },
  });
}

void integer_suffix() {
  oneof({
    [](){ unsigned_suffix();  opt(long_suffix); },
    [](){ unsigned_suffix();  opt(long_long_suffix); },
    [](){ long_suffix();      opt(unsigned_suffix); },
    [](){ long_long_suffix(); opt(unsigned_suffix); },
  });
}

void integer_constant() {
  oneof({
    [](){ decimal_constant();     opt(integer_suffix); },
    [](){ octal_constant();       opt(integer_suffix); },
    [](){ hexadecimal_constant(); opt(integer_suffix); },
  });
}

void digit_sequence() {
  oneof({
    [](){ digit(); },
    [](){ digit_sequence(); digit(); },
  });
}

void fractional_constant() {
  oneof({
    [](){ opt(digit_sequence); emit("."); digit_sequence(); },
    [](){ digit_sequence(); emit("."); },
  });
}

void sign() {
  oneof({"+", "-"});
}

void exponent_part() {
  oneof({
    [](){ emit("e"); opt(sign); digit_sequence(); },
    [](){ emit("E"); opt(sign); digit_sequence(); },
  });
}

void floating_suffix() {
  oneof({ "f", "l", "F", "L" });
}

void decimal_floating_constant() {
  oneof({
    [](){ fractional_constant(); opt(exponent_part); opt(floating_suffix); },
    [](){ digit_sequence(); exponent_part(); opt(floating_suffix); },
  });
}

void hexadecimal_digit_sequence() {
  oneof({
    [](){ hexadecimal_digit(); },
    [](){ hexadecimal_digit_sequence(); hexadecimal_digit(); },
  });
}

void binary_exponent_part() {
  oneof({
    [](){ emit("p"); opt(sign); digit_sequence(); },
    [](){ emit("P"); opt(sign); digit_sequence();  },
  });
}

void hexadecimal_fractional_constant() {
  oneof({
    [](){ opt(hexadecimal_digit_sequence);  emit("."); hexadecimal_digit_sequence(); },
    [](){     hexadecimal_digit_sequence(); emit("."); },
  });
}

void hexadecimal_floating_constant() {
  oneof({
    []() { hexadecimal_prefix(); hexadecimal_fractional_constant(); binary_exponent_part(); opt(floating_suffix); },
    []() { hexadecimal_prefix(); hexadecimal_digit_sequence(); binary_exponent_part(); opt(floating_suffix); },
  });
}

void floating_constant() {
  oneof({ decimal_floating_constant, hexadecimal_floating_constant });
}

void enumeration_constant() {
  identifier();
}

void simple_escape_sequence() {
  oneof({ "\\'", "\\\"", "\\?", "\\\\", "\\a", "\\b", "\\f", "\\n", "\\r", "\\t", "\\v" });
}

void octal_escape_sequence() {
  oneof({
    [](){ emit("\\"); octal_digit(); },
    [](){ emit("\\"); octal_digit(); octal_digit(); },
    [](){ emit("\\"); octal_digit(); octal_digit(); octal_digit(); },
  });
}

void hexadecimal_escape_sequence() {
  oneof({
    [](){ emit("\\x"); hexadecimal_digit(); },
    [](){ hexadecimal_escape_sequence(); hexadecimal_digit(); },
  });
}

void hex_quad() {
  hexadecimal_digit();
  hexadecimal_digit();
  hexadecimal_digit();
  hexadecimal_digit();
}

void universal_character_name() {
  oneof({
    [](){ emit("\\u"); hex_quad(); },
    [](){ emit("\\U"); hex_quad(); hex_quad(); },
  });
}

void escape_sequence() {
  oneof({
    simple_escape_sequence,
    octal_escape_sequence,
    hexadecimal_escape_sequence,
    universal_character_name
  });
}

void c_char() {
  oneof({
    [](){ any_char(); },
    [](){ escape_sequence(); },
  });
}

void c_char_sequence() {
  oneof({
    [](){ c_char(); },
    [](){ c_char_sequence(); c_char(); },
  });
}

void character_constant() {
  oneof({
    [](){ emit("'");  c_char_sequence(); emit("'"); },
    [](){ emit("L'"); c_char_sequence(); emit("'"); },
  });
}

void constant() {
  oneof({
    integer_constant,
    floating_constant,
    enumeration_constant,
    character_constant
  });
}

void s_char() {
  oneof({
    [](){ any_char(); },
    [](){ escape_sequence(); },
  });
}

void s_char_sequence() {
  oneof({
    [](){ s_char(); },
    [](){ s_char_sequence(); s_char(); },
  });
}

void punctuator() {
  oneof({
    "[", "]", "(", ")", "{", "}", ".", "->", "++", "--", "&", "*", "+", "-", "~", "!",
    "/", "%", "<<", ">>", "<", ">", "<=", ">=", "==", "!=", "^", "|", "&&", "||",
    "?", ":", ";", "...", "=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|=",
    ",", "#", "##",
    "<:", ":>", "<%", "%>", "%:", "%:%:"
  });
}

void string_literal() {
  oneof({
    [](){ emit("\""); opt(s_char_sequence);  emit("\""); },
    [](){ emit("L\""); opt(s_char_sequence); emit("\""); },
  });
}

//------------------------------------------------------------------------------

void expression();

void primary_expression() {
  oneof({
    identifier,
    constant,
    string_literal,
    [](){ emit("("); expression(); emit(")"); }
  });
}

void specifier_qualifier_list() {
  FIX_FUNC;
}

void identifier_list() {
  oneof({
    [](){ identifier(); },
    [](){ identifier_list(); emit(","); identifier(); },
  });
}

void type_qualifier() {
  oneof({"const", "restrict", "volatile"});
}

void type_qualifier_list() {
  oneof({
    [](){ type_qualifier(); },
    [](){ type_qualifier_list(); type_qualifier(); },
  });
}

void pointer() {
  oneof({
    [](){ emit("*"); opt(type_qualifier_list); },
    [](){ emit("*"); opt(type_qualifier_list); pointer(); },
  });
}

void abstract_declarator();

void assignment_expression() {
  FIX_FUNC;
}

void storage_class_specifier() {
  oneof({"typedef", "extern", "static", "auto", "register"});
}

void and_expression() {
  FIX_FUNC;
}

void exclusive_or_expression() {
  oneof({
    [](){ and_expression(); },
    [](){ and_expression(); },
    [](){ and_expression(); },
    [](){ exclusive_or_expression(); emit("^"); and_expression(); },
  });
}

void inclusive_or_expression() {
  oneof({
    [](){ exclusive_or_expression(); },
    [](){ exclusive_or_expression(); },
    [](){ exclusive_or_expression(); },
    [](){ inclusive_or_expression(); emit("|"); exclusive_or_expression(); },
  });
}

void logical_and_expression() {
  oneof({
    [](){ inclusive_or_expression(); },
    [](){ inclusive_or_expression(); },
    [](){ inclusive_or_expression(); },
    [](){ logical_and_expression(); emit("&&"); inclusive_or_expression(); },
  });
}

void logical_or_expression() {
  oneof({
    [](){ logical_and_expression(); },
    [](){ logical_and_expression(); },
    [](){ logical_and_expression(); },
    [](){ logical_or_expression(); emit("||"); logical_and_expression(); },
  });
}

void conditional_expression() {
  oneof({
    [](){ logical_or_expression(); },
    [](){ logical_or_expression(); },
    [](){ logical_or_expression(); },
    [](){ logical_or_expression(); emit("?"); expression(); emit(":"); conditional_expression(); },
  });
}

void constant_expression() {
  conditional_expression();
}

void enumerator() {
  oneof({
    [](){ enumeration_constant(); },
    [](){ enumeration_constant(); emit("="); constant_expression(); },
  });
}

void enumerator_list() {
  oneof({
    [](){ enumerator(); },
    [](){ enumerator_list(); emit(","); enumerator(); },
  });
}

void enum_specifier() {
  oneof({
    [](){ emit("enum"); opt(identifier); emit("{"); enumerator_list(); emit("}"); },
    [](){ emit("enum"); opt(identifier); emit("{"); enumerator_list(); emit(","); emit("}"); },
    [](){ emit("enum"); identifier();    },
  });
}

void struct_or_union_specifier() {
  oneof({"struct", "union"});
}

void typedef_name() {
  identifier();
}

void type_specifier() {
  oneof({
    [](){ emit("void"); },
    [](){ emit("char"); },
    [](){ emit("short"); },
    [](){ emit("int"); },
    [](){ emit("long"); },
    [](){ emit("float"); },
    [](){ emit("double"); },
    [](){ emit("signed"); },
    [](){ emit("unsigned"); },
    [](){ emit("_Bool"); },
    [](){ emit("_Complex"); },
    [](){ struct_or_union_specifier(); },
    [](){ enum_specifier(); },
    [](){ typedef_name(); },
  });
}

void function_specifier() {
  emit("inline");
}

void declaration_specifiers() {
  oneof({
    [](){ storage_class_specifier(); opt(declaration_specifiers); },
    [](){ type_specifier();          opt(declaration_specifiers); },
    [](){ type_qualifier();          opt(declaration_specifiers); },
    [](){ function_specifier();      opt(declaration_specifiers); },
  });
}

void direct_declarator() {
  FIX_FUNC;
}

void declarator() {
  opt(pointer); direct_declarator();
}

void parameter_declaration() {
  oneof({
    [](){ declaration_specifiers(); declarator(); },
    [](){ declaration_specifiers(); opt(abstract_declarator); },
  });
}

void parameter_list() {
  oneof({
    [](){ parameter_declaration(); },
    [](){ parameter_list(); emit(","); parameter_declaration(); },
  });
}

void parameter_type_list() {
  oneof({
    [](){ parameter_list(); },
    [](){ parameter_list(); emit(","); emit("..."); },
  });
}

void direct_abstract_declarator() {
  oneof({
    [](){ emit("("); abstract_declarator(); emit(")"); },
    [](){ opt(direct_abstract_declarator); emit("["); opt(type_qualifier_list); opt(assignment_expression); emit("]"); },
    [](){ opt(direct_abstract_declarator); emit("["); emit("static"); opt(type_qualifier_list); assignment_expression(); emit("]"); },
    [](){ opt(direct_abstract_declarator); emit("["); type_qualifier_list(); emit("static"); assignment_expression(); emit("]"); },
    [](){ opt(direct_abstract_declarator); emit("["); emit("*"); emit("]"); },
    [](){ opt(direct_abstract_declarator); emit("("); opt(parameter_type_list); emit(")"); },
  });
}

void abstract_declarator() {
  oneof({
    [](){ pointer(); },
    [](){ opt(pointer); direct_abstract_declarator(); },
  });
}

void type_name() {
  specifier_qualifier_list(); opt(abstract_declarator);
}

void postfix_expression();

void unary_expression();

void cast_expression() {
  oneof({
    [](){ unary_expression(); },
    [](){ emit("("); type_name(); emit(")"); cast_expression(); },
  });
}

void unary_operator() {
  oneof({"&", "*", "+", "-", "~", "!"});
}

void unary_expression() {
  oneof({
    [](){ postfix_expression(); },
    [](){ emit("++"); unary_expression(); },
    [](){ emit("--"); unary_expression(); },
    [](){ unary_operator(); cast_expression(); },
    [](){ emit("sizeof "); unary_expression(); },
    [](){ emit("sizeof("); type_name(); emit(")"); },
  });
}

void argument_expression_list() {
  oneof({
    [](){ assignment_expression(); },
    [](){ argument_expression_list(); emit(","); assignment_expression(); },
  });
}

void designator() {
  oneof({
    [](){ emit("["); constant_expression(); emit("]"); },
    [](){ emit("."); identifier(); },
  });
}

void designator_list() {
  oneof({
    [](){ designator(); },
    [](){ designator_list(); designator(); },
  });
}

void designation() {
  designator_list(); emit("=");
}

void initializer();

void initializer_list() {
  oneof({
    [](){ opt(designation); initializer(); },
    [](){ initializer_list(); emit(","); opt(designation); initializer(); },
  });
}

void initializer() {
  oneof({
    [](){ assignment_expression(); },
    [](){ assignment_expression(); },
    [](){ assignment_expression(); },
    [](){ assignment_expression(); },
    [](){ emit("{"); initializer_list(); emit("}"); },
    [](){ emit("{"); initializer_list(); emit(","); emit("}");},
  });
}

void postfix_expression() {
  oneof({
    [](){ primary_expression(); },
    [](){ primary_expression(); },
    [](){ primary_expression(); },
    [](){ primary_expression(); },
    [](){ primary_expression(); },
    [](){ primary_expression(); },
    [](){ primary_expression(); },
    [](){ primary_expression(); },
    [](){ primary_expression(); },
    [](){ primary_expression(); },
    [](){ postfix_expression(); emit("[");  expression(); emit("]"); },
    [](){ postfix_expression(); emit("(");  opt(argument_expression_list); emit(")"); },
    [](){ postfix_expression(); emit(".");  identifier(); },
    [](){ postfix_expression(); emit("->"); identifier(); },
    [](){ postfix_expression(); emit("++"); },
    [](){ postfix_expression(); emit("--"); },
    [](){ emit("("); type_name(); emit(") {"); initializer_list(); emit("}"); },
    [](){ emit("("); type_name(); emit(") {"); initializer_list(); emit(",}"); },
  });
}

void expression() {
  FIX_FUNC;
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("cgen\n");
  for (int i = 0; i < 100; i++) {
    postfix_expression();
    printf("\n");
  }
  return 0;
}
