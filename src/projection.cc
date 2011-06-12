//
// File:        projection.cc
//

#include "projection.h"
#include "ql_error.h"
#include "sm_error.h"

using namespace std;

Projection::Projection(Iterator* lhsIt_,
                       RC& status,
                       int nProjections,
                       const AggRelAttr projections[])
  :lhsIt(lhsIt_)
{
  if(lhsIt == NULL || nProjections <= 0) {
    status = SM_NOSUCHTABLE;
    return;
  }

  attrCount = nProjections;
  assert(attrCount > 0);

  attrs = new DataAttrInfo[attrCount];
  lattrs = new DataAttrInfo[attrCount];

  DataAttrInfo * itattrs = lhsIt_->GetAttr();
  int offsetsofar = 0;

  // cout << "Projection::Projection() lhsIt->GetAttrCount() " << lhsIt->GetAttrCount() << endl;
      
  for(int j = 0; j < attrCount; j++) {
    for(int i = 0; i < lhsIt->GetAttrCount(); i++) {
      // cout << "Projection::Projection() itattrs[i].func " << itattrs[i].func
      //      << endl;
      // cout << "Projection::Projection() itattrs[i].attrName " << itattrs[i].attrName << endl;

      if(strcmp(projections[j].relName, itattrs[i].relName) == 0 &&
         strcmp(projections[j].attrName, itattrs[i].attrName) == 0 &&
         projections[j].func == itattrs[i].func
        ) {
        lattrs[j] = itattrs[i];
        attrs[j] = itattrs[i];
        attrs[j].offset = offsetsofar;
        offsetsofar += itattrs[i].attrLength;
        attrs[j].func = itattrs[i].func;
        // cout << "Projection::Projection() attrs[j].attrName " << attrs[j].attrName << endl;
        // cout << "Projection::Projection() lattrs[i].offset " << lattrs[i].offset << endl;
        // cout << "offsetsofar " << offsetsofar << endl;
        break;
      }
    }
  }

  // project leaves sort order of child intact
  if(lhsIt->IsSorted()) {
    bSorted = true;
    desc = lhsIt->IsDesc();
    sortRel = lhsIt->GetSortRel();
    sortAttr = lhsIt->GetSortAttr();
  }

  explain << "Project\n";
  if(nProjections > 0) {
    explain << "   nProjections = " << nProjections << "\n";
    explain << "      ";
    for (int i = 0; i < nProjections; i++) {
      // explain << "   projections[" << i << "]:" << "\n";
      explain << projections[i] << ",";
    }
    explain 
      // << " " << lattrs[i].offset << "," << lattrs[i].attrLength
      // << " " << attrs[i].offset << "," << attrs[i].attrLength
      << "\b\n";
  }
  
  status = 0;
}

string Projection::Explain() {
  stringstream dyn;
  dyn << indent << explain.str();
  lhsIt->SetIndent(indent + "-----");
  dyn << lhsIt->Explain();
  return dyn.str();
}

Projection::~Projection()
{
  delete lhsIt;
  delete [] attrs;
  delete [] lattrs;
}

RC Projection::GetNext(Tuple &t)
{
  Tuple ltuple = lhsIt->GetTuple();
  RC rc = lhsIt->GetNext(ltuple);
  if(rc !=0) return rc;

  char * buf;
  t.GetData(buf);
  char *lbuf;
  ltuple.GetData(lbuf);

  // cout << "Projection::GetNext() ltuple " << ltuple << endl;

  for(int i = 0; i < attrCount; i++) {
    // cout << "Projection::GetNext() attrs[i].offset " << attrs[i].offset << endl;
    // cout << "Projection::GetNext() lattrs[i].offset " << lattrs[i].offset << endl;

    memcpy(buf + attrs[i].offset,
           lbuf + lattrs[i].offset,
           attrs[i].attrLength);
  }
  // cout << "Projection::GetNext() tuple " << t << endl;
  return 0;
}
