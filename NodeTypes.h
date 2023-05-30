#include "Node.h"
#include <vector>

struct TranslationUnit : public Node {
  TranslationUnit(Lexeme* lex_a, Lexeme* lex_b) : Node(NODE_TRANSLATION_UNIT, lex_a, lex_b) {}

  std::vector<Node*> decls;
};
