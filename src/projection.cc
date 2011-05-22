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
                       const RelAttr projections[])
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

  DataAttrInfo * itattrs = lhsIt->GetAttr();
  int offsetsofar = 0;

  for(int j = 0; j < attrCount; j++) {
    for(int i = 0; i < lhsIt->GetAttrCount(); i++) {
      if(strcmp(projections[j].relName, itattrs[i].relName) == 0 &&
         strcmp(projections[j].attrName, itattrs[i].attrName) == 0) {
        lattrs[j] = itattrs[i];
        attrs[j] = itattrs[i];
        attrs[j].offset = offsetsofar;
        offsetsofar += itattrs[i].attrLength;
        break;
      }
    }
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

  for(int i = 0; i < attrCount; i++) {
    memcpy(buf + attrs[i].offset,
           lbuf + lattrs[i].offset,
           attrs[i].attrLength);
  }
  return 0;
}
