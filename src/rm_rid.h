//
// rm_rid.h
//
//   The Record Id interface
//

#ifndef RM_RID_H
#define RM_RID_H

// We separate the interface of RID from the rest of RM because some
// components will require the use of RID but not the rest of RM.

#include "redbase.h"

//
// PageNum: uniquely identifies a page in a file
//
typedef int PageNum;

//
// SlotNum: uniquely identifies a record in a page
//
typedef int SlotNum;

//
// RID: Record id interface
//
class RID {
public:
  static const PageNum NULL_PAGE = -1;
  static const SlotNum NULL_SLOT = -1;
  RID()
    :page(NULL_PAGE), slot(NULL_SLOT) {}     // Default constructor
  RID(PageNum pageNum, SlotNum slotNum)
    :page(pageNum), slot(slotNum) {}
  ~RID(){}                                        // Destructor
  
  RC GetPageNum(PageNum &pageNum) const          // Return page number
  { return page; }
  RC GetSlotNum(SlotNum &slotNum) const         // Return slot number
  { return slot; }

private:
  PageNum page;
  SlotNum slot;
};

#endif // RM_RID_H
