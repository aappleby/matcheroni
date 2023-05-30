// Preprocessor macros generally won't work, but include guards should be OK.

#ifndef INCLUDE_GUARDS_H
#define INCLUDE_GUARDS_H

class Module {
public:
  int blah() {
    return 7;
  }
};

#endif
