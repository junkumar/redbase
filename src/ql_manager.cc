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
#include "iterator.h"
#include "index_scan.h"
#include "file_scan.h"
#include "nested_loop_join.h"
#include "nested_loop_index_join.h"
#include "nested_block_join.h"
#include "merge_join.h"
#include "sort.h"
#include "agg.h"
#include "parser.h"
#include "projection.h"
#include <map>
#include <vector>
#include <functional>

using namespace std;

namespace {
  bool strlt (char* i, char* j) { return (strcmp(i,j) < 0); }
  bool streq (char* i, char* j) { return (strcmp(i,j) == 0); }

  class npageslt : public std::binary_function<char*, char*, bool>
  {
  public:
    npageslt(const SM_Manager& smm): psmm(&smm) {}
    inline bool operator() (char* i, char* j) {
      return (psmm->GetNumPages(i) < psmm->GetNumPages(j));
    }
  private:
    const SM_Manager* psmm;
  };

  class nrecslt : public std::binary_function<char*, char*, bool>
  {
  public:
    nrecslt(const SM_Manager& smm): psmm(&smm) {}
    inline bool operator() (char* i, char* j) {
      return (psmm->GetNumRecords(i) < psmm->GetNumRecords(j));
    }
  private:
    const SM_Manager* psmm;
  };

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
//RC QL_Manager::Select(int nSelAttrs, const RelAttr selAttrs_[],
RC QL_Manager::Select(int nSelAttrs, const AggRelAttr selAttrs_[],
                      int nRelations, const char * const relations_[],
                      int nConditions, const Condition conditions_[],
                      int order, RelAttr orderAttr,
                      bool group, RelAttr groupAttr)
{
  RC invalid = IsValid(); if(invalid) return invalid;
  int i;

  // copies for rewrite
  RelAttr* selAttrs = new RelAttr[nSelAttrs];
  for (i = 0; i < nSelAttrs; i++) {
    selAttrs[i].relName = selAttrs_[i].relName;
    selAttrs[i].attrName = selAttrs_[i].attrName;
  }
  
  AggRelAttr* selAggAttrs = new AggRelAttr[nSelAttrs];
  for (i = 0; i < nSelAttrs; i++) {
    selAggAttrs[i].func = selAttrs_[i].func;
    selAggAttrs[i].relName = selAttrs_[i].relName;
    selAggAttrs[i].attrName = selAttrs_[i].attrName;
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
      delete [] aa;
    }
    
    delete [] selAttrs;
    delete [] selAggAttrs;
    selAttrs = new RelAttr[nSelAttrs];
    selAggAttrs = new AggRelAttr[nSelAttrs];
    int j = 0;
    for (int i = 0; i < nRelations; i++) {
      int ac;
      DataAttrInfo * aa;
      RC rc = smm.GetFromTable(relations[i], ac, aa);
      if (rc != 0) return rc;
      for (int k = 0; k < ac; k++) {
        selAttrs[j].attrName = strdup(aa[k].attrName);
        selAttrs[j].relName = relations[i];
        selAggAttrs[j].attrName = strdup(aa[k].attrName);
        selAggAttrs[j].relName = relations[i];
        selAggAttrs[j].func = NO_F;
        j++;
      }
      delete [] aa;
    }
  } // if rewrite select "*"

  if(order != 0) {
    RC rc = smm.FindRelForAttr(orderAttr, nRelations, relations);
    if(rc != 0) return rc;
    rc = smm.SemCheck(orderAttr);
    if(rc != 0) return rc;
  }

  if(group) {
    RC rc = smm.FindRelForAttr(groupAttr, nRelations, relations);
    if(rc != 0) return rc;
    rc = smm.SemCheck(groupAttr);
    if(rc != 0) return rc;
  } else {
    // make sure no agg functions are defined
    for (i = 0; i < nSelAttrs; i++) {
      if(selAggAttrs[i].func != NO_F)
        return SM_BADAGGFUN;
    }
  }

  // rewrite select COUNT(*)
  for (i = 0; i < nSelAttrs; i++) {
    if(strcmp(selAggAttrs[i].attrName, "*") == 0 && 
       selAggAttrs[i].func == COUNT_F) {
      selAggAttrs[i].attrName = strdup(groupAttr.attrName);
      selAggAttrs[i].relName = strdup(groupAttr.relName);
      selAttrs[i].attrName = strdup(groupAttr.attrName);
      selAttrs[i].relName = strdup(groupAttr.relName);
      // cout << "rewriting select count(*) " << selAggAttrs[i] << endl;
    }
  }

