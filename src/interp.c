/*
 * interp.c: interpreter for RQL
 *
 * Authors: Dallan Quass (quass@cs.stanford.edu)
 *          Jan Jannink
 * originally by: Mark McAuliffe, University of Wisconsin - Madison, 1991
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "redbase.h"
#include "parser_internal.h"
#include "y.tab.h"

#include "sm.h"
#include "ql.h"

extern SM_Manager *pSmm;
extern QL_Manager *pQlm;

#define E_OK                0
#define E_INCOMPATIBLE      -1
#define E_TOOMANY           -2
#define E_NOLENGTH          -3
#define E_INVINTSIZE        -4
#define E_INVREALSIZE       -5
#define E_INVFORMATSTRING   -6
#define E_INVSTRLEN         -7
#define E_DUPLICATEATTR     -8
#define E_TOOLONG           -9
#define E_STRINGTOOLONG     -10

/*
 * file pointer to which error messages are printed
 */
#define ERRFP stderr

/*
 * local functions
 */
static int mk_attr_infos(NODE *list, int max, AttrInfo attrInfos[]);
static int parse_format_string(char *format_string, AttrType *type, int *len);
static int mk_rel_attrs(NODE *list, int max, RelAttr relAttrs[]);
static void mk_rel_attr(NODE *node, RelAttr &relAttr);
static int mk_relations(NODE *list, int max, char *relations[]);
static int mk_conditions(NODE *list, int max, Condition conditions[]);
static int mk_values(NODE *list, int max, Value values[]);
static void mk_value(NODE *node, Value &value);
static void print_error(char *errmsg, RC errval);
static void echo_query(NODE *n);
static void print_attrtypes(NODE *n);
static void print_op(CompOp op);
static void print_relattr(NODE *n);
static void print_value(NODE *n);
static void print_condition(NODE *n);
static void print_relattrs(NODE *n);
static void print_relations(NODE *n);
static void print_conditions(NODE *n);
static void print_values(NODE *n);

/*
 * interp: interprets parse trees
 *
 */
