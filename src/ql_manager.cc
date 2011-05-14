//
// ql_manager.cc
//

#include <cstdio>
#include <iostream>
#include <algorithm>
#include <sys/times.h>
#include <sys/types.h>
#include <cassert>
#include <unistd.h>
#include "redbase.h"
#include "ql.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"

using namespace std;

namespace {
  bool strlt (char* i,char* j) { return (strcmp(i,j) < 0); }
  bool streq (char* i,char* j) { return (strcmp(i,j) == 0); }
};
//
// Constructor for the QL Manager
//
QL_Manager::QL_Manager(SM_Manager &smm_, IX_Manager &ixm_, RM_Manager &rmm_)
  :rmm(rmm_), ixm(ixm_), smm(smm_)
{
  
}

// Users will call - RC invalid = IsValid(); if(invalid) return invalid; 
RC QL_Manager::IsValid () const
{
  bool ret = true;
  ret = ret && (smm.IsValid() == 0);
  return ret ? 0 : QL_BADOPEN;
}

//
// QL_Manager::~QL_Manager()
//
// Destructor for the QL Manager
//
QL_Manager::~QL_Manager()
{
}

//
// Handle the select clause
//
RC QL_Manager::Select(int nSelAttrs, const RelAttr selAttrs_[],
                      int nRelations, const char * const relations_[],
                      int nConditions, const Condition conditions_[])
{
  RC invalid = IsValid(); if(invalid) return invalid;
  int i;

  // copies for rewrite
  RelAttr* selAttrs = new RelAttr[nSelAttrs];
  for (i = 0; i < nSelAttrs; i++) {
    selAttrs[i] = selAttrs_[i];
  }
  

  char** relations = new char*[nRelations];
  for (i = 0; i < nRelations; i++) {
    // strncpy(relations[i], relations_[i], MAXNAME);
    relations[i] = strdup(relations_[i]);
  }

  Condition* conditions = new Condition[nConditions];
  for (i = 0; i < nConditions; i++) {
    conditions[i] = conditions_[i];
  }


  for (i = 0; i < nRelations; i++) {
    RC rc = smm.SemCheck(relations[i]);
    if (rc != 0) return rc;
  }


  sort(relations, 
       relations + nRelations,
       strlt);

  char** dup = adjacent_find(relations,
                             relations + nRelations,
                             streq);
  if(dup != (relations + nRelations))
    return QL_DUPREL;

  // rewrite select *
  bool SELECTSTAR = false;
  if(nSelAttrs == 1 && strcmp(selAttrs[0].attrName, "*") == 0) {
    SELECTSTAR = true;
    nSelAttrs = 0;
    for (int i = 0; i < nRelations; i++) {
      int ac;
      DataAttrInfo * aa;
      RC rc = smm.GetFromTable(relations[i], ac, aa);
      if (rc != 0) return rc;
      nSelAttrs += ac;
      delete aa;
    }
    
    selAttrs = new RelAttr[nSelAttrs];
    int j = 0;
    for (int i = 0; i < nRelations; i++) {
      int ac;
      DataAttrInfo * aa;
      RC rc = smm.GetFromTable(relations[i], ac, aa);
      if (rc != 0) return rc;
      for (int k = 0; k < ac; k++) {
        selAttrs[j].attrName = strdup(aa[k].attrName);
        selAttrs[j].relName = relations[i];
        j++;
      }
      delete aa;
    }
  } // if rewrite select "*"

  for (i = 0; i < nSelAttrs; i++) {
    if(selAttrs[i].relName == NULL) {
      RC rc = smm.FindRelForAttr(selAttrs[i], nRelations, relations);
      if (rc != 0) return rc;
    }
    RC rc = smm.SemCheck(selAttrs[i]);
    if (rc != 0) return rc;
  }
  
  for (i = 0; i < nConditions; i++) {
    if(conditions[i].lhsAttr.relName == NULL) {
      RC rc = smm.FindRelForAttr(conditions[i].lhsAttr, nRelations, relations);
      if (rc != 0) return rc;
    }
    RC rc = smm.SemCheck(conditions[i].lhsAttr);
    if (rc != 0) return rc;
    
    if(conditions[i].bRhsIsAttr == TRUE) {
      if(conditions[i].rhsAttr.relName == NULL) {
        RC rc = smm.FindRelForAttr(conditions[i].rhsAttr, nRelations, relations);
        if (rc != 0) return rc;
      }
      RC rc = smm.SemCheck(conditions[i].rhsAttr);
      if (rc != 0) return rc;
    }

    rc = smm.SemCheck(conditions[i]);
    if (rc != 0) return rc;
  }


  cout << "Select\n";

  cout << "   nSelAttrs = " << nSelAttrs << "\n";
  for (i = 0; i < nSelAttrs; i++)
    cout << "   selAttrs[" << i << "]:" << selAttrs[i] << "\n";

  cout << "   nRelations = " << nRelations << "\n";
  for (i = 0; i < nRelations; i++)
    cout << "   relations[" << i << "] " << relations[i] << "\n";

  cout << "   nCondtions = " << nConditions << "\n";
  for (i = 0; i < nConditions; i++)
    cout << "   conditions[" << i << "]:" << conditions[i] << "\n";



  if(SELECTSTAR)
    for(int i = 0; i < nSelAttrs; i++)
      free(selAttrs[i].attrName);
  delete [] selAttrs;
  for (i = 0; i < nRelations; i++) {
    free(relations[i]);
  }
  delete [] relations;
  delete [] conditions;
  return 0;
}