  for (i = 0; i < nSelAttrs; i++) {
    if(selAttrs[i].relName == NULL) 
    {
      RC rc = smm.FindRelForAttr(selAttrs[i], nRelations, relations);
      if (rc != 0) return rc;
    } else {
      selAttrs[i].relName = strdup(selAttrs[i].relName);
    }
    selAggAttrs[i].relName = strdup(selAttrs[i].relName);
    RC rc = smm.SemCheck(selAttrs[i]);
    if (rc != 0) return rc;
    rc = smm.SemCheck(selAggAttrs[i]);
    if (rc != 0) return rc;
  }
  
  for (i = 0; i < nConditions; i++) {
    if(conditions[i].lhsAttr.relName == NULL) {
      RC rc = smm.FindRelForAttr(conditions[i].lhsAttr, nRelations, relations);
      if (rc != 0) return rc;
    } else {
      conditions[i].lhsAttr.relName = strdup(conditions[i].lhsAttr.relName);
    }
    RC rc = smm.SemCheck(conditions[i].lhsAttr);
    if (rc != 0) return rc;
    
    if(conditions[i].bRhsIsAttr == TRUE) {
      if(conditions[i].rhsAttr.relName == NULL) {
        RC rc = smm.FindRelForAttr(conditions[i].rhsAttr, nRelations, relations);
        if (rc != 0) return rc;
      } else {
        conditions[i].rhsAttr.relName = strdup(conditions[i].rhsAttr.relName);
      }
      RC rc = smm.SemCheck(conditions[i].rhsAttr);
      if (rc != 0) return rc;
    }

    rc = smm.SemCheck(conditions[i]);
    if (rc != 0) return rc;
  }
  
  // ensure that all relations mentioned in conditions are in the from clause
  for (int i = 0; i < nConditions; i++) {
    bool lfound = false;
    for (int j = 0; j < nRelations; j++) {
      if(strcmp(conditions[i].lhsAttr.relName, relations[j]) == 0) {
        lfound = true;
        break;
      }
    }
    if(!lfound) 
      return QL_RELMISSINGFROMFROM;

    if(conditions[i].bRhsIsAttr == TRUE) {
      bool rfound = false;
      for (int j = 0; j < nRelations; j++) {
        if(strcmp(conditions[i].rhsAttr.relName, relations[j]) == 0) {
          rfound = true;
          break;
        }
      }
      if(!rfound) 
        return QL_RELMISSINGFROMFROM;
    }
  }

  // cerr << "sem checks done" << endl;

  Iterator* it = NULL;

  if(nRelations == 1) {
    it = GetLeafIterator(relations[0], nConditions, conditions, 0, NULL, order, &orderAttr);
    RC rc = MakeRootIterator(it, nSelAttrs, selAggAttrs, nRelations, relations,
                             order, orderAttr, group, groupAttr);
    if(rc != 0) return rc;
    rc = PrintIterator(it);
    if(rc != 0) return rc;
  }