RC interp(NODE *n)
{
   RC errval = 0;         /* returned error value      */

   /* if input not coming from a terminal, then echo the query */
   if(!isatty(0))
      echo_query(n);

   switch(n -> kind){

      case N_CREATETABLE:            /* for CreateTable() */
         {
            int nattrs;
            AttrInfo attrInfos[MAXATTRS];

            /* Make sure relation name isn't too long */
            if(strlen(n -> u.CREATETABLE.relname) > MAXNAME){
               print_error((char*)"create", E_TOOLONG);
               break;
            }

            /* Make a list of AttrInfos suitable for sending to Create */
            nattrs = mk_attr_infos(n -> u.CREATETABLE.attrlist, MAXATTRS, 
                  attrInfos);
            if(nattrs < 0){
               print_error((char*)"create", nattrs);
               break;
            }

            /* Make the call to create */
            errval = pSmm->CreateTable(n->u.CREATETABLE.relname, nattrs, 
                  attrInfos);
            break;
         }   

      case N_CREATEINDEX:            /* for CreateIndex() */

         errval = pSmm->CreateIndex(n->u.CREATEINDEX.relname,
               n->u.CREATEINDEX.attrname);
         break;

      case N_DROPINDEX:            /* for DropIndex() */

         errval = pSmm->DropIndex(n->u.DROPINDEX.relname,
               n->u.DROPINDEX.attrname);
         break;

      case N_DROPTABLE:            /* for DropTable() */

         errval = pSmm->DropTable(n->u.DROPTABLE.relname);
         break;

      case N_LOAD:            /* for Load() */

         errval = pSmm->Load(n->u.LOAD.relname,
               n->u.LOAD.filename);
         break;

      case N_SET:                    /* for Set() */

         errval = pSmm->Set(n->u.SET.paramName,
               n->u.SET.string);
         break;

      case N_HELP:            /* for Help() */

         if (n->u.HELP.relname)
            errval = pSmm->Help(n->u.HELP.relname);
         else
            errval = pSmm->Help();
         break;

      case N_PRINT:            /* for Print() */

         errval = pSmm->Print(n->u.PRINT.relname);
         break;

      case N_QUERY:            /* for Query() */
         {
            int       nSelAttrs = 0;
            RelAttr  relAttrs[MAXATTRS];
            int       nRelations = 0;
            char      *relations[MAXATTRS];
            int       nConditions = 0;
            Condition conditions[MAXATTRS];

            /* Make a list of RelAttrs suitable for sending to Query */
            nSelAttrs = mk_rel_attrs(n->u.QUERY.relattrlist, MAXATTRS,
                  relAttrs);
            if(nSelAttrs < 0){
               print_error((char*)"select", nSelAttrs);
               break;
            }

            /* Make a list of relation names suitable for sending to Query */
            nRelations = mk_relations(n->u.QUERY.rellist, MAXATTRS, relations);
            if(nRelations < 0){
               print_error((char*)"select", nRelations);
               break;
            }

            /* Make a list of Conditions suitable for sending to Query */
            nConditions = mk_conditions(n->u.QUERY.conditionlist, MAXATTRS,
                  conditions);
            if(nConditions < 0){
               print_error((char*)"select", nConditions);
               break;
            }

            /* Make the call to Select */
            errval = pQlm->Select(nSelAttrs, relAttrs,
                  nRelations, relations,
                  nConditions, conditions);
            break;
         }   

      case N_INSERT:            /* for Insert() */
         {
            int nValues = 0;
            Value values[MAXATTRS];

            /* Make a list of Values suitable for sending to Insert */
            nValues = mk_values(n->u.INSERT.valuelist, MAXATTRS, values);
            if(nValues < 0){
               print_error((char*)"insert", nValues);
               break;
            }

            /* Make the call to insert */
            errval = pQlm->Insert(n->u.INSERT.relname,
                  nValues, values);
            break;
         }   

      case N_DELETE:            /* for Delete() */
         {
            int nConditions = 0;
            Condition conditions[MAXATTRS];

            /* Make a list of Conditions suitable for sending to delete */
            nConditions = mk_conditions(n->u.DELETE.conditionlist, MAXATTRS,
                  conditions);
            if(nConditions < 0){
               print_error((char*)"delete", nConditions);
               break;
            }

            /* Make the call to delete */
            errval = pQlm->Delete(n->u.DELETE.relname,
                  nConditions, conditions);
            break;
         }   

      case N_UPDATE:            /* for Update() */
         {
            RelAttr relAttr;

            // The RHS can be either a value or an attribute
            Value rhsValue;
            RelAttr rhsRelAttr;
            int bIsValue;

            int nConditions = 0;
            Condition conditions[MAXATTRS];

            /* Make a RelAttr suitable for sending to Update */
            mk_rel_attr(n->u.UPDATE.relattr, relAttr);

            struct node *rhs = n->u.UPDATE.relorvalue;
            if (rhs->u.RELATTR_OR_VALUE.relattr) {
               mk_rel_attr(rhs->u.RELATTR_OR_VALUE.relattr, rhsRelAttr);
               bIsValue = 0;
            } else {
               /* Make a value suitable for sending to update */
               mk_value(rhs->u.RELATTR_OR_VALUE.value, rhsValue);
               bIsValue = 1;
            }

            /* Make a list of Conditions suitable for sending to Update */
            nConditions = mk_conditions(n->u.UPDATE.conditionlist, MAXATTRS,
                  conditions);
            if(nConditions < 0){
               print_error((char*)"update", nConditions);
               break;
            }

            /* Make the call to update */
            errval = pQlm->Update(n->u.UPDATE.relname, relAttr, bIsValue, 
                  rhsRelAttr, rhsValue, nConditions, conditions);
            break;
         }   

      default:   // should never get here
         break;
   }

   return (errval);
}

/*
 * mk_attr_infos: converts a list of attribute descriptors (attribute names,
 * types, and lengths) to an array of AttrInfo's so it can be sent to
 * Create.
 *
 * Returns:
 *    length of the list on success ( >= 0 )
 *    error code otherwise
 */
static int mk_attr_infos(NODE *list, int max, AttrInfo attrInfos[])
{
   int i;
   int len;
   AttrType type;
   NODE *attr;
   RC errval;

   /* for each element of the list... */
   for(i = 0; list != NULL; ++i, list = list -> u.LIST.next) {

      /* if the list is too long, then error */
      if(i == max)
         return E_TOOMANY;

      attr = list -> u.LIST.curr;

      /* Make sure the attribute name isn't too long */
      if(strlen(attr -> u.ATTRTYPE.attrname) > MAXNAME)
         return E_TOOLONG;

      /* interpret the format string */
      errval = parse_format_string(attr -> u.ATTRTYPE.type, &type, &len);
      if(errval != E_OK)
         return errval;

      /* add it to the list */
      attrInfos[i].attrName = attr -> u.ATTRTYPE.attrname;
      attrInfos[i].attrType = type;
      attrInfos[i].attrLength = len;
   }

   return i;
}

