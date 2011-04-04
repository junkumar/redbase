//
// statistics.cc
//

// This file holds the implementation for the StatisticsMgr class.
//
// The class is designed to dynamically track statistics for the client.
// You can add any statistic that you would like to track via a call to
// StatisticsMgr::Register.

// There is no need to setup in advance which statistics that you want to
// track.  The call to Register is sufficient.

// This is essentially a (poor-man's) simplified version of gprof.

// Andre Bergholz, who was the TA for the 2000 offering has written
// some (or maybe all) of this code.

#include <cstring>
#include <iostream>
#include "statistics.h"

using namespace std;

//
// Here are Statistics Keys utilized by the PF layer of the Redbase
// project.
//
const char *PF_GETPAGE = "GETPAGE";
const char *PF_PAGEFOUND = "PAGEFOUND";
const char *PF_PAGENOTFOUND = "PAGENOTFOUND";
const char *PF_READPAGE = "READPAGE";           // IO
const char *PF_WRITEPAGE = "WRITEPAGE";         // IO
const char *PF_FLUSHPAGES = "FLUSHPAGES";

//
// Statistic class
//
// This class will track a single statistic

//
// Default Constructor utilized by the templates
//
Statistic::Statistic()
{
   psKey = NULL;
   iValue = 0;
}


//
// Constructor utilized by the StatisticMgr class
//
// We are assured by the StatisticMgr that psKey_ is not a NULL pointer.
//
Statistic::Statistic(const char *psKey_)
{
   psKey = new char[strlen(psKey_) + 1];
   strcpy (psKey, psKey_);

   iValue = 0;
}

//
// Copy constructor
//
Statistic::Statistic(const Statistic &stat)
{
    psKey = new char[strlen(stat.psKey)+1];
    strcpy (psKey, stat.psKey);

    iValue = stat.iValue;
}

//
// Equality constructor
//
Statistic& Statistic::operator=(const Statistic &stat)
{
   if (this==&stat)
      return *this;

   delete [] psKey;
   psKey = new char[strlen(stat.psKey)+1];
   strcpy (psKey, stat.psKey);

   iValue = stat.iValue;

   return *this;
}

//
// Destructor
//
Statistic::~Statistic()
{
   delete [] psKey;
}

Boolean Statistic::operator==(const char *psKey_) const
{
   return (strcmp(psKey_, psKey)==0);
}

// --------------------------------------------------------------

//
// StatisticMgr class
//
// This class will track a dynamic list of statistics.
//

//
// Register
//
// Register a change to a statistic.  The psKey is the char* name of
// the statistic to be tracked.  This method will look for the statistic
// name withing its list of statistics and perform the operation over the
// stored value.  The piValue is utilized for some of the operations.
//
// Note: if the statistic isn't found (as it will not be the very first
// time) then it will be initialized to 0 - the default value.
//
RC StatisticsMgr::Register (const char *psKey, const Stat_Operation op,
      const int *const piValue)
{
   int i, iCount;
   Statistic *pStat = NULL;

   if (psKey==NULL || (op != STAT_ADDONE && piValue == NULL))
      return STAT_INVALID_ARGS;

   iCount = llStats.GetLength();

   for (i=0; i < iCount; i++) {
      pStat = llStats[i];
      if (*pStat == psKey)
         break;
   }

   // Check to see if we found the Stat
   if (i==iCount)
      // We haven't found it so create a new statistic
      // with the key psKey and initial value of 0.
      pStat = new Statistic( psKey );

   // Now perform the operation over the statistic
   switch (op) {
      case STAT_ADDONE:
         pStat->iValue++;
         break;
      case STAT_ADDVALUE:
         pStat->iValue += *piValue;
         break;
      case STAT_SETVALUE:
         pStat->iValue = *piValue;
         break;
      case STAT_MULTVALUE:
         pStat->iValue *= *piValue;
         break;
      case STAT_DIVVALUE:
         pStat->iValue = (int) (pStat->iValue/(*piValue));
         break;
      case STAT_SUBVALUE:
         pStat->iValue -= *piValue;
         break;
   };

   // Finally, if the statistic wasn't in the original list then add it to
   // the list.
   //  JASON:: Confirm that it makes a copy of the object in line 229 of
   //  linkedlist.h.
   if (i==iCount) {
      llStats.Append(*pStat);
      delete pStat;
   }

   return 0;
}

//
// Print
//
// Print out the information pertaining to a specific statistic
RC StatisticsMgr::Print(const char *psKey)
{
   if (psKey==NULL)
      return STAT_INVALID_ARGS;

   int *iValue = Get(psKey);

   if (iValue)
      cout << psKey << "::" << *iValue << "\n";
   else
      return STAT_UNKNOWN_KEY;

   delete iValue;

   return 0;
}


//
// Get
//
// The Get method will return a pointer to the integer value associated
// with a particular statistic.  If it cannot find the statistic then it
// will return NULL.  The caller must remember to delete the memory
// returned when done.
//
int *StatisticsMgr::Get(const char *psKey)
{
   int i, iCount;
   Statistic *pStat = NULL;

   iCount = llStats.GetLength();

   for (i=0; i < iCount; i++) {
      pStat = llStats[i];
      if (*pStat == psKey)
         break;
   }

   // Check to see if we found the Stat
   if (i==iCount)
      return NULL;

   return new int(pStat->iValue);
}

//
// Print
//
// Print out all the statistics tracked
//
void StatisticsMgr::Print()
{
   int i, iCount;
   Statistic *pStat = NULL;

   iCount = llStats.GetLength();

   for (i=0; i < iCount; i++) {
      pStat = llStats[i];
      cout << pStat->psKey << "::" << pStat->iValue << "\n";
   }
}

//
// Reset
//
// Reset a specific statistic.  The easiest way to do this is to remove it
// completely from the list
//
RC StatisticsMgr::Reset(const char *psKey)
{
   int i, iCount;
   Statistic *pStat = NULL;

   if (psKey==NULL)
      return STAT_INVALID_ARGS;

   iCount = llStats.GetLength();

   for (i=0; i < iCount; i++) {
      pStat = llStats[i];
      if (*pStat == psKey)
         break;
   }

   // If we found the statistic then remove it from the list
   if (i!=iCount)
      llStats.Delete(i);
   else
      return STAT_UNKNOWN_KEY;

   return 0;
}

//
// Reset
//
// Reset all of the statistics.  The easiest way is to tell the linklist of
// elements to Erase itself.
//
void StatisticsMgr::Reset()
{
   llStats.Erase();
}