  if(nRelations >= 2) {
    // Heuristic - join smaller operands first - sort relations by numRecords
    nrecslt _n(smm);
    sort(relations, 
         relations + nRelations,
         _n);

    // Heuristic - left-deep join tree shape
    Condition* lcond = NULL;
    int lcount = -1;
    GetCondsForSingleRelation(nConditions, conditions, relations[0], lcount,
                              lcond);
    it = GetLeafIterator(relations[0], lcount, lcond, 0, NULL, order, &orderAttr);
    if(lcount != 0) delete [] lcond;
    
    for(int i = 1; i < nRelations; i++) {
      Condition* jcond = NULL;
      int jcount = 0;
      GetCondsForTwoRelations(nConditions, conditions, i, relations, relations[i],
                              jcount, jcond);


      Condition* rcond = NULL;
      int rcount = -1;
      GetCondsForSingleRelation(nConditions, conditions, relations[i], rcount,
                                rcond);
      Iterator* rfs = GetLeafIterator(relations[i], rcount, rcond, jcount,
                                      jcond, order, &orderAttr);
      if(rcount != 0) delete [] rcond;

      Iterator* newit = NULL;

      if(i == 1) {
        FileScan* fit = dynamic_cast<FileScan*>(it);
        RC status = -1;
        if(fit != NULL)
          newit = new NestedBlockJoin(fit, rfs, status, jcount, jcond);
        else
          newit = new NestedLoopJoin(it, rfs, status, jcount, jcond);
        if (status != 0) return status;
      }


      IndexScan* rixit = dynamic_cast<IndexScan*>(rfs);
      IndexScan* lixit = NULL;

      // flag to see if index merge join is possible
      int indexMergeCond = -1;
      // look for equijoin (addl conditions are ok)
      for(int k = 0; k < jcount; k++) {
        if((jcond[k].op == EQ_OP) && 
           (rixit != NULL) && 
           (strcmp(rixit->GetIndexAttr().c_str(), jcond[k].lhsAttr.attrName) == 0
            || strcmp(rixit->GetIndexAttr().c_str(), jcond[k].rhsAttr.attrName) ==
            0)) {
             indexMergeCond = k;
             break;
           }
      }

      string mj("");
      smm.Get("mergejoin", mj);
      if(mj == "no")
        indexMergeCond = -1;


      if(indexMergeCond > -1 && i == 1) {
        Condition* lcond = NULL;
        int lcount = -1;
        GetCondsForSingleRelation(nConditions, conditions, relations[0], lcount,
                                  lcond);
   
        delete it;
        it = GetLeafIterator(relations[0], lcount, lcond, jcount, jcond, order, &orderAttr);
        if(lcount != 0) delete [] lcond;

        lixit = dynamic_cast<IndexScan*>(it);

        if((lixit == NULL) ||
           (strcmp(lixit->GetIndexAttr().c_str(), jcond[indexMergeCond].lhsAttr.attrName) != 0
            && strcmp(lixit->GetIndexAttr().c_str(), jcond[indexMergeCond].rhsAttr.attrName) !=
            0)) {
          indexMergeCond = -1;
          cerr << "null lixit" << endl;
        }

        if(lixit->IsDesc() != rixit->IsDesc()) {
          indexMergeCond = -1;
          cerr << "order mismatch" << endl;
        }
      }

      bool nlijoin = true;
      string nlij("");
      smm.Get("nlij", nlij);
      if(nlij == "no")
        nlijoin = false;

      if(indexMergeCond > -1 && i == 1) { //both have to be indexscans
        RC status = -1;
        newit = new MergeJoin(lixit, rixit, status, jcount, indexMergeCond, jcond);
        if (status != 0) return status;
      } else {
        if(rixit != NULL && nlijoin) {
          RC status = -1;
          newit = new NestedLoopIndexJoin(it, rixit, status, jcount, jcond);
          if (status != 0) return status;
        }  else {
          if(newit == NULL) {
            RC status = -1;
            newit = new NestedLoopJoin(it, rfs, status, jcount, jcond);
            if (status != 0) return status;
          }
        }
      }

      // cout << "Select done with NBJ init\n";

      if(i == nRelations - 1) {
        RC rc = MakeRootIterator(newit, nSelAttrs, selAggAttrs, nRelations, relations,
                                 order, orderAttr, group, groupAttr);
        if(rc != 0) return rc;
      }

      if(jcount != 0) delete [] jcond;

      it = newit;
    }

    // cout << "Select done with for relations\n";

    RC rc = PrintIterator(it);
    if(rc != 0) return rc;
  }

  // cout << "Select\n";

  // cout << "   nSelAttrs = " << nSelAttrs << "\n";
  // for (int i = 0; i < nSelAttrs; i++)
  //   cout << "   selAttrs[" << i << "]:" << selAggAttrs[i] << "\n";

  // cout << "   nRelations = " << nRelations << "\n";
  // for (int i = 0; i < nRelations; i++)
  //   cout << "   relations[" << i << "] " << relations[i] << "\n";

  // cout << "   nConditions = " << nConditions << "\n";
  // for (int i = 0; i < nConditions; i++)
  //   cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

  // if(order != 0) 
  //   cout << "   orderAttr:" << orderAttr 
  //        << ((order == -1) ? " DESC" : " ASC")
  //        << "\n";

  // if(group) 
  //   cout << "   groupAttr:" << groupAttr 
  //        << "\n";

  // recursively delete iterators
  delete it;

  for(int i = 0; i < nSelAttrs; i++) {
    if(SELECTSTAR) {
      free(selAttrs[i].attrName);
      free(selAggAttrs[i].attrName);
    }
    free(selAttrs[i].relName);
    free(selAggAttrs[i].relName);
  }
  delete [] selAttrs;
  delete [] selAggAttrs;
  for (i = 0; i < nRelations; i++) {
    free(relations[i]);
  }
  delete [] relations;
  for(int i = 0; i < nConditions; i++) {
    free(conditions[i].lhsAttr.relName);
    if(conditions[i].bRhsIsAttr == TRUE)
      free(conditions[i].rhsAttr.relName);
  }
  delete [] conditions;
  // TODO - free() the FindRelForAttr() result in orderAttr and groupAttr
  // relName is only filled in optionally
  return 0;
}