/*
 * mk_rel_attrs: converts a list of relation-attributes (<relation,
 * attribute> pairs) into an array of RelAttrs
 *
 * Returns:
 *    the lengh of the list on success ( >= 0 )
 *    error code otherwise
 */
static int mk_rel_attrs(NODE *list, int max, RelAttr relAttrs[])
{
   int i;

   /* For each element of the list... */
   for(i = 0; list != NULL; ++i, list = list -> u.LIST.next){
      /* If the list is too long then error */
      if(i == max)
         return E_TOOMANY;

      mk_rel_attr(list->u.LIST.curr, relAttrs[i]);
   }

   return i;
}

/*
 * mk_rel_attr: converts a single relation-attribute (<relation,
 * attribute> pair) into a RelAttr
 */
static void mk_rel_attr(NODE *node, RelAttr &relAttr)
{
   relAttr.relName = node->u.RELATTR.relname;
   relAttr.attrName = node->u.RELATTR.attrname;
}

/*
 * mk_relations: converts a list of relations into an array of relations
 *
 * Returns:
 *    the lengh of the list on success ( >= 0 )
 *    error code otherwise
 */
static int mk_relations(NODE *list, int max, char *relations[])
{
   int i;
   NODE *current;

   /* for each element of the list... */
   for(i = 0; list != NULL; ++i, list = list -> u.LIST.next){
      /* If the list is too long then error */
      if(i == max)
         return E_TOOMANY;

      current = list -> u.LIST.curr;
      relations[i] = current->u.RELATION.relname;
   }

   return i;
}

/*
 * mk_conditions: converts a list of conditions into an array of conditions
 *
 * Returns:
 *    the lengh of the list on success ( >= 0 )
 *    error code otherwise
 */
static int mk_conditions(NODE *list, int max, Condition conditions[])
{
   int i;
   NODE *current;

   /* for each element of the list... */
   for(i = 0; list != NULL; ++i, list = list -> u.LIST.next){
      /* If the list is too long then error */
      if(i == max)
         return E_TOOMANY;

      current = list -> u.LIST.curr;
      conditions[i].lhsAttr.relName = 
         current->u.CONDITION.lhsRelattr->u.RELATTR.relname;
      conditions[i].lhsAttr.attrName = 
         current->u.CONDITION.lhsRelattr->u.RELATTR.attrname;
      conditions[i].op = current->u.CONDITION.op;
      if (current->u.CONDITION.rhsRelattr) {
         conditions[i].bRhsIsAttr = TRUE;
         conditions[i].rhsAttr.relName = 
            current->u.CONDITION.rhsRelattr->u.RELATTR.relname;
         conditions[i].rhsAttr.attrName = 
            current->u.CONDITION.rhsRelattr->u.RELATTR.attrname;
      }
      else {
         conditions[i].bRhsIsAttr = FALSE;
         mk_value(current->u.CONDITION.rhsValue, conditions[i].rhsValue);
      }
   }

   return i;
}

/*
 * mk_values: converts a list of values into an array of values
 *
 * Returns:
 *    the lengh of the list on success ( >= 0 )
 *    error code otherwise
 */
static int mk_values(NODE *list, int max, Value values[])
{
   int i;

   /* for each element of the list... */
   for(i = 0; list != NULL; ++i, list = list -> u.LIST.next){
      /* If the list is too long then error */
      if(i == max)
         return E_TOOMANY;

      mk_value(list->u.LIST.curr, values[i]);
   }

   return i;
}

/*
 * mk_values: converts a single value node into a Value
 */
static void mk_value(NODE *node, Value &value)
{
   value.type = node->u.VALUE.type;
   switch (value.type) {
      case INT:
         value.data = (void *)&node->u.VALUE.ival;
         break;
      case FLOAT:
         value.data = (void *)&node->u.VALUE.rval;
         break;
      case STRING:
         value.data = (void *)node->u.VALUE.sval;
         break;
   }
}

/*
 * parse_format_string: deciphers a format string of the form: xl
 * where x is a type specification (one of `i' INTEGER, `r' REAL,
 * `s' STRING, or `c' STRING (character)) and l is a length (l is
 * optional for `i' and `r'), and stores the type in *type and the
 * length in *len.
 *
 * Returns
 *    E_OK on success
 *    error code otherwise
 */
