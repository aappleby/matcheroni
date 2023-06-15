#include "Node.h"
#include <stdio.h>

void NodeStack::push(NodeBase* n) {
  assert(_stack[_top] == nullptr);
  _stack[_top] = n;
  _top++;

  static int max_top = 0;
  if (_top > max_top) {
    printf("max top %d\n", _top);
    max_top = _top;
  }
}

NodeBase* NodeStack::pop() {
  assert(_top);
  _top--;
  NodeBase* result = _stack[_top];
  _stack[_top] = nullptr;
  return result;
}

NodeBase* NodeStack::back() { return _stack[_top - 1]; }

size_t NodeStack::top() const { return _top; }

void NodeStack::dump_top() {
  _stack[_top-1]->dump_tree();
}

void NodeStack::clear_to(size_t new_top) {
  while(_top > new_top) {
    delete pop();
  }
}

void NodeStack::pop_to(size_t new_top) {
  while (_top > new_top) pop();
}

NodeStack NodeBase::node_stack;