//
// Insert the values into relName
//
RC QL_Manager::Insert(const char *relName,
                      int nValues, const Value values[])
{
  RC invalid = IsValid(); if(invalid) return invalid;

  RC rc = smm.SemCheck(relName);
  if (rc != 0) return rc;

  int attrCount;
  DataAttrInfo* attr;
  rc = smm.GetFromTable(relName, attrCount, attr);

  if(nValues != attrCount) {
    delete [] attr;
    return QL_INVALIDSIZE;
  }

  for(int i =0; i < nValues; i++) {
    if(values[i].type != attr[i].attrType) {
      delete [] attr;
      return QL_JOINKEYTYPEMISMATCH;
    }
  }

  delete [] attr;

  int i;

  cout << "Insert\n";

  cout << "   relName = " << relName << "\n";
  cout << "   nValues = " << nValues << "\n";
  for (i = 0; i < nValues; i++)
    cout << "   values[" << i << "]:" << values[i] << "\n";

  return 0;
}

//
// Delete from the relName all tuples that satisfy conditions
//
RC QL_Manager::Delete(const char *relName_,
                      int nConditions, const Condition conditions_[])
{
  RC invalid = IsValid(); if(invalid) return invalid;

  char relName[MAXNAME];
  strncpy(relName, relName_, MAXNAME);

  RC rc = smm.SemCheck(relName);
  if (rc != 0) return rc;

  Condition* conditions = new Condition[nConditions];
  for (int i = 0; i < nConditions; i++) {
    conditions[i] = conditions_[i];
  }

  for (int i = 0; i < nConditions; i++) {
    if(conditions[i].lhsAttr.relName == NULL) {
      conditions[i].lhsAttr.relName = relName;
    }
    if(strcmp(conditions[i].lhsAttr.relName, relName) != 0) {
      delete [] conditions;
      return QL_BADATTR;
    }

    RC rc = smm.SemCheck(conditions[i].lhsAttr);
    if (rc != 0) return rc;
    
    if(conditions[i].bRhsIsAttr == TRUE) {
      if(conditions[i].rhsAttr.relName == NULL) {
        conditions[i].rhsAttr.relName = relName;
      }
      if(strcmp(conditions[i].rhsAttr.relName, relName) != 0) {
        delete [] conditions;
        return QL_BADATTR;
      }

      RC rc = smm.SemCheck(conditions[i].rhsAttr);
      if (rc != 0) return rc;
    }

    rc = smm.SemCheck(conditions[i]);
    if (rc != 0) return rc;
  }


  int i;

  cout << "Delete\n";

  cout << "   relName = " << relName << "\n";
  cout << "   nCondtions = " << nConditions << "\n";
  for (i = 0; i < nConditions; i++)
    cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

  delete [] conditions;
  return 0;
}


//
// Update from the relName all tuples that satisfy conditions
//
RC QL_Manager::Update(const char *relName_,
                      const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue,
                      int nConditions, const Condition conditions_[])
{
  RC invalid = IsValid(); if(invalid) return invalid;

  char relName[MAXNAME];
  strncpy(relName, relName_, MAXNAME);

  RC rc = smm.SemCheck(relName);
  if (rc != 0) return rc;

  Condition* conditions = new Condition[nConditions];
  for (int i = 0; i < nConditions; i++) {
    conditions[i] = conditions_[i];
  }

  RelAttr ra;
  ra.relName = relName;
  ra.attrName = updAttr.attrName;
  rc = smm.SemCheck(ra);
  if (rc != 0) return rc;

  Condition cond;
  cond.lhsAttr = ra;
  cond.bRhsIsAttr = (bIsValue == TRUE ? FALSE : TRUE);
  cond.rhsAttr.attrName = rhsRelAttr.attrName;
  cond.rhsAttr.relName = relName;
  cond.op = EQ_OP;
  cond.rhsValue.type = rhsValue.type;
  cond.rhsValue.data = rhsValue.data;

  if (bIsValue != TRUE) {
    ra.attrName = rhsRelAttr.attrName;
    rc = smm.SemCheck(ra);
    if (rc != 0) return rc;
  }

  rc = smm.SemCheck(cond);
  if (rc != 0) return rc;
  
  for (int i = 0; i < nConditions; i++) {
    if(conditions[i].lhsAttr.relName == NULL) {
      conditions[i].lhsAttr.relName = relName;
    }
    if(strcmp(conditions[i].lhsAttr.relName, relName) != 0) {
      delete [] conditions;
      return QL_BADATTR;
    }

    RC rc = smm.SemCheck(conditions[i].lhsAttr);
    if (rc != 0) return rc;
    
    if(conditions[i].bRhsIsAttr == TRUE) {
      if(conditions[i].rhsAttr.relName == NULL) {
        conditions[i].rhsAttr.relName = relName;
      }
      if(strcmp(conditions[i].rhsAttr.relName, relName) != 0) {
        delete [] conditions;
        return QL_BADATTR;
      }

      RC rc = smm.SemCheck(conditions[i].rhsAttr);
      if (rc != 0) return rc;
    }

    rc = smm.SemCheck(conditions[i]);
    if (rc != 0) return rc;
  }

  int i;

  cout << "Update\n";

  cout << "   relName = " << relName << "\n";
  cout << "   updAttr:" << updAttr << "\n";
  if (bIsValue)
    cout << "   rhs is value: " << rhsValue << "\n";
  else
    cout << "   rhs is attribute: " << rhsRelAttr << "\n";

  cout << "   nCondtions = " << nConditions << "\n";
  for (i = 0; i < nConditions; i++)
    cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

  return 0;
}