static int parse_format_string(char *format_string, AttrType *type, int *len)
{
   int n;
   char c;

   /* extract the components of the format string */
   n = sscanf(format_string, "%c%d", &c, len);

   /* if no length given... */
   if(n == 1){

      switch(c){
         case 'i':
            *type = INT;
            *len = sizeof(int);
            break;
         case 'f':
         case 'r':
            *type = FLOAT;
            *len = sizeof(float);
            break;
         case 's':
         case 'c':
            return E_NOLENGTH;
         default:
            return E_INVFORMATSTRING;
      }
   }

   /* if both are given, make sure the length is valid */
   else if(n == 2){

      switch(c){
         case 'i':
            *type = INT;
            if(*len != sizeof(int))
               return E_INVINTSIZE;
            break;
         case 'f':
         case 'r':
            *type = FLOAT;
            if(*len != sizeof(float))
               return E_INVREALSIZE;
            break;
         case 's':
         case 'c':
            *type = STRING;
            if(*len < 1 || *len > MAXSTRINGLEN)
               return E_INVSTRLEN;
            break;
         default:
            return E_INVFORMATSTRING;
      }
   }

   /* otherwise it's not a valid format string */
   else
      return E_INVFORMATSTRING;

   return E_OK;
}

/*
 * print_error: prints an error message corresponding to errval
 */
static void print_error(char *errmsg, RC errval)
{
   if(errmsg != NULL)
      fprintf(stderr, "%s: ", errmsg);
   switch(errval){
      case E_OK:
         fprintf(ERRFP, "no error\n");
         break;
      case E_INCOMPATIBLE:
         fprintf(ERRFP, "attributes must be from selected relation(s)\n");
         break;
      case E_TOOMANY:
         fprintf(ERRFP, "too many elements\n");
         break;
      case E_NOLENGTH:
         fprintf(ERRFP, "length must be specified for STRING attribute\n");
         break;
      case E_INVINTSIZE:
         fprintf(ERRFP, "invalid size for INTEGER attribute (should be %d)\n",
               (int)sizeof(int));
         break;
      case E_INVREALSIZE:
         fprintf(ERRFP, "invalid size for REAL attribute (should be %d)\n",
               (int)sizeof(real));
         break;
      case E_INVFORMATSTRING:
         fprintf(ERRFP, "invalid format string\n");
         break;
      case E_INVSTRLEN:
         fprintf(ERRFP, "invalid length for string attribute\n");
         break;
      case E_DUPLICATEATTR:
         fprintf(ERRFP, "duplicated attribute name\n");
         break;
      case E_TOOLONG:
         fprintf(stderr, "relation name or attribute name too long\n");
         break;
      case E_STRINGTOOLONG:
         fprintf(stderr, "string attribute too long\n");
         break;
      default:
         fprintf(ERRFP, "unrecognized errval: %d\n", errval);
   }
}