RC QL_Manager::MakeRootIterator(Iterator*& newit,
                                int nSelAttrs, const AggRelAttr selAttrs[],
                                int nRelations, const char * const relations[],
                                int order, RelAttr orderAttr,
                                bool group, RelAttr groupAttr) const
{
  RC status = -1;

  if(order != 0) {
    RC rc = smm.FindRelForAttr(orderAttr, nRelations, relations);
    if(rc != 0) return rc;
    rc = smm.SemCheck(orderAttr);
    if(rc != 0) return rc;
  }

  if(group) {
    bool desc = (order == -1) ? true : false;
    RC rc = smm.FindRelForAttr(groupAttr, nRelations, relations);
    if(rc != 0) return rc;
    rc = smm.SemCheck(groupAttr);
    if(rc != 0) return rc;
    DataAttrInfo d;
    DataAttrInfo* pattr = newit->GetAttr();
    for(int i = 0; i < newit->GetAttrCount(); i++) {
      if(strcmp(pattr[i].relName, groupAttr.relName) == 0 &&
         strcmp(pattr[i].attrName, groupAttr.attrName) == 0)
        d = pattr[i];
    }

    if(newit->IsSorted() &&
       newit->IsDesc() == desc &&
       newit->GetSortRel() == string(groupAttr.relName) &&
       newit->GetSortAttr() == string(groupAttr.attrName)) {
      // cout << "already sorted - no need for sort operator - agg\n";
    } else {
      // cout << "Making sort iterator - agg\n";
      newit = new Sort(newit, d.attrType, d.attrLength, d.offset, status, desc);
      if(status != 0) return status;
    }

    AggRelAttr* extraAttrs = new AggRelAttr[nSelAttrs];
    for(int i = 0; i < nSelAttrs; i++) {
      extraAttrs[i] = selAttrs[i];
    }
    int nExtraSelAttrs = nSelAttrs;

    if(order != 0) {
      // add the sort column as a projection just in case
      delete [] extraAttrs;
      nExtraSelAttrs = nSelAttrs + 1;
      extraAttrs = new AggRelAttr[nExtraSelAttrs];
      AggRelAttr* extraAttrsNoF = new AggRelAttr[nExtraSelAttrs];

      for(int i = 0; i < nExtraSelAttrs-1; i++) {
        extraAttrs[i] = selAttrs[i];
        extraAttrsNoF[i] = selAttrs[i];
        extraAttrsNoF[i].func = NO_F;
      }
      extraAttrs[nExtraSelAttrs - 1].relName = strdup(orderAttr.relName);
      extraAttrs[nExtraSelAttrs - 1].attrName = strdup(orderAttr.attrName);
      extraAttrs[nExtraSelAttrs - 1].func = NO_F;
      extraAttrsNoF[nExtraSelAttrs - 1] = extraAttrs[nExtraSelAttrs - 1];

      // cout << "extraAttrs[new] " << extraAttrs[nExtraSelAttrs - 1] << endl;

      newit = new Projection(newit, status, nExtraSelAttrs, extraAttrsNoF);
      if(status != 0) return status;
    }

    newit = new Agg(newit, groupAttr, nExtraSelAttrs, extraAttrs, status);
    if(status != 0) return status;

    {
      //TODO makes project work
      newit->Explain();
    }

    // if(newit != NULL)
    //   cout << "Non null Agg newit" << endl;

  }

  if(order != 0) {
    bool desc = (order == -1) ? true : false;

    DataAttrInfo d;
    {
      DataAttrInfo* pattr = newit->GetAttr();
      newit->Explain();
      // cout << "\n" << newit->Explain() << "\n";
      for(int i = 0; i < newit->GetAttrCount(); i++) {
        if(strcmp(pattr[i].relName, orderAttr.relName) == 0 &&
           strcmp(pattr[i].attrName, orderAttr.attrName) == 0 &&
           pattr[i].func != MIN_F && 
           pattr[i].func != MAX_F &&
           pattr[i].func != COUNT_F
          )
          d = pattr[i];
        // cout << "pattr[" << i << "].relName was " << pattr[i].relName << endl;
        // cout << "pattr[" << i << "].attrName was " << pattr[i].attrName << endl;
        // cout << "pattr[" << i << "].func was " << int(pattr[i].func) << endl;
      }
    }
    // cout << "orderAttr.relName was " << orderAttr.relName << endl;
    // cout << "orderAttr.attrName was " << orderAttr.attrName << endl;
    // cout << "d.func was " << d.func << endl;
    // cout << "d.offset was " << d.offset << endl;
    // cout << "d.attrName was " << d.attrName << endl;

    if(newit->IsSorted() &&
       newit->IsDesc() == desc &&
       newit->GetSortRel() == string(orderAttr.relName) &&
       newit->GetSortAttr() == string(orderAttr.attrName)) {
      // cout << "already sorted - no need for sort operator\n";
    } else {
      // cout << "Making sort iterator\n";
      newit = new Sort(newit, d.attrType, d.attrLength, d.offset, status, desc);
      if(status != 0) return status;
    }
  }
  // cout << "Sort cons done" << endl;
      

  newit = new Projection(newit, status, nSelAttrs, selAttrs);
  if(status != 0) return status;
  return 0;
}

