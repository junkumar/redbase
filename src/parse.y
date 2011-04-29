%{
/*
 * parser.y: yacc specification for RQL
 *
 * Authors: Dallan Quass
 *          Jan Jannink
 *          Jason McHugh
 *
 * originally by: Mark McAuliffe, University of Wisconsin - Madison, 1991
 *
 * 1997: Added "print buffer", "print io", "reset io" and the "*" in
 * SFW Query.
 * 1998: Added "reset buffer", "resize buffer [int]", "queryplans on",
 * and "queryplans off".
 * 2000: Added "const" to yyerror-header
 *
 */

#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <cstdlib>
#include <unistd.h>
#include "redbase.h"
#include "parser_internal.h"
#include "pf.h"     // for PF_PrintError
#include "rm.h"     // for RM_PrintError
#include "ix.h"     // for IX_PrintError
#include "sm.h"
#include "ql.h"

using namespace std;

  // Added by Wendy Tobagus
#ifndef yyrestart
void yyrestart(FILE*);
#endif

// The PF_STATS indicates that we will be tracking statistics for the PF
// Layer.  The Manager is defined within pf_buffermgr.cc.  
// We include it within the parser so that a system command can display
// statistics about the DB.
#ifdef PF_STATS
#include "statistics.h"

// This is defined within the pf_buffermgr.cc
extern StatisticsMgr *pStatisticsMgr;

#endif    // PF_STATS

/*
 * string representation of tokens; provided by scanner
 */
extern char *yytext;

/*
 * points to root of parse tree
 */
static NODE *parse_tree;

int bExit;                 // when to return from RBparse

int bQueryPlans;           // When to print the query plans

PF_Manager *pPfm;          // PF component manager
SM_Manager *pSmm;          // SM component manager
QL_Manager *pQlm;          // QL component manager

%}

%union{
    int ival;
    CompOp cval;
    float rval;
    char *sval;
    NODE *n;
}

%token     
      RW_CREATE
      RW_DROP
      RW_TABLE
      RW_INDEX
      RW_LOAD
      RW_SET
      RW_HELP
      RW_PRINT
      RW_EXIT
      RW_SELECT
      RW_FROM
      RW_WHERE
      RW_INSERT
      RW_DELETE
      RW_UPDATE
      RW_AND
      RW_INTO
      RW_VALUES
      T_EQ
      T_LT
      T_LE
      T_GT
      T_GE
      T_NE
      T_EOF
      NOTOKEN
      RW_RESET
      RW_IO
      RW_BUFFER
      RW_RESIZE
      RW_QUERY_PLAN
      RW_ON
      RW_OFF

%token   <ival>   T_INT

%token   <rval>   T_REAL

%token   <sval>   T_STRING
      T_QSTRING
      T_SHELL_CMD

%type   <cval>   op

%type   <sval>   opt_relname

%type   <n>   command
      ddl
      dml
      utility
      createtable
      createindex
      droptable
      dropindex
      load
      set
      help
      print
      exit
      query
      insert
      delete 
      update
      non_mt_attrtype_list
      attrtype
      non_mt_relattr_list
      non_mt_select_clause
      relattr
      non_mt_relation_list
      relation
      opt_where_clause
      non_mt_cond_list
      condition
      relattr_or_value
      non_mt_value_list
      value
      buffer
      statistics
      queryplans
%%

start
   : command ';'
   {
      parse_tree = $1;
      YYACCEPT;
   }
   | T_SHELL_CMD
   {
      if (!isatty(0)) {
        cout << ($1) << "\n";
        cout.flush();
      }
      system($1);
      parse_tree = NULL;
      YYACCEPT;
   }
   | error
   {
      reset_scanner();
      parse_tree = NULL;
      YYACCEPT;
   }
   | T_EOF
   {
      parse_tree = NULL;
      bExit = 1;
      YYACCEPT;
   }
   ;

command
   : ddl
   | dml
   | utility
   | nothing
   {
      $$ = NULL;
   }
   ;

ddl
   : createtable
   | createindex
   | droptable
   | dropindex
   ;

dml
   : query
   | insert
   | delete
   | update
   ;

utility
   : load
   | exit
   | set
   | help
   | print
   | buffer
   | statistics 
   | queryplans 
   ;

queryplans
   : RW_QUERY_PLAN RW_ON
   {
      bQueryPlans = 1;
      cout << "Query plan display turned on.\n";
      $$ = NULL;
   }
   | RW_QUERY_PLAN RW_OFF
   { 
      bQueryPlans = 0;
      cout << "Query plan display turned off.\n";
      $$ = NULL;
   }
   ;

   

