%{
/*
 * scan.l: lex spec for RQL
 *
 * Authors: Dallan Quass
 *          Jan Jannink
 *
 * originally by: Mark McAuliffe, University of Wisconsin - Madison, 1991
 */

#include <string.h>
#include "redbase.h"          /* parse.h needs the definition of real */
#include "parser_internal.h"  /* y.tab.h needs the definition of NODE */
#include "y.tab.h"

static int get_id(char *s);   /* defined in scanhelp.c */
static char *get_qstring(char *qstring, int len);

%}
letter               [A-Za-z]
digit                [0-9]
num                  {digit}+
s_num                [+\-]?{num}
%x comment
%x shell_cmd
%%
"/*"                 {BEGIN(comment);}
<comment>[^*]        {/* ignore the text of the comment */}
<comment>"*/"        {BEGIN(INITIAL);}
<comment>\*          {/* ignore *'s that aren't part of */}
[ \n\t]              {/* ignore spaces, tabs, and newlines */}
{s_num}              {sscanf(yytext, "%d", &yylval.ival);
                      return T_INT;}
{s_num}\.{num}       {sscanf(yytext, "%f", &yylval.rval);
                      return T_REAL;}
{s_num}\.{num}[Ee]{s_num}   {sscanf(yytext, "%f", &yylval.rval);
                             return T_REAL;}
\"([^\"\n]|(\"\"))*\"       {yylval.sval = get_qstring(yytext, yyleng);
                             return T_QSTRING;}
\"([^\"\n]|(\"\"))*\n       {printf("newline in string constant\n");}
{letter}({letter}|{digit}|_)*   {return get_id(yylval.sval = yytext);}
"<"                  {return T_LT;}
"<="                 {return T_LE;}
">"                  {return T_GT;}
">="                 {return T_GE;}
"="                  {return T_EQ;}
"!="                 {return T_NE;}
"<>"                 {return T_NE;}
!                    {BEGIN(shell_cmd);}
<shell_cmd>[^\n]*    {yylval.sval = yytext; return T_SHELL_CMD;}
<shell_cmd>\n        {BEGIN(INITIAL);}
[*/+\-=<>':;,.|&()]  {return yytext[0];}
<<EOF>>              {return T_EOF;}
.                    {printf("illegal character [%c]\n", yytext[0]);}
%%
#include "scanhelp.c"