static void echo_query(NODE *n)
{
   switch(n -> kind){
      case N_CREATETABLE:            /* for CreateTable() */
         printf("create table %s (", n -> u.CREATETABLE.relname);
         print_attrtypes(n -> u.CREATETABLE.attrlist);
         printf(")");
         printf(";\n");
         break;
      case N_CREATEINDEX:            /* for CreateIndex() */
         printf("create index %s(%s);\n", n -> u.CREATEINDEX.relname,
               n -> u.CREATEINDEX.attrname);
         break;
      case N_DROPINDEX:            /* for DropIndex() */
         printf("drop index %s(%s);\n", n -> u.DROPINDEX.relname,
               n -> u.DROPINDEX.attrname);
         break;
      case N_DROPTABLE:            /* for DropTable() */
         printf("drop table %s;\n", n -> u.DROPTABLE.relname);
         break;
      case N_LOAD:            /* for Load() */
         printf("load %s(\"%s\");\n",
               n -> u.LOAD.relname, n -> u.LOAD.filename);
         break;
      case N_HELP:            /* for Help() */
         printf("help");
         if(n -> u.HELP.relname != NULL)
            printf(" %s", n -> u.HELP.relname);
         printf(";\n");
         break;
      case N_PRINT:            /* for Print() */
         printf("print %s;\n", n -> u.PRINT.relname);
         break;
      case N_SET:                                 /* for Set() */
         printf("set %s = \"%s\";\n", n->u.SET.paramName, n->u.SET.string);
         break;
      case N_QUERY:            /* for Query() */
         printf("select ");
         print_relattrs(n -> u.QUERY.relattrlist);
         printf("\n from ");
         print_relations(n -> u.QUERY.rellist);
         printf("\n");
         if (n->u.QUERY.conditionlist) {
            printf("where ");
            print_conditions(n->u.QUERY.conditionlist);
         }
         printf(";\n");
         break;
      case N_INSERT:            /* for Insert() */
         printf("insert into %s values ( ",n->u.INSERT.relname);
         print_values(n -> u.INSERT.valuelist);
         printf(");\n");
         break;
      case N_DELETE:            /* for Delete() */
         printf("delete %s ",n->u.DELETE.relname);
         if (n->u.DELETE.conditionlist) {
            printf("where ");
            print_conditions(n->u.DELETE.conditionlist);
         }
         printf(";\n");
         break;
      case N_UPDATE:            /* for Update() */
         {
            printf("update %s set ",n->u.UPDATE.relname);
            print_relattr(n->u.UPDATE.relattr);
            printf(" = ");
            struct node *rhs = n->u.UPDATE.relorvalue;

            /* The RHS can be either a relation.attribute or a value */
            if (rhs->u.RELATTR_OR_VALUE.relattr) {
               /* Print out the relation.attribute */
               print_relattr(rhs->u.RELATTR_OR_VALUE.relattr);
            } else {
               /* Print out the value */
               print_value(rhs->u.RELATTR_OR_VALUE.value);
            }
            if (n->u.UPDATE.conditionlist) {
               printf("where ");
               print_conditions(n->u.UPDATE.conditionlist);
            }
            printf(";\n");
            break;
         }
      default:   // should never get here
         break;
   }
   fflush(stdout);
}

static void print_attrtypes(NODE *n)
{
   NODE *attr;

   for(; n != NULL; n = n -> u.LIST.next){
      attr = n -> u.LIST.curr;
      printf("%s = %s", attr -> u.ATTRTYPE.attrname, attr -> u.ATTRTYPE.type);
      if(n -> u.LIST.next != NULL)
         printf(", ");
   }
}

static void print_op(CompOp op)
{
   switch(op){
      case EQ_OP:
         printf(" =");
         break;
      case NE_OP:
         printf(" <>");
         break;
      case LT_OP:
         printf(" <");
         break;
      case LE_OP:
         printf(" <=");
         break;
      case GT_OP:
         printf(" >");
         break;
      case GE_OP:
         printf(" >=");
         break;
      case NO_OP:
         printf(" NO_OP");
         break;
   }
}

static void print_relattr(NODE *n)
{
   printf(" ");
   if (n->u.RELATTR.relname)
      printf("%s.",n->u.RELATTR.relname);
   printf("%s",n->u.RELATTR.attrname);
}  

static void print_value(NODE *n)
{
   switch(n -> u.VALUE.type){
      case INT:
         printf(" %d", n -> u.VALUE.ival);
         break;
      case FLOAT:
         printf(" %f", n -> u.VALUE.rval);
         break;
      case STRING:
         printf(" \"%s\"", n -> u.VALUE.sval);
         break;
   }
}

static void print_condition(NODE *n)
{
   print_relattr(n->u.CONDITION.lhsRelattr);
   print_op(n->u.CONDITION.op);
   if (n->u.CONDITION.rhsRelattr)
      print_relattr(n->u.CONDITION.rhsRelattr);
   else
      print_value(n->u.CONDITION.rhsValue);
}

static void print_relattrs(NODE *n)
{
   for(; n != NULL; n = n -> u.LIST.next){
      print_relattr(n->u.LIST.curr);
      if(n -> u.LIST.next != NULL)
         printf(",");
   }
}

static void print_relations(NODE *n)
{
   for(; n != NULL; n = n -> u.LIST.next){
      printf(" %s", n->u.LIST.curr->u.RELATION.relname);
      if(n -> u.LIST.next != NULL)
         printf(",");
   }
}

static void print_conditions(NODE *n)
{
   for(; n != NULL; n = n -> u.LIST.next){
      print_condition(n->u.LIST.curr);
      if(n -> u.LIST.next != NULL)
         printf(" and");
   }
}

static void print_values(NODE *n)
{
   for(; n != NULL; n = n -> u.LIST.next){
      print_value(n->u.LIST.curr);
      if(n -> u.LIST.next != NULL)
         printf(",");
   }
}