buffer
   : RW_RESET RW_BUFFER
   {
      if (pPfm->ClearBuffer())
         cout << "Trouble clearing buffer!  Things may be pinned.\n";
      else 
         cout << "Everything kicked out of Buffer!\n";
      $$ = NULL;
   }
   | RW_PRINT RW_BUFFER
   {
      pPfm->PrintBuffer();
      $$ = NULL;
   }
   | RW_RESIZE RW_BUFFER T_INT
   {
      pPfm->ResizeBuffer($3);
      $$ = NULL;
   }
   ;

statistics
   : RW_PRINT RW_IO
   {
      #ifdef PF_STATS
         cout << "Statistics\n";
         cout << "----------\n";
         pStatisticsMgr->Print();
      #else
         cout << "Statitisics not compiled.\n";
      #endif
      $$ = NULL;
   }
   | RW_RESET RW_IO
   {
      #ifdef PF_STATS
         cout << "Statistics reset.\n";
         pStatisticsMgr->Reset();
      #else
         cout << "Statitisics not compiled.\n";
      #endif
      $$ = NULL;
   }
   ;

createtable
   : RW_CREATE RW_TABLE T_STRING '(' non_mt_attrtype_list ')'
   {
      $$ = create_table_node($3, $5);
   }
   ;

createindex
   : RW_CREATE RW_INDEX T_STRING '(' T_STRING ')'
   {
      $$ = create_index_node($3, $5);
   }
   ;

droptable
   : RW_DROP RW_TABLE T_STRING
   {
      $$ = drop_table_node($3);
   }
   ;

dropindex
   : RW_DROP RW_INDEX T_STRING '(' T_STRING ')'
   {
      $$ = drop_index_node($3, $5);
   }
   ;

load
   : RW_LOAD T_STRING '(' T_QSTRING ')'
   {
      $$ = load_node($2, $4);
   }
   ;


set
   : RW_SET T_STRING T_EQ T_QSTRING
   {
      $$ = set_node($2, $4);
   }
   ;

help
   : RW_HELP opt_relname
   {
      $$ = help_node($2);
   }
   ;

print
   : RW_PRINT T_STRING
   {
      $$ = print_node($2);
   }
   ;

exit
   : RW_EXIT
   {
      $$ = NULL;
      bExit = 1;
   }
   ;

query
   : RW_SELECT non_mt_select_clause RW_FROM non_mt_relation_list opt_where_clause
   {
      $$ = query_node($2, $4, $5);
   }
   ;

insert
   : RW_INSERT RW_INTO T_STRING RW_VALUES '(' non_mt_value_list ')'
   {
      $$ = insert_node($3, $6);
   }
   ;

delete
   : RW_DELETE RW_FROM T_STRING opt_where_clause
   {
      $$ = delete_node($3, $4);
   }
   ;

update
   : RW_UPDATE T_STRING RW_SET relattr T_EQ relattr_or_value opt_where_clause
   {
      $$ = update_node($2, $4, $6, $7);
   }
   ;

non_mt_attrtype_list
   : attrtype ',' non_mt_attrtype_list
   {
      $$ = prepend($1, $3);
   }
   | attrtype
   {
      $$ = list_node($1);
   }
   ;

attrtype
   : T_STRING T_STRING
    {
      $$ = attrtype_node($1, $2);
   }
   ;

non_mt_select_clause
   : non_mt_relattr_list
   | '*'
   {
       $$ = list_node(relattr_node(NULL, (char*)"*"));
   }
      

non_mt_relattr_list
   : relattr ',' non_mt_relattr_list
   {
      $$ = prepend($1, $3);
   }
   | relattr
   {
      $$ = list_node($1);
   }
   ;

relattr
   : T_STRING '.' T_STRING
   {
      $$ = relattr_node($1, $3);
   }
   | T_STRING
   {
      $$ = relattr_node(NULL, $1);
   }
   ;

non_mt_relation_list
   : relation ',' non_mt_relation_list
   {
      $$ = prepend($1, $3);
   }
   | relation
   {
      $$ = list_node($1);
   }
   ;

relation
   : T_STRING
   {
      $$ = relation_node($1);
   }
   ;

opt_where_clause
   : RW_WHERE non_mt_cond_list
   {
      $$ = $2;
   }
   | nothing
   {
      $$ = NULL;
   }
   ;

non_mt_cond_list
   : condition RW_AND non_mt_cond_list
   {
      $$ = prepend($1, $3);
   }
   | condition
   {
      $$ = list_node($1);
   }
   ;

condition
   : relattr op relattr_or_value
   {
      $$ = condition_node($1, $2, $3);
   }
   ;

relattr_or_value
   : relattr
   {
      $$ = relattr_or_value_node($1, NULL);
   }
   | value
   {
      $$ = relattr_or_value_node(NULL, $1);
   }
   ;

non_mt_value_list
   : value ',' non_mt_value_list
   {
      $$ = prepend($1, $3);
   }
   | value
   {
      $$ = list_node($1);
   }
   ;

