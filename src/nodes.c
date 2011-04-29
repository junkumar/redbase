/*
 * nodes.c: functions to allocate an initialize parse-tree nodes
 *
 * Authors; Dallan Quass
 *          Jan Jannink
 *
 * originally by: Mark McAuliffe, University of Wisconsin - Madison, 1991
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "redbase.h"
#include "parser_internal.h"
#include "y.tab.h"

/*
 * total number of nodes available for a given parse-tree
 */
#define MAXNODE		100

static NODE nodepool[MAXNODE];
static int nodeptr = 0;

/*
 * reset_parser: resets the scanner and parser when a syntax error occurs
 *
 * No return value
 */
void reset_parser(void)
{
    reset_scanner();
    nodeptr = 0;
}

static void (*cleanup_func)() = NULL;

/*
 * new_query: prepares for a new query by releasing all resources used
 * by previous queries.
 *
 * No return value.
 */
void new_query(void)
{
    nodeptr = 0;
    reset_charptr();
    if(cleanup_func != NULL)
	(*cleanup_func)();
}

void register_cleanup_function(void (*func)())
{
    cleanup_func = func;
}

/*
 * newnode: allocates a new node of the specified kind and returns a pointer
 * to it on success.  Returns NULL on error.
 */
NODE *newnode(NODEKIND kind)
{
    NODE *n;

    /* if we've used up all of the nodes then error */
    if(nodeptr == MAXNODE){
	fprintf(stderr, "out of memory\n");
	exit(1);
    }

    /* get the next node */
    n = nodepool + nodeptr;
    ++nodeptr;

    /* initialize the `kind' field */
    n -> kind = kind;
    return n;
}

/*
 * create_table_node: allocates, initializes, and returns a pointer to a new
 * create table node having the indicated values.
 */
NODE *create_table_node(char *relname, NODE *attrlist)
{
    NODE *n = newnode(N_CREATETABLE);

    n -> u.CREATETABLE.relname = relname;
    n -> u.CREATETABLE.attrlist = attrlist;
    return n;
}

/*
 * create_index_node: allocates, initializes, and returns a pointer to a new
 * create index node having the indicated values.
 */
NODE *create_index_node(char *relname, char *attrname)
{
    NODE *n = newnode(N_CREATEINDEX);

    n -> u.CREATEINDEX.relname = relname;
    n -> u.CREATEINDEX.attrname = attrname;
    return n;
}

/*
 * drop_index_node: allocates, initializes, and returns a pointer to a new
 * drop index node having the indicated values.
 */
NODE *drop_index_node(char *relname, char *attrname)
{
    NODE *n = newnode(N_DROPINDEX);

    n -> u.DROPINDEX.relname = relname;
    n -> u.DROPINDEX.attrname = attrname;
    return n;
}

/*
 * drop_table_node: allocates, initializes, and returns a pointer to a new
 * drop table node having the indicated values.
 */
NODE *drop_table_node(char *relname)
{
    NODE *n = newnode(N_DROPTABLE);

    n -> u.DROPTABLE.relname = relname;
    return n;
}

/*
 * load_node: allocates, initializes, and returns a pointer to a new
 * load node having the indicated values.
 */
NODE *load_node(char *relname, char *filename)
{
    NODE *n = newnode(N_LOAD);

    n -> u.LOAD.relname = relname;
    n -> u.LOAD.filename = filename;
    return n;
}

/*
 * set_node: allocates, initializes, and returns a pointer to a new
 * set node having the indicated values.
 */
NODE *set_node(char *paramName, char *string)
{
    NODE *n = newnode(N_SET);

    n -> u.SET.paramName = paramName;
    n -> u.SET.string = string;
    return n;
}

/*
 * help_node: allocates, initializes, and returns a pointer to a new
 * help node having the indicated values.
 */
NODE *help_node(char *relname)
{
    NODE *n = newnode(N_HELP);

    n -> u.HELP.relname = relname;
    return n;
}

/*
 * print_node: allocates, initializes, and returns a pointer to a new
 * print node having the indicated values.
 */
NODE *print_node(char *relname)
{
    NODE *n = newnode(N_PRINT);

    n -> u.PRINT.relname = relname;
    return n;
}

/*
 * query_node: allocates, initializes, and returns a pointer to a new
 * query node having the indicated values.
 */
NODE *query_node(NODE *relattrlist, NODE *rellist, NODE *conditionlist)
{
    NODE *n = newnode(N_QUERY);

    n->u.QUERY.relattrlist = relattrlist;
    n->u.QUERY.rellist = rellist;
    n->u.QUERY.conditionlist = conditionlist;
    return n;
}

