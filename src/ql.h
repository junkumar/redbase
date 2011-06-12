//
// ql.h
//   Query Language Component Interface
//

// This file only gives the stub for the QL component

#ifndef QL_H
#define QL_H

#include <stdlib.h>
#include <string.h>
#include "redbase.h"
#include "parser.h"
#include "ql_error.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"
#include "iterator.h"

//
// QL_Manager: query language (DML)
//
class QL_Manager {
 public:
  QL_Manager (SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm);
  ~QL_Manager();                               // Destructor

  RC Select  (int nSelAttrs,                   // # attrs in select clause
              //              const RelAttr selAttrs[],        // attrs in select clause
              const AggRelAttr selAttrs[],     // attrs in select clause              
              int   nRelations,                // # relations in from clause
              const char * const relations[],  // relations in from clause
              int   nConditions,               // # conditions in where clause
              const Condition conditions[],    // conditions in where clause
              int order,                       // order from order by clause
              RelAttr orderAttr,               // the single attr ordered by
              bool group,
              RelAttr groupAttr);

  RC Insert  (const char *relName,             // relation to insert into
              int   nValues,                   // # values
              const Value values[]);           // values to insert

  RC Delete  (const char *relName,             // relation to delete from
              int   nConditions,               // # conditions in where clause
              const Condition conditions[]);   // conditions in where clause

  RC Update  (const char *relName,             // relation to update
              const RelAttr &updAttr,          // attribute to update
              const int bIsValue,              // 1 if RHS is a value, 0 if attr
              const RelAttr &rhsRelAttr,       // attr on RHS to set LHS eq to
              const Value &rhsValue,           // or value to set attr eq to
              int   nConditions,               // # conditions in where clause
              const Condition conditions[]);   // conditions in where clause
 public:
  RC IsValid() const;
  // Choose between filescan and indexscan for first operation - leaf level of
  // operator tree
  // to see if NLIJ is possible, join condition is passed down
  Iterator* GetLeafIterator(const char *relName,
                            int nConditions, 
                            const Condition conditions[],
                            int nJoinConditions = 0,
                            const Condition jconditions[] = NULL,
                            int order = 0,
                            RelAttr* porderAttr = NULL);

  RC MakeRootIterator(Iterator*& newit,
                      int nSelAttrs, const AggRelAttr selAttrs[],
                      int nRelations, const char * const relations[],
                      int order, RelAttr orderAttr,
                      bool group, RelAttr groupAttr) const;

  RC PrintIterator(Iterator* it) const;

  void GetCondsForSingleRelation(int nConditions,
                                 Condition conditions[],
                                 char* relName,
                                 int& retCount, Condition*& retConds) const;

  // get conditions that involve both relations. intermediate relations are
  // possible from previous joins done so far - hence relsSoFar.
  void GetCondsForTwoRelations(int nConditions,
                               Condition conditions[],
                               int nRelsSoFar,
                               char* relations[],
                               char* relName2,
                               int& retCount, Condition*& retConds) const;
  
 private:
  RM_Manager& rmm;
  IX_Manager& ixm;
  SM_Manager& smm;
};

#endif // QL_H
