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
#include <ctype.h>
#include <errno.h>

#include "external.h"

#include "jansson.h"

#ifndef __MAC_OS_X_VERSION_MIN_REQUIRED
#define snprintf sprintf_s
#endif

/*
 
 These definitions make working with the desktop externals sdk just bearable.
 Many are from Trever DaVore's SSL library but I've added a few of my own.
 
 */

#define LIVECODE_FUNCTION(x) void x(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
#define LIVECODE_ERROR(x) { *r_err = True; 		*r_pass = False; 		*r_result = strdup(x); 		return; }

#define LIVECODE_READARG(var, number, tmpl) if(!sscanf(p_arguments[ number ], tmpl, & var)) { 	LIVECODE_ERROR("Failed to read argument"); }

#define LIVECODE_READVARIABLE(var,number) { \
GetVariable(p_arguments[ number ], &success); \
if (success == EXTERNAL_FAILURE) { \
LIVECODE_ERROR("Could not read variable"); \
} \
}

#define LIVECODE_WRITEVARIABLE(var,number) { \
SetVariable(p_arguments[ number ],var, &success); \
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

/* calling function must free the return val */
char * getPrimitiveString(json_t * tJSON) {
    char * tReturn;
    switch (json_typeof(tJSON))
    {
        case JSON_TRUE:
            tReturn = (char*)malloc(5);
            tReturn = strdup("true");
            break;
        case JSON_FALSE:
            tReturn = (char*)malloc(6);
            tReturn = strdup("false");
            break;
        case JSON_NULL:
            tReturn = (char*)malloc(5);
            tReturn = strdup("null");
            break;
        case JSON_INTEGER:
        {
            tReturn = (char*)malloc(sizeof(long long));
            snprintf(tReturn,sizeof(long long),"%lld",json_integer_value(tJSON));
        }
            break;
        case JSON_REAL:
        {
            tReturn = (char*)malloc(sizeof(double));
            snprintf(tReturn,sizeof(double),"%lf",json_real_value(tJSON));
        }
            break;
        case JSON_STRING:
        {
            char * tTemp = (char *)json_string_value(tJSON);
            tReturn = (char*)malloc(strlen(tTemp)+1);
            tReturn = strdup(tTemp);
        }
            break;
        case JSON_OBJECT:
        case JSON_ARRAY:
            // should not get here
            break; 
    }
    return tReturn;
    
}

json_t * getPrimitiveJSON(char * tString,char * tForceType) {
    json_t * tJSON;
    if (tString[0] == '}') {
        
    }
    if (!strcmp(tString, "{}")) { // empty object
        tJSON = json_object();
    } else if (!strcmp(tString, "[]")) { // empty array
        tJSON = json_array();
    } else if (!strcmp(tString, "true")) { // true
        tJSON = json_true();
    } else if (!strcmp(tString, "false")) { // true
        tJSON = json_false();
    } else if (!strcmp(tString, "null")) { // true
        tJSON = json_null();
    } else {
        Bool tIsString = False;
        if (isdigit(tString[0])) {
            char * tEnd;
            json_int_t tIntVal = strtoll(tString,&tEnd,10);
            if (!*tEnd && !errno) {
                tJSON = json_integer(tIntVal);
            }
            double tDoubleVal = strtod(tString,&tEnd);
            if (!*tEnd && !errno) {
                tJSON = json_real(tDoubleVal);
            } else {
                tIsString = True;
            }
        } else {
            tIsString = True;
        }
        if (tIsString) { // it's a string
            tJSON = json_string(tString);
        }
    }
    return tJSON;
}