/*
 * insert_node: allocates, initializes, and returns a pointer to a new
 * insert node having the indicated values.
 */
NODE *insert_node(char *relname, NODE *valuelist)
{
    NODE *n = newnode(N_INSERT);

    n->u.INSERT.relname = relname;
    n->u.INSERT.valuelist = valuelist;
    return n;
}

/*
 * delete_node: allocates, initializes, and returns a pointer to a new
 * delete node having the indicated values.
 */
NODE *delete_node(char *relname, NODE *conditionlist)
{
    NODE *n = newnode(N_DELETE);

    n->u.DELETE.relname = relname;
    n->u.DELETE.conditionlist = conditionlist;
    return n;
}

/*
 * update_node: allocates, initializes, and returns a pointer to a new
 * update node having the indicated values.
 */
NODE *update_node(char *relname, NODE *relattr, NODE *relorvalue, 
		  NODE *conditionlist)
{
    NODE *n = newnode(N_UPDATE);

    n->u.UPDATE.relname = relname;
    n->u.UPDATE.relattr = relattr;
    n->u.UPDATE.relorvalue = relorvalue;
    n->u.UPDATE.conditionlist = conditionlist;
    return n;
}


/*
 * relattr_node: allocates, initializes, and returns a pointer to a new
 * relattr node having the indicated values.
 */
NODE *relattr_node(char *relname, char *attrname)
{
    NODE *n = newnode(N_RELATTR);

    n -> u.RELATTR.relname = relname;
    n -> u.RELATTR.attrname = attrname;
    return n;
}

/*
 * condition_node: allocates, initializes, and returns a pointer to a new
 * condition node having the indicated values.
 */
NODE *condition_node(NODE *lhsRelattr, CompOp op, NODE *rhsRelattrOrValue)
{
    NODE *n = newnode(N_CONDITION);

    n->u.CONDITION.lhsRelattr = lhsRelattr;
    n->u.CONDITION.op = op;
    n->u.CONDITION.rhsRelattr = 
      rhsRelattrOrValue->u.RELATTR_OR_VALUE.relattr;
    n->u.CONDITION.rhsValue = 
      rhsRelattrOrValue->u.RELATTR_OR_VALUE.value;
    return n;
}

/*
 * value_node: allocates, initializes, and returns a pointer to a new
 * value node having the indicated values.
 */
NODE *value_node(AttrType type, void *value)
{
    NODE *n = newnode(N_VALUE);

    n->u.VALUE.type = type;
    switch (type) {
    case INT:
      n->u.VALUE.ival = *(int *)value;
      break;
    case FLOAT:
      n->u.VALUE.rval = *(float *)value;
      break;
    case STRING:
      n->u.VALUE.sval = (char *)value;
      break;
    }
    return n;
}

/*
 * relattr_or_valuenode: allocates, initializes, and returns a pointer to 
 * a new relattr_or_value node having the indicated values.
 */
NODE *relattr_or_value_node(NODE *relattr, NODE *value)
{
    NODE *n = newnode(N_RELATTR_OR_VALUE);

    n->u.RELATTR_OR_VALUE.relattr = relattr;
    n->u.RELATTR_OR_VALUE.value = value;
    return n;
}

/*
 * attrtype_node: allocates, initializes, and returns a pointer to a new
 * attrtype node having the indicated values.
 */
NODE *attrtype_node(char *attrname, char *type)
{
    NODE *n = newnode(N_ATTRTYPE);

    n -> u.ATTRTYPE.attrname = attrname;
    n -> u.ATTRTYPE.type = type;
    return n;
}

/*
 * relation_node: allocates, initializes, and returns a pointer to a new
 * relation node having the indicated values.
 */
NODE *relation_node(char *relname)
{
    NODE *n = newnode(N_RELATION);

    n->u.RELATION.relname = relname;
    return n;
}

/*
 * list_node: allocates, initializes, and returns a pointer to a new
 * list node having the indicated values.
 */
NODE *list_node(NODE *n)
{
    NODE *list = newnode(N_LIST);

    list -> u.LIST.curr = n;
    list -> u.LIST.next = NULL;
    return list;
}

/*
 * prepends node n onto the front of list.
 *
 * Returns the resulting list.
 */
NODE *prepend(NODE *n, NODE *list)
{
    NODE *newlist = newnode(N_LIST);

    newlist -> u.LIST.curr = n;
    newlist -> u.LIST.next = list;
    return newlist;
}
