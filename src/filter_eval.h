#ifndef FILTER_EVAL_H
#define FILTER_EVAL_H

#include <ostream>
#include <cstring>
#include "redbase.h"
#include "parser.h"   // Condition
#include "sm.h"       // SM_Manager, DataAttrInfo

struct FilterMeta {
  AttrType type;
  int length;
  int offset;
  CompOp op;
  bool rhsIsAttr;
  int rhsOffset;         // valid iff rhsIsAttr
  const void* rhsValue;  // valid iff !rhsIsAttr
};

class FilterEvaluator {
public:
  FilterEvaluator()
    : n(0), filters(NULL), meta(NULL), psmm(NULL), relName(NULL) {}

  ~FilterEvaluator() {
    delete [] filters;
    delete [] meta;
  }

  RC init(SM_Manager* smm,
          const char* rel,
          int nOutFilters,
          const Condition outFilters[]) {
    psmm = smm;
    relName = rel;
    n = (nOutFilters > 0 && outFilters != NULL) ? nOutFilters : 0;
    if (n == 0) {
      filters = NULL;
      meta = NULL;
      return 0;
    }
    filters = new Condition[n];
    for (int i = 0; i < n; i++) filters[i] = outFilters[i];
    meta = new FilterMeta[n];
    for (int i = 0; i < n; i++) {
      DataAttrInfo lhs; RID tmp;
      RC rc = psmm->GetAttrFromCat(relName, filters[i].lhsAttr.attrName, lhs, tmp);
      if (rc != 0) return rc;
      meta[i].type = lhs.attrType;
      meta[i].length = lhs.attrLength;
      meta[i].offset = lhs.offset;
      meta[i].op = filters[i].op;
      meta[i].rhsIsAttr = (filters[i].bRhsIsAttr == TRUE);
      if (meta[i].rhsIsAttr) {
        DataAttrInfo rhs;
        rc = psmm->GetAttrFromCat(relName, filters[i].rhsAttr.attrName, rhs, tmp);
        if (rc != 0) return rc;
        meta[i].rhsOffset = rhs.offset;
        meta[i].rhsValue = NULL;
      } else {
        meta[i].rhsOffset = -1;
        meta[i].rhsValue = filters[i].rhsValue.data;
      }
    }
    return 0;
  }

  bool empty() const { return n == 0; }
  int count() const { return n; }

  bool passes(const char* buf) const {
    for (int i = 0; i < n; i++) {
      const FilterMeta &m = meta[i];
      Predicate p(m.type,
                  m.length,
                  m.offset,
                  m.op,
                  m.rhsIsAttr ? NULL : (void*)m.rhsValue,
                  NO_HINT);
      const char* rhs = m.rhsIsAttr ? (buf + m.rhsOffset)
                                    : (const char*)m.rhsValue;
      if (!p.eval(buf, rhs, m.op)) return false;
    }
    return true;
  }

  void explainTo(std::ostream& os) const {
    if (n <= 0) return;
    os << "   nFilters = " << n << "\n";
    for (int i = 0; i < n; i++) {
      os << "   filters[" << i << "]:" << filters[i] << "\n";
    }
  }

private:
  int n;
  Condition* filters;
  FilterMeta* meta;
  SM_Manager* psmm;
  const char* relName;
};

#endif // FILTER_EVAL_H