LIVECODE_FUNCTION(mergJSONEncode)
{
    // one parameter: Array or variable to encode
    LIVECODE_ARG(1);
    
    char * tForceType;
    if (p_argument_count > 1) {
        LIVECODE_READARG(tForceType, 1, "%s");
    }
    
    Bool tPretty = False;
    if (p_argument_count > 2) {
        tPretty = !strcmp(p_arguments[1], "true");
    }
    int tSuccess;
    int tKeycount = 0;
    json_t * tJSON;
    char * tErrorString;
    
    GetArray(p_arguments[0], &tKeycount, NULL, NULL, &tSuccess);
    if (tSuccess == EXTERNAL_FAILURE || tKeycount == 0) {
        
        char * tString = GetVariable(p_arguments[0], &tSuccess);
        if (tSuccess == EXTERNAL_SUCCESS) {
            tJSON = getPrimitiveJSON(tString,tForceType);
            free(tString);
        } else {
             LIVECODE_ERROR("could not read variable");
        }
        
    } else {
        
        ExternalString tArray[tKeycount];
        char * tKeys[tKeycount];
        GetArray(p_arguments[0], &tKeycount, tArray, tKeys, &tSuccess);
        if (tSuccess == EXTERNAL_FAILURE) LIVECODE_ERROR("could not read variable");
        
        // NEED TO MAINTAIN ARRAY ORDER IN JSON ARRAYS BUT LC ARRAY KEYS ARE OUT OF NUMERIC ORDER
        Bool tIsArray = True;
        int tKeyMap[tKeycount];
        Bool tCheckMap[tKeycount];
        int tKey;
        for (int i=0; i<tKeycount; i++) {
            if (sscanf(tKeys[i],"%d",&tKey)==1) {
                if (tKey > 0 && tKey <= tKeycount) {
                    tKeyMap[tKey-1] = i;
                    tCheckMap[tKey-1] = True;
                } else {
                    tIsArray = False;
                }
            } else {
                tIsArray = False;
            }
        }
        if (tIsArray) {
            for (int i=0;i<tKeycount;i++) {
                if (!tCheckMap[i]) {
                    tIsArray = False;
                }
            }
        }
        if (tIsArray) {
            tJSON = json_array();
        } else {
            tJSON = json_object();
        }
        int tKeyIndex;
        
        // Use an error flag so we can do any required cleanup
        Bool tDecodingError = False;
        
        for (int i=0; i<tKeycount; i++) {
            if (!tDecodingError) {
                if (tIsArray) {
                    tKeyIndex = tKeyMap[i];
                } else {
                    tKeyIndex = i;
                }
                json_t * tKeyJSON;
                json_error_t tError;
                tKeyJSON = json_loadb(tArray[tKeyIndex].buffer, tArray[tKeyIndex].length, JSON_DECODE_ANY, &tError);
                if (!tKeyJSON) {
                    tErrorString = malloc(strlen("could not decode JSON in array element: ")+strlen(tError.text)+1);
                    sprintf(tErrorString,"could not decode JSON in array element: %s",tError.text);
                    tDecodingError = True;
                }
                //                char * tString = (char *)malloc(tArray[tKeyIndex].length+1);
                //                memcpy(tString, tArray[tKeyIndex].buffer, tArray[tKeyIndex].length);
                //                ((char*)tString)[tArray[tKeyIndex].length] = 0;
                //                tKeyJSON = getPrimitiveJSON(tString);
                //                free(tString);
                if (tIsArray) {
                    json_array_append_new(tJSON, tKeyJSON);
                } else {
                    json_object_set_new(tJSON, tKeys[tKeyIndex], tKeyJSON);
                }
            }
        }
        if (tDecodingError) {
            json_decref(tJSON);
            LIVECODE_ERROR(tErrorString);
        }
        
    }
    size_t tFlags = JSON_ENCODE_ANY;
    if (tPretty) {
        tFlags = tFlags | JSON_INDENT(2);
    } else {
        tFlags = tFlags | JSON_COMPACT;
    }
    char * tReturn = json_dumps(tJSON, tFlags);
    json_decref(tJSON);
    
    LIVECODE_RETURN_THIS_STRING(tReturn);
}