value
   : T_QSTRING
   {
      $$ = value_node(STRING, (void *) $1);
   }
   | T_INT
   {
      $$ = value_node(INT, (void *)& $1);
   }
   | T_REAL
   {
      $$ = value_node(FLOAT, (void *)& $1);
   }
   ;

opt_relname
   : T_STRING
   {
      $$ = $1;
   }
   | nothing
   {
      $$ = NULL;
   }
   ;

op
   : T_LT
   {
      $$ = LT_OP;
   }
   | T_LE
   {
      $$ = LE_OP;
   }
   | T_GT
   {
      $$ = GT_OP;
   }
   | T_GE
   {
      $$ = GE_OP;
   }
   | T_EQ
   {
      $$ = EQ_OP;
   }
   | T_NE
   {
      $$ = NE_OP;
   }
   ;

nothing
   : /* epsilon */
   ;

%%

//
// PrintError
//
// Desc: Print an error message by calling the proper component-specific
//       print-error function
//
void PrintError(RC rc)
{
   if (abs(rc) <= END_PF_WARN)
      PF_PrintError(rc);
   else if (abs(rc) <= END_RM_WARN)
      RM_PrintError(rc);
   else if (abs(rc) <= END_IX_WARN)
      IX_PrintError(rc);
   else if (abs(rc) <= END_SM_WARN)
      SM_PrintError(rc);
   else if (abs(rc) <= END_QL_WARN)
      QL_PrintError(rc);
   else
      cerr << "Error code out of range: " << rc << "\n";
}

//
// RBparse
//
// Desc: Parse redbase commands
//
void RBparse(PF_Manager &pfm, SM_Manager &smm, QL_Manager &qlm)
{
   RC rc;

   // Set up global variables to their defaults
   pPfm  = &pfm;
   pSmm  = &smm;
   pQlm  = &qlm;
   bExit = 0;
   bQueryPlans = 0;

   /* Do forever */
   while (!bExit) {

      /* Reset parser and scanner for a new query */
      new_query();

      /* Print a prompt */
      cout << PROMPT;

      /* Get the prompt to actually show up on the screen */
      cout.flush(); 

      /* If a query was successfully read, interpret it */
      if(yyparse() == 0 && parse_tree != NULL)
         if ((rc = interp(parse_tree))) {
            PrintError(rc);
            if (rc < 0)
               bExit = TRUE;
         }
   }
}

//
// Functions for printing the various structures to an output stream
//
ostream &operator<<(ostream &s, const AttrInfo &ai)
{
   return
      s << " attrName=" << ai.attrName
      << " attrType=" << 
      (ai.attrType == INT ? "INT" :
       ai.attrType == FLOAT ? "FLOAT" : "STRING")
      << " attrLength=" << ai.attrLength;
}

ostream &operator<<(ostream &s, const RelAttr &qa)
{
   return
      s << (qa.relName ? qa.relName : "NULL")
      << "." << qa.attrName;
}

ostream &operator<<(ostream &s, const Condition &c)
{
   s << "\n      lhsAttr:" << c.lhsAttr << "\n"
      << "      op=" << c.op << "\n";
   if (c.bRhsIsAttr)
      s << "      bRhsIsAttr=TRUE \n      rhsAttr:" << c.rhsAttr;
   else
      s << "      bRshIsAttr=FALSE\n      rhsValue:" << c.rhsValue;
   return s;
}

ostream &operator<<(ostream &s, const Value &v)
{
   s << "AttrType: " << v.type;
   switch (v.type) {
      case INT:
         s << " *(int *)data=" << *(int *)v.data;
         break;
      case FLOAT:
         s << " *(float *)data=" << *(float *)v.data;
         break;
      case STRING:
         s << " (char *)data=" << (char *)v.data;
         break;
   }
   return s;
}

ostream &operator<<(ostream &s, const CompOp &op)
{
   switch(op){
      case EQ_OP:
         s << " =";
         break;
      case NE_OP:
         s << " <>";
         break;
      case LT_OP:
         s << " <";
         break;
      case LE_OP:
         s << " <=";
         break;
      case GT_OP:
         s << " >";
         break;
      case GE_OP:
         s << " >=";
         break;
      case NO_OP:
         s << " NO_OP";
         break;
   }
   return s;
}

ostream &operator<<(ostream &s, const AttrType &at)
{
   switch(at){
      case INT:
         s << "INT";
         break;
      case FLOAT:
         s << "FLOAT";
         break;
      case STRING:
         s << "STRING";
         break;
   }
   return s;
}

/*
 * Required by yacc
 */
void yyerror(char const *s) // New in 2000
{
   puts(s);
}

#if 0
/*
 * Sometimes required
 */
int yywrap(void)
{
   return 1;
}
#endif
