#include "ParseNode.h"
#include <stdio.h>

void ParseNodeStack::push(ParseNode* n) {
  if (_top == stack_size) {
    printf("Node stack overflow, size %d\n", stack_size);
    exit(1);
  }

  assert(_stack[_top] == nullptr);
  _stack[_top] = n;
  _top++;
}

ParseNode* ParseNodeStack::pop() {
  assert(_top);
  _top--;
  ParseNode* result = _stack[_top];
  _stack[_top] = nullptr;
  return result;
}

ParseNode* ParseNodeStack::back() { return _stack[_top - 1]; }

size_t ParseNodeStack::top() const { return _top; }

void ParseNodeStack::dump_top() {
  _stack[_top-1]->dump_tree();
}

void ParseNodeStack::clear_to(size_t new_top) {
  while(_top > new_top) {
    delete pop();
  }
}

void ParseNodeStack::pop_to(size_t new_top) {
  while (_top > new_top) pop();
}

ParseNodeStack ParseNode::node_stack;