RC QL_Manager::PrintIterator(Iterator* it) const {
  if(bQueryPlans == TRUE)
    cout << "\n" << it->Explain() << "\n";
  
  Tuple t = it->GetTuple();
  RC rc = it->Open();
  if (rc != 0) return rc;

  Printer p(t);
  p.PrintHeader(cout);

  while(1) {
    rc = it->GetNext(t);
    // cout << "QL_Manager::PrintIterator tuple " << t << endl;

    if(rc ==  it->Eof())
      break;
    if (rc != 0) return rc;

    p.Print(cout, t);
  }

  p.PrintFooter(cout);
  rc = it->Close();
  if (rc != 0) return rc;
  return 0;
}

void QL_Manager::GetCondsForSingleRelation(int nConditions,
                                           Condition conditions[],
                                           char* relName,
                                           int& retCount, Condition*& retConds)
  const
{
  vector<int> v;

  for(int j = 0; j < nConditions; j++) {
    if(conditions[j].bRhsIsAttr == TRUE)
      continue;
    if(strcmp(conditions[j].lhsAttr.relName, relName) == 0) {
      v.push_back(j);
    }
  }
  
  retCount = v.size();
  if(retCount == 0) 
    return;
  retConds = new Condition[retCount];
  for(int i = 0; i < retCount; i++)
    retConds[i] = conditions[v[i]];
  return;
}

