#include "c99_parser.h"

#include "Node.h"

#ifdef MATCHERONI_USE_NAMESPACE
using namespace matcheroni;
#endif

//------------------------------------------------------------------------------

struct NodePreproc : public NodeMaker<NodePreproc> {
  using pattern = Atom<TOK_PREPROC>;
};

//------------------------------------------------------------------------------

template<typename P>
struct ProgressBar {
  static const Token* match(const Token* a, const Token* b) {
    printf("%.40s\n", a->lex->span_a);
    return P::match(a, b);
  }
};

struct NodeTranslationUnit : public NodeMaker<NodeTranslationUnit> {
  using pattern = Any<
    ProgressBar<
      Oneof<
        Ref<parse_preproc>,
        Ref<parse_declaration>,
        Ref<parse_definition>
      >
    >
  >;
};