LIVECODE_FUNCTION(mergJSONDecode)
{
    // three parameters: JSON to decode, variable name to place the result, variable name to place keys to be expanded
    LIVECODE_ARG(3);
    
    json_t * tJSON;
    json_error_t tError;
    
    tJSON = json_loads(p_arguments[0], JSON_DECODE_ANY, &tError);
    
    if (!tJSON) {
        char * tErrorString = malloc(strlen("could not decode JSON: ")+strlen(tError.text)+1);
        sprintf(tErrorString,"could not decode JSON: %s",tError.text);
        
        LIVECODE_ERROR(tErrorString);
    }
    char *tExpandableKeys;
	tExpandableKeys = (char *) malloc(sizeof(""));
    strcpy(tExpandableKeys, "");
	int tLength = strlen(tExpandableKeys)+1;
    char *tExpandableKeysPtr;
    
    int success;
    
    switch (json_typeof(tJSON)) {
        case JSON_ARRAY:
            if (json_array_size(tJSON) == 0) {
                LIVECODE_WRITEVARIABLE("[]", 1);
            } else {
                ExternalString tArray[json_array_size(tJSON)];
                for (int i = 0;i < json_array_size(tJSON);i++) {
                    json_t * tValue = json_array_get(tJSON, i);
                    char * tKeyJSON;
                    if (json_is_array(tValue) || json_is_object(tValue)) {
                        tKeyJSON = json_dumps(tValue, JSON_ENCODE_ANY);
                        char * tKey = malloc(20);
                        snprintf(tKey, 20, "%d",i+1);
                        tLength += strlen(tKey)+1;
                        tExpandableKeysPtr = (char*) realloc (tExpandableKeys, tLength);
                        if (tExpandableKeysPtr != NULL) {
                            tExpandableKeys = tExpandableKeysPtr;
                            snprintf(tExpandableKeys, tLength,"%s%s\n",tExpandableKeys,tKey);
                        }
                    } else {
                        tKeyJSON = getPrimitiveString(tValue);
                    }
                    tArray[i].buffer = tKeyJSON;
                    tArray[i].length = (int)strlen(tKeyJSON);
                    
                }
                SetArray(p_arguments[1], (int) json_array_size(tJSON), tArray, NULL, &success);
                if (success == EXTERNAL_FAILURE) {
                    LIVECODE_ERROR("could not set array");
                }
                for (int i=1;i<json_object_size(tJSON);i++) {
                    free((void *)tArray[i].buffer);
                }
                
                SetVariable(p_arguments[2], tExpandableKeys, &success);
                free(tExpandableKeys);
                if (success == EXTERNAL_FAILURE) {
                    LIVECODE_ERROR("could not set expandable keys");
                }
             }
            break;
        case JSON_OBJECT:
            if (json_object_size(tJSON) == 0) {
                LIVECODE_WRITEVARIABLE("{}", 1);
            } else {
                ExternalString tArray[json_object_size(tJSON)];
                char *tKeys[json_object_size(tJSON)];
                const char *tKey;
                json_t *tValue;
                int i = 0;
                json_object_foreach(tJSON, tKey, tValue) {
                    tKeys[i] = strdup(tKey);
                    tLength += strlen(tKey)+1;
                    char * tKeyJSON;
                    if (json_is_array(tValue) || json_is_object(tValue)) {
                        tKeyJSON = json_dumps(tValue, JSON_ENCODE_ANY);
                        tExpandableKeysPtr = (char*) realloc (tExpandableKeys, tLength);
                        if (tExpandableKeysPtr != NULL) {
                            tExpandableKeys = tExpandableKeysPtr;
                            snprintf(tExpandableKeys, tLength,"%s%s\n",tExpandableKeys,tKey);
                        }
                    } else {
                        tKeyJSON = getPrimitiveString(tValue);
                    }
                    tArray[i].buffer = tKeyJSON;
                    tArray[i].length = (int)strlen(tKeyJSON);
                    
                    i++;
                }
                SetArray(p_arguments[1], (int) json_object_size(tJSON), tArray, tKeys, &success);
                if (success == EXTERNAL_FAILURE) {
                    LIVECODE_ERROR("could not set array");
                }
                
                for (int i=1;i<json_object_size(tJSON);i++) {
                    free((void *)tArray[i].buffer);
                }
                
                SetVariable(p_arguments[2], tExpandableKeys, &success);
                free(tExpandableKeys);
                if (success == EXTERNAL_FAILURE) {
                    LIVECODE_ERROR("could not set expandable keys");
                }
            }
            break;
        default:
        {
            char * tTemp = getPrimitiveString(tJSON);
            LIVECODE_WRITEVARIABLE(tTemp, 1);
            free(tTemp);
        }
            break;
    }
    if (!json_is_array(tJSON) && !json_is_object(tJSON)) {
        SetVariable(p_arguments[2], tExpandableKeys, &success);
        if (success == EXTERNAL_FAILURE) {
            LIVECODE_ERROR("could not set expandable keys");
        }
        free(tExpandableKeys);
    }
    json_decref(tJSON);
    LIVECODE_NOERROR;
}


EXTERNAL_BEGIN_DECLARATIONS("mergJSON")
EXTERNAL_DECLARE_FUNCTION("mergJSONEncode", mergJSONEncode)
EXTERNAL_DECLARE_COMMAND("mergJSONDecode", mergJSONDecode)
EXTERNAL_END_DECLARATIONS