void QL_Manager::GetCondsForTwoRelations(int nConditions,
                                         Condition conditions[],
                                         int nRelsSoFar,
                                         char* relations[],
                                         char* relName2,
                                         int& retCount, Condition*& retConds)
  const
{
  vector<int> v;

  for(int i = 0; i < nRelsSoFar; i++) {
    char* relName1 = relations[i];
    for(int j = 0; j < nConditions; j++) {
      if(conditions[j].bRhsIsAttr == FALSE)
        continue;
      if(strcmp(conditions[j].lhsAttr.relName, relName1) == 0
         && strcmp(conditions[j].rhsAttr.relName, relName2) == 0) {
        v.push_back(j);
      }
      if(strcmp(conditions[j].lhsAttr.relName, relName2) == 0
         && strcmp(conditions[j].rhsAttr.relName, relName1) == 0) {
        v.push_back(j);
      }
    }
  }

  retCount = v.size();
  if(retCount == 0) 
    return;
  retConds = new Condition[retCount];
  for(int i = 0; i < retCount; i++)
    retConds[i] = conditions[v[i]];
  return;
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

  int size = 0;
  for(int i =0; i < nValues; i++) {
    if(values[i].type != attr[i].attrType) {
      delete [] attr;
      return QL_JOINKEYTYPEMISMATCH;
    }
    size += attr[i].attrLength;
  }

  char * buf = new char[size];
  int offset = 0;
  for(int i =0; i < nValues; i++) {
    assert(values[i].data != NULL);
    memcpy(buf + offset,
           values[i].data,
           attr[i].attrLength);
    offset += attr[i].attrLength;
  }

  rc = smm.LoadRecord(relName, size, buf);
  if (rc != 0) return rc;

  Printer p(attr, attrCount);
  p.PrintHeader(cout);
  p.Print(cout, buf);
  p.PrintFooter(cout);

  delete [] attr;
  delete [] buf;
  // cout << "Insert\n";

  // cout << "   relName = " << relName << "\n";
  // cout << "   nValues = " << nValues << "\n";
  // for (int i = 0; i < nValues; i++)
  //   cout << "   values[" << i << "]:" << values[i] << "\n";

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

  Iterator* it = GetLeafIterator(relName, nConditions, conditions);

  if(bQueryPlans == TRUE)
    cout << "\n" << it->Explain() << "\n";

  Tuple t = it->GetTuple();
  rc = it->Open();
  if (rc != 0) return rc;

  Printer p(t);
  p.PrintHeader(cout);

  RM_FileHandle fh;
  rc =	rmm.OpenFile(relName, fh);
  if (rc != 0) return rc;

  int attrCount = -1;
  DataAttrInfo * attributes;
  rc = smm.GetFromTable(relName, attrCount, attributes);
  if(rc != 0) return rc;
  IX_IndexHandle * indexes = new IX_IndexHandle[attrCount];
  for (int i = 0; i < attrCount; i++) {
    if(attributes[i].indexNo != -1) {
      ixm.OpenIndex(relName, attributes[i].indexNo, indexes[i]);
    }
  }

  while(1) {
    rc = it->GetNext(t);
    if(rc ==  it->Eof())
      break;
    if (rc != 0) return rc;

    rc = fh.DeleteRec(t.GetRid());
    if (rc != 0) return rc;

    for (int i = 0; i < attrCount; i++) {
      if(attributes[i].indexNo != -1) {
        void * pKey;
        t.Get(attributes[i].offset, pKey);
        indexes[i].DeleteEntry(pKey, t.GetRid());
      }
    }

    p.Print(cout, t);
  }

  p.PrintFooter(cout);
  
  for (int i = 0; i < attrCount; i++) {
    if(attributes[i].indexNo != -1) {
      RC rc = ixm.CloseIndex(indexes[i]);
      if(rc != 0 ) return rc;
    }
  }
  delete [] indexes;
  delete [] attributes;
  
  rc =	rmm.CloseFile(fh);
  if (rc != 0) return rc;
 
  // cout << "Delete\n";

  // cout << "   relName = " << relName << "\n";
  // cout << "   nCondtions = " << nConditions << "\n";
  // for (int i = 0; i < nConditions; i++)
  //   cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

  delete [] conditions;
  rc = it->Close();
  if (rc != 0) return rc;

  //delete it;
  //cerr << "done with delete it" << endl;
  return 0;
}


