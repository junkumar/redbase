//
// statistics.h
//

// This file contains the interface for the statistics class
// which can be setup to dynamically track statistics within a
// C++ program.

// This is essentially a (poor-man's) simplified version of gprof.

// My intent here is to allow the StatisticsMgr to be used with very little
// setup on the part of the client.  Notice that you don't need to setup
// all the statistics that you will be tracking.  You just Register the
// statistic as you go.  In the end the Print or Get methods will allow you
// to report all the statistics.

// Andre Bergholz, who was the TA for the 2000 offering, has written
// some (or probably all) of this code.

#ifndef STATISTICS_H
#define STATISTICS_H

// Some common definitions that might not already be set
#ifndef Boolean
typedef char Boolean;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef RC
typedef int RC;
#endif

#ifndef STAT_BASE
const int STAT_BASE = 9000;
#endif

// This include must come after the common defines
#include "linkedlist.h"    // Template class for the link list

// A single statistic will be tracked by a Statistic class
class Statistic {
public:
    Statistic();
    ~Statistic();
    Statistic(const char *psName);

    // Copy constructor
    Statistic(const Statistic &stat);

    // Equality constructor
    Statistic& operator=(const Statistic &stat);

    // Check for equality between a Statistic and a name based upon the
    // names given to the current statistic.
    Boolean operator==(const char *psName_) const;

    // The name or key given to the statistic that this structure is
    // tracking
    char *psKey;

    // Currently, I have only allowed the statistic to track integer values.
    // Initial value will be 0.
    int iValue;
};

// These are the different operations that a single statistic can undergo
// duing a call to StatisticsMgr::Register.
enum Stat_Operation {
    STAT_ADDONE,
    STAT_ADDVALUE,
    STAT_SETVALUE,
    STAT_MULTVALUE,
    STAT_DIVVALUE,
    STAT_SUBVALUE
};

// The StatisticsMgr will track a group of statistics
class StatisticsMgr {

public:
    StatisticsMgr() {};
    ~StatisticsMgr() {};

    // Add a new statistic or register a change to an existing statistic.
    // The piValue for can be NULL, except for those operations that require
    // it.  When adding the default value is 0 with the Stat_Operation being
    // performed over the initial value.
    RC Register(const char *psKey, const Stat_Operation op,
                const int *const piValue = NULL);

    // Get will return the value associated with a particular statistic.
    // Caller is responsible for deleting the memory returned.
    int *Get(const char *psKey);

    // Print out a specific statistic
    RC Print(const char *psKey);

    // Print out all the statistics tracked
    void Print();

    // Reset a specific statistic
    RC Reset(const char *psKey);

    // Reset all of the statistics
    void Reset();

private:
    LinkList<Statistic> llStats;
};

//
// Return codes
//
const int STAT_INVALID_ARGS = STAT_BASE+1;  // Bad Args in call to method
const int STAT_UNKNOWN_KEY  = STAT_BASE+2;  // No such Key being tracked

//
// The following are specifically for tracking the statistics in the PF
// component of Redbase.  When statistics are utilized, these constants
// will be used as the keys for the statistics manager.
//
extern const char *PF_GETPAGE;
extern const char *PF_PAGEFOUND;
extern const char *PF_PAGENOTFOUND;
extern const char *PF_READPAGE;         // IO
extern const char *PF_WRITEPAGE;        // IO
extern const char *PF_FLUSHPAGES;

#endif

