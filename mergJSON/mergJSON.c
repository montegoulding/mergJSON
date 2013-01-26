//
//  mergJSON.c
//  mergJSON
//
//  Created by Monte Goulding on 26/01/13.
//  Copyright (c) 2013 mergExt. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "external.h"

#include "jansson.h"

#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
#include <Carbon/Carbon.h>
#else
#define snprintf sprintf_s
#include <windows.h>
#include <windef.h>
#endif

/*
 
 These definitions make working with the desktop externals sdk just bearable.
 Many are from Trever DaVore's SSL library but I've added a few of my own.
 
 */

#define LIVECODE_FUNCTION(x) void x(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
#define LIVECODE_ERROR(x) { *r_err = True; 		*r_pass = False; 		*r_result = strdup(x); 		return; }

#define LIVECODE_READARG(var, number, tmpl) if(!sscanf(p_arguments[ number ], tmpl, & var)) { 	LIVECODE_ERROR("Failed to read argument"); }

#define LIVECODE_READVARIABLE(var,number) { \
GetVariableEx(p_arguments[ number ], "", &var, &success); \
if (success == EXTERNAL_FAILURE) { \
LIVECODE_ERROR("Could not read variable"); \
} \
}

#define LIVECODE_WRITEVARIABLE(var,number) { \
SetVariableEx(p_arguments[ number ], "", &var, &success); \
if (success == EXTERNAL_FAILURE) { \
LIVECODE_ERROR("Could not write variable"); \
} \
}

#define LIVECODE_ARG(argn) { if(p_argument_count < argn) { 	LIVECODE_ERROR("Incorrect number of arguments"); }}

#define LIVECODE_NOERROR { *r_err = False; *r_pass = False; *r_result = strdup(""); }

#define LIVECODE_RETURN_THIS_STRING(x) { \
*r_err = False; *r_pass = False; \
if (x == NULL) { *r_result = strdup(""); } \
else { *r_result = x; }}

#define LIVECODE_RETURN_UNSIGNED { \
*r_err = False; *r_pass = False; \
*r_result = (char *)malloc(20); \
snprintf(*r_result, 20, "%d", result); \
}

#define LIVECODE_RETURN_UNSIGNED_LONG { \
*r_err = False; *r_pass = False; \
*r_result = (char *)malloc(sizeof(long)); \
snprintf(*r_result, sizeof(long), "%ld", result); \
}


LIVECODE_FUNCTION(mergJSONEncode)
{
    LIVECODE_ARG(3);
}

LIVECODE_FUNCTION(mergJSONDecode)
{
    LIVECODE_ARG(3);
}


EXTERNAL_BEGIN_DECLARATIONS("mergJSON")
EXTERNAL_DECLARE_COMMAND("mergJSONEncode", mergJSONEncode)
EXTERNAL_DECLARE_COMMAND("mergJSONDecode", mergJSONDecode)
EXTERNAL_END_DECLARATIONS