//
// Update from the relName all tuples that satisfy conditions
//
RC QL_Manager::Update(const char *relName_,
                      const RelAttr &updAttr_,
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

  RelAttr updAttr;
  updAttr.relName = relName;
  updAttr.attrName = updAttr_.attrName;
  rc = smm.SemCheck(updAttr);
  if (rc != 0) return rc;

  Condition cond;
  cond.lhsAttr = updAttr;
  cond.bRhsIsAttr = (bIsValue == TRUE ? FALSE : TRUE);
  cond.rhsAttr.attrName = rhsRelAttr.attrName;
  cond.rhsAttr.relName = relName;
  cond.op = EQ_OP;
  cond.rhsValue.type = rhsValue.type;
  cond.rhsValue.data = rhsValue.data;

  if (bIsValue != TRUE) {
    updAttr.attrName = rhsRelAttr.attrName;
    rc = smm.SemCheck(updAttr);
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

  Iterator* it;
  // handle halloween problem by not choosing indexscan on an attr when the attr
  // is the one being updated.
  if(smm.IsAttrIndexed(updAttr.relName, updAttr.attrName)) {
    // temporarily make attr unindexed
    rc = smm.DropIndexFromAttrCatAlone(updAttr.relName, updAttr.attrName);
    if (rc != 0) return rc;

    it = GetLeafIterator(relName, nConditions, conditions);

    rc = smm.ResetIndexFromAttrCatAlone(updAttr.relName, updAttr.attrName);
    if (rc != 0) return rc;
  } else {
    it = GetLeafIterator(relName, nConditions, conditions);
  }

  if(bQueryPlans == TRUE)
    cout << "\n" << it->Explain() << "\n";

  Tuple t = it->GetTuple();
  rc = it->Open();
  if (rc != 0) return rc;

  void * val = NULL;
  if(bIsValue == TRUE)
    val = rhsValue.data;
  else
    t.Get(rhsRelAttr.attrName, val);

  Printer p(t);
  p.PrintHeader(cout);

  RM_FileHandle fh;
  rc =	rmm.OpenFile(relName, fh);
  if (rc != 0) return rc;

  int attrCount = -1;
  int updAttrOffset = -1;
  DataAttrInfo * attributes;
  rc = smm.GetFromTable(relName, attrCount, attributes);
  if(rc != 0) return rc;
  IX_IndexHandle * indexes = new IX_IndexHandle[attrCount];
  for (int i = 0; i < attrCount; i++) {
    if(attributes[i].indexNo != -1 && 
       strcmp(attributes[i].attrName, updAttr.attrName) == 0) {
      ixm.OpenIndex(relName, attributes[i].indexNo, indexes[i]);
    }
    if(strcmp(attributes[i].attrName, updAttr.attrName) == 0) {
      updAttrOffset = attributes[i].offset;
    }
  }

  while(1) {
    rc = it->GetNext(t);
    if(rc ==  it->Eof())
      break;
    if (rc != 0) return rc;

    RM_Record rec;
    
    for (int i = 0; i < attrCount; i++) {
    if(attributes[i].indexNo != -1 && 
       strcmp(attributes[i].attrName, updAttr.attrName) == 0) {
        void * pKey;
        t.Get(attributes[i].offset, pKey);
        rc = indexes[i].DeleteEntry(pKey, t.GetRid());
        if (rc != 0) return rc;
        rc = indexes[i].InsertEntry(val, t.GetRid());
        if (rc != 0) return rc;
      }
    }

    t.Set(updAttrOffset, val);
    char * newbuf;
    t.GetData(newbuf);
    rec.Set(newbuf, it->TupleLength(), t.GetRid());
    rc = fh.UpdateRec(rec);
    if (rc != 0) return rc;

    p.Print(cout, t);
  }

  p.PrintFooter(cout);
  rc = it->Close();
  if (rc != 0) return rc;

  for (int i = 0; i < attrCount; i++) {
    if(attributes[i].indexNo != -1 && 
       strcmp(attributes[i].attrName, updAttr.attrName) == 0) {
      RC rc = ixm.CloseIndex(indexes[i]);
      if(rc != 0 ) return rc;
    }
  }

  delete [] indexes;
  delete [] attributes;

  rc =	rmm.CloseFile(fh);
  if (rc != 0) return rc;

  delete it;

  // cout << "Update\n";

  // cout << "   relName = " << relName << "\n";
  // cout << "   updAttr:" << updAttr << "\n";
  // if (bIsValue)
  //   cout << "   rhs is value: " << rhsValue << "\n";
  // else
  //   cout << "   rhs is attribute: " << rhsRelAttr << "\n";

  // cout << "   nConditions = " << nConditions << "\n";
  // for (int i = 0; i < nConditions; i++)
  //   cout << "   conditions[" << i << "]:" << conditions[i] << "\n";
  delete [] conditions;
  return 0;
}

//
// Choose between filescan and indexscan for first operation - leaf level of
// operator tree
// REturned iterator should be deleted by user after use.
Iterator* QL_Manager::GetLeafIterator(const char *relName,
                                      int nConditions, 
                                      const Condition conditions[],
                                      int nJoinConditions,
                                      const Condition jconditions[],
                                      int order,
                                      RelAttr* porderAttr)
{
  RC invalid = IsValid(); if(invalid) return NULL;

  if(relName == NULL) {
    return NULL;
  }

  int attrCount = -1;
  DataAttrInfo * attributes;
  RC rc = smm.GetFromTable(relName, attrCount, attributes);
  if(rc != 0) return NULL;

  int nIndexes = 0;
  char* chosenIndex = NULL;
  const Condition * chosenCond = NULL;
  Condition * filters = NULL;
  int nFilters = -1;
  Condition jBased = NULLCONDITION;

  map<string, const Condition*> jkeys;

  // cerr << relName << " nJoinConditions was " << nJoinConditions << endl;
            
  for(int j = 0; j < nJoinConditions; j++) {
    if(strcmp(jconditions[j].lhsAttr.relName, relName) == 0) {
      jkeys[string(jconditions[j].lhsAttr.attrName)] = &jconditions[j];
    }

    if(jconditions[j].bRhsIsAttr == TRUE &&
       strcmp(jconditions[j].rhsAttr.relName, relName) == 0) {
      jkeys[string(jconditions[j].rhsAttr.attrName)] = &jconditions[j];
    }
  }
  
  for(map<string, const Condition*>::iterator it = jkeys.begin(); it != jkeys.end(); it++) {
    // Pick last numerical index or at least one non-numeric index
    for (int i = 0; i < attrCount; i++) {
      if(attributes[i].indexNo != -1 && 
         strcmp(it->first.c_str(), attributes[i].attrName) == 0) {
        nIndexes++;
        if(chosenIndex == NULL ||
           attributes[i].attrType == INT || attributes[i].attrType == FLOAT) {
          chosenIndex = attributes[i].attrName;
          jBased = *(it->second);
          jBased.lhsAttr.attrName = chosenIndex;
          jBased.bRhsIsAttr = FALSE;
          jBased.rhsValue.type = attributes[i].attrType;
          jBased.rhsValue.data = NULL;

          chosenCond = &jBased;

          // cerr << "chose index for iscan based on join condition " <<
          //   *(chosenCond) << endl;
        }
      }
    }
  }

  // if join conds resulted in chosenCond
  if(chosenCond != NULL) {
    nFilters = nConditions;
    filters = new Condition[nFilters];
    for(int j = 0; j < nConditions; j++) {
      if(chosenCond != &(conditions[j])) {
        filters[j] = conditions[j];
      }
    }
  } else {
    // (chosenCond == NULL) // prefer join cond based index
    map<string, const Condition*> keys;

    for(int j = 0; j < nConditions; j++) {
      if(strcmp(conditions[j].lhsAttr.relName, relName) == 0) {
        keys[string(conditions[j].lhsAttr.attrName)] = &conditions[j];
      }

      if(conditions[j].bRhsIsAttr == TRUE &&
         strcmp(conditions[j].rhsAttr.relName, relName) == 0) {
        keys[string(conditions[j].rhsAttr.attrName)] = &conditions[j];
      }
    }
  
    for(map<string, const Condition*>::iterator it = keys.begin(); it != keys.end(); it++) {
      // Pick last numerical index or at least one non-numeric index
      for (int i = 0; i < attrCount; i++) {
        if(attributes[i].indexNo != -1 && 
           strcmp(it->first.c_str(), attributes[i].attrName) == 0) {
          nIndexes++;
          if(chosenIndex == NULL ||
             attributes[i].attrType == INT || attributes[i].attrType == FLOAT) {
            chosenIndex = attributes[i].attrName;
            chosenCond = it->second;
          }
        }
      }
    }
  
    if(chosenCond == NULL) {
      nFilters = nConditions;
      filters = new Condition[nFilters];
      for(int j = 0; j < nConditions; j++) {
        if(chosenCond != &(conditions[j])) {
          filters[j] = conditions[j];
        }
      }
    } else {
      nFilters = nConditions - 1;
      filters = new Condition[nFilters];
      for(int j = 0, k = 0; j < nConditions; j++) {
        if(chosenCond != &(conditions[j])) {
          filters[k] = conditions[j];
          k++;
        }
      }
    }
  }

  if(chosenCond == NULL && (nConditions == 0 || nIndexes == 0)) {
    Condition cond = NULLCONDITION;

    RC status = -1;
    Iterator* it = NULL;
    if(nConditions == 0)
      it = new FileScan(smm, rmm, relName, status, cond);
    else
      it = new FileScan(smm, rmm, relName, status, cond, nConditions,
                        conditions);
    
    if(status != 0) {
      PrintErrorAll(status);
      return NULL;
    }
    delete [] filters;
    delete [] attributes;
    return it;
  }

  // use an index scan
  RC status = -1;
  Iterator* it;

  bool desc = false;
  if(order != 0 &&
     strcmp(porderAttr->relName, relName) == 0 &&
     strcmp(porderAttr->attrName, chosenIndex) == 0)
    desc = (order == -1 ? true : false);

  if(chosenCond != NULL) {
    if(chosenCond->op == EQ_OP ||
       chosenCond->op == GT_OP ||
       chosenCond->op == GE_OP)
      if(order == 0) // use only if there is no order-by
        desc = true; // more optimal

    it = new IndexScan(smm, rmm, ixm, relName, chosenIndex, status,
                       *chosenCond, nFilters, filters, desc);
  }
  else // non-conditional index scan
    it = new IndexScan(smm, rmm, ixm, relName, chosenIndex, status,
                       NULLCONDITION, nFilters, filters, desc);

  if(status != 0) {
    PrintErrorAll(status);
    return NULL;
  }

  delete [] filters;
  delete [] attributes;
  return it;
}
