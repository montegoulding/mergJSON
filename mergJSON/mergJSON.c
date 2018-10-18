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

/*
 
 These definitions make working with the desktop externals sdk just bearable.
 Many are from Trever DeVore's SSL library but I've added a few of my own.
 
 */

#define LIVECODE_FUNCTION(x) void x(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
#define LIVECODE_ERROR(x) { *r_err = True; 		*r_pass = False; 		*r_result = strdup(x); 		return; }

#define LIVECODE_WRITEVARIABLE(var,number) { \
SetVariable(p_arguments[ number ],var, &success); \
if (success == EXTERNAL_FAILURE) { \
LIVECODE_ERROR("Could not write variable"); \
} \
}

#define LIVECODE_ARG(argn) { if(p_argument_count < argn) { 	LIVECODE_ERROR("Incorrect number of arguments"); }}

#define LIVECODE_RETURN_THIS_STRING(x) { \
*r_err = False; *r_pass = False; \
if (x == NULL) { *r_result = strdup(""); } \
else { *r_result = x; }}


/* calling function must free the return val */
char * getPrimitiveString(json_t * tJSON) {
    char * tReturn = NULL;
    switch (json_typeof(tJSON))
    {
        case JSON_TRUE:
            tReturn = strdup("true");
            break;
        case JSON_FALSE:
            tReturn = strdup("false");
            break;
        case JSON_NULL:
            tReturn = strdup("null");
            break;
        case JSON_INTEGER:
        {
            const int tLength = snprintf(NULL,0,"%lld",json_integer_value(tJSON))+1;
            tReturn = (char*)malloc(tLength);
            snprintf(tReturn,tLength,"%lld",json_integer_value(tJSON));
        }
            break;
        case JSON_REAL:
        {
            const int tLength = snprintf(NULL,0,"%.16g",json_real_value(tJSON))+1;
            tReturn = (char*)malloc(tLength);
            snprintf(tReturn,tLength,"%.16g",json_real_value(tJSON));
        }
            break;
        case JSON_STRING:
        {
            char * tTemp = (char *)json_string_value(tJSON);
            int t_length = strlen(tTemp)+1;
            tReturn = (char*)malloc(t_length);
            snprintf(tReturn,t_length,"%s",tTemp);
        }
            break;
        case JSON_OBJECT:
        case JSON_ARRAY:
            // should not get here
            break;
    }
    return tReturn;
    
}

json_t * getPrimitiveJSON(char * tString,const char * tForceType) {
    json_t * tJSON;
    Bool tIsString = tForceType != NULL && !strcmp(tForceType, "string");
    if (!tIsString) {
        // need to encode
        if (!strcmp(tString, "true")) { // true
            tJSON = json_true();
        } else if (!strcmp(tString, "false")) { // true
            tJSON = json_false();
        } else if (!strcmp(tString, "null")) { // true
            tJSON = json_null();
        } else {
            if (isdigit(tString[0]) || tString[0] == '-') {
                char * tEnd;
                errno = 0;
                json_int_t tIntVal = strtoll(tString,&tEnd,10);
                if (!*tEnd && !errno) {
                    tJSON = json_integer(tIntVal);
                } else {
                    errno = 0;
                    double tDoubleVal = strtod(tString,&tEnd);
                    if (!*tEnd && !errno) {
                        tJSON = json_real(tDoubleVal);
                    } else {
                        tIsString = True;
                    }
                }
            } else {
                tIsString = True;
            }
        }
    }
    if (tIsString) { // it's a string
        tJSON = json_string(tString);
    }
    
    return tJSON;
}

LIVECODE_FUNCTION(mergJSONEncode)
{
    int i;
    // parameter 1: Array or variable to encode
    // parameter 2 (optional): Object type for forcing {"object"|"string") - any other value will let the external work out how to encode the values
    // parameter 3 (optional): PrettyPrint {true|false}
    LIVECODE_ARG(1);
    
    char * tForceType = NULL;
    if (p_argument_count > 1) {
        tForceType = p_arguments[1];
    }
    
    Bool tPretty = False;
    if (p_argument_count > 2) {
        tPretty = !strcmp(p_arguments[2], "true");
    }
    int tSuccess = EXTERNAL_SUCCESS;
    int tKeycount = 0;
    json_t * tJSON;
    char * tErrorString = NULL;
    
    GetArray(p_arguments[0], &tKeycount, NULL, NULL, &tSuccess);
    if (tSuccess == EXTERNAL_FAILURE || tKeycount == 0) {
        
        char * tString = GetVariable(p_arguments[0], &tSuccess);
        if (tSuccess == EXTERNAL_SUCCESS) {
            
            // could be an empty string the user wants as object or array
            if (strlen(tString) == 0 && (tForceType != NULL && (!strcmp(tForceType, "object") || !strcmp(tForceType, "array")))) {
                if (!strcmp(tForceType, "object")) {
                    tJSON = json_object();
                } else {
                    tJSON = json_array();
                }
            } else {
                if (tString[0] == '}' && (tForceType == NULL  || strcmp(tForceType, "string"))) {
                    // it's pre-encoded
                    json_error_t tError;
                    tJSON = json_loads(tString+1, JSON_DECODE_ANY, &tError);
                    if (!tJSON) {
                        tErrorString = malloc(strlen("could not decode JSON: ")+strlen(tError.text)+1);
                        sprintf(tErrorString,"could not decode JSON: %s",tError.text);
                        free(tString);
                        LIVECODE_ERROR(tErrorString);
                    }
                } else {
                    tJSON = getPrimitiveJSON(tString,tForceType);
                    if (!tJSON) {
                        free(tString);
                        LIVECODE_ERROR("could not encode value");
                    }
                }
            }
            free(tString);
            
        } else {
            LIVECODE_ERROR("could not read variable");
        }
        
    } else {
        
        ExternalString *tArray = (ExternalString *)malloc(sizeof(ExternalString) * tKeycount);
        char ** tKeys = (char **)malloc(sizeof(char *) * tKeycount);
        
        GetArray(p_arguments[0], &tKeycount, tArray, tKeys, &tSuccess);
        if (tSuccess == EXTERNAL_FAILURE)
        {
            free(tArray);
            free(tKeys);
            LIVECODE_ERROR("could not read variable");
        }
        
        // NEED TO MAINTAIN ARRAY ORDER IN JSON ARRAYS BUT LC ARRAY KEYS ARE OUT OF NUMERIC ORDER
        Bool tIsArray = (tForceType == NULL  || strcmp(tForceType, "object"));
        int *tKeyMap = NULL;
        if (tIsArray)
        {
            tKeyMap = (int *)malloc(sizeof(int) * tKeycount);
        }
        
        int tKey;
        
        if (tIsArray) {
            Bool *tCheckMap = (Bool *)malloc(sizeof(Bool) * tKeycount);
            for (i=0; i<tKeycount; i++) {
                if (isdigit(tKeys[i][0])) {
                    char * tEnd;
                    errno = 0;
                    tKey = (int) strtol(tKeys[i],&tEnd,10);
                    if ((!*tEnd && !errno) && (tKey > 0 && tKey <= tKeycount)) {
                        tKeyMap[tKey-1] = i;
                        tCheckMap[tKey-1] = True;
                    } else {
                        tIsArray = False;
                        break;
                    }
                } else {
                    tIsArray = False;
                    break;
                }
            }
            
            // This makes sure that the keys start with 1 -> tKeycount
            
            if (tIsArray) {
                for (i=0;i<tKeycount;i++) {
                    if (!tCheckMap[i]) {
                        tIsArray = False;
                        break;
                    }
                }
            }
            
            free(tCheckMap);
        }
        if (tIsArray) {
            tJSON = json_array();
        } else {
            tJSON = json_object();
        }
        int tKeyIndex;
        
        for (i=0; i<tKeycount; i++) {
            if (tIsArray) {
                tKeyIndex = tKeyMap[i];
            } else {
                tKeyIndex = i;
            }
            json_t * tKeyJSON;
            json_error_t tError;
            if (!tArray[tKeyIndex].buffer) {
                tKeyJSON = json_string("");
            } else {
                if (tArray[tKeyIndex].buffer[0] == '}') {
                    // it' either being forced to be string or it's already encoded
                    if (tArray[tKeyIndex].buffer[1] == '}') {
                        char * tString = (char *)malloc(tArray[tKeyIndex].length-1);
                        memcpy(tString, tArray[tKeyIndex].buffer+2, tArray[tKeyIndex].length-2);
                        tString[tArray[tKeyIndex].length-2] = 0;
                        tKeyJSON = json_string(tString);
                        free(tString);
                        if (!tKeyJSON)
                        {
                            if (tKeyMap != NULL)
                            {
                                free(tKeyMap);
                            }
                            free(tArray);
                            free(tKeys);
                            LIVECODE_ERROR("could not encode value in array element");
                        }
                    } else {
                        // it's pre-encoded so we skip the first char from the buffer
                        tKeyJSON = json_loadb(tArray[tKeyIndex].buffer+1, tArray[tKeyIndex].length-1, JSON_DECODE_ANY, &tError);
                        if (!tKeyJSON) {
                            tErrorString = malloc(strlen("could not decode JSON in array element: ")+strlen(tError.text)+1);
                            json_decref(tJSON);
                            if (tKeyMap != NULL)
                            {
                                free(tKeyMap);
                            }
                            free(tArray);
                            free(tKeys);
                            LIVECODE_ERROR(tErrorString);
                        }
                    }
                } else {
                    // need to encode primitive
                    char * tString = (char *)malloc(tArray[tKeyIndex].length+1);
                    memcpy(tString, tArray[tKeyIndex].buffer, tArray[tKeyIndex].length);
                    tString[tArray[tKeyIndex].length] = 0;
                    tKeyJSON = getPrimitiveJSON(tString,tForceType);
                    free(tString);
                    if (!tKeyJSON) {
                        if (tKeyMap != NULL)
                        {
                            free(tKeyMap);
                        }
                        free(tArray);
                        free(tKeys);
                        LIVECODE_ERROR("could not encode value in array element");
                    }
                }
           }
            if (tIsArray) {
                json_array_append_new(tJSON, tKeyJSON);
            } else {
                json_object_set_new(tJSON, tKeys[tKeyIndex], tKeyJSON);
            }
        }
        
        if (tKeyMap != NULL)
        {
            free(tKeyMap);
        }
        free(tArray);
        free(tKeys);
    }
    size_t tFlags = JSON_ENCODE_ANY;
    if (tPretty) {
        tFlags = tFlags | JSON_INDENT(2);
        tFlags = tFlags | JSON_SORT_KEYS;
    } else {
        tFlags = tFlags | JSON_COMPACT;
    }
    char * tReturn = json_dumps(tJSON, tFlags);
    json_decref(tJSON);
    
    LIVECODE_RETURN_THIS_STRING(tReturn);
}

LIVECODE_FUNCTION(mergJSONDecode)
{
    // two parameters: JSON to decode, variable name to place the result
    LIVECODE_ARG(2);
    
    int i;
    
    json_t * tJSON;
    json_error_t tError;
    
    tJSON = json_loads(p_arguments[0], JSON_DECODE_ANY | JSON_DECODE_NUMBER_AS_STRING, &tError);
    
    if (!tJSON) {
        char * tErrorString = malloc(strlen("could not decode JSON: ")+strlen(tError.text)+1);
        sprintf(tErrorString,"could not decode JSON: %s",tError.text);
        
        LIVECODE_ERROR(tErrorString);
    }
    char *tExpandableKeys;
	tExpandableKeys = (char *) malloc(sizeof(""));
    strcpy(tExpandableKeys, "");
	int tLength = 1;
    char *tExpandableKeysPtr;
    
    int success;
    int tSize;
    switch (json_typeof(tJSON)) {
        case JSON_ARRAY:
            if (json_array_size(tJSON) == 0) {
                LIVECODE_WRITEVARIABLE("", 1);
            } else {
                tSize = json_array_size(tJSON);
                ExternalString tArray[tSize];
                for (i = 0;i < tSize;i++) {
                    json_t * tValue = json_array_get(tJSON, i);
                    char * tKeyJSON;
                    if (json_is_array(tValue) || json_is_object(tValue)) {
                        tKeyJSON = json_dumps(tValue, JSON_ENCODE_ANY);
                        char * tKey = malloc(20);
                        snprintf(tKey, 20, "%d",i+1);
                        tLength = tLength+strlen(tKey)+1;
                        tExpandableKeysPtr = (char*) realloc (tExpandableKeys, tLength);
                        if (tExpandableKeysPtr != NULL) {
                            tExpandableKeys = tExpandableKeysPtr;
                            strncat(tExpandableKeys, tKey, tLength);
                            strncat(tExpandableKeys, "\n", tLength);
                        }
                        
                        free(tKey);
                    } else {
                        tKeyJSON = getPrimitiveString(tValue);
                    }
                    tArray[i].buffer = tKeyJSON;
                    tArray[i].length = (int)strlen(tKeyJSON);
                }
                SetArray(p_arguments[1], tSize, tArray, NULL, &success);
                if (success == EXTERNAL_FAILURE) {
                    LIVECODE_ERROR("could not set array");
                }
                for (i=0;i<tSize;i++) {
                    free((void *)tArray[i].buffer);
                }
                
            }
            break;
        case JSON_OBJECT:
            if (json_object_size(tJSON) == 0) {
                LIVECODE_WRITEVARIABLE("", 1);
            } else {
                tSize = json_object_size(tJSON);
                ExternalString tArray[tSize];
                char *tKeys[tSize];
                const char *tKey;
                json_t *tValue;
                int i = 0;
                json_object_foreach(tJSON, tKey, tValue) {
                    tKeys[i] = strdup(tKey);
                    char * tKeyJSON;
                    if (json_is_array(tValue) || json_is_object(tValue)) {
                        tLength = tLength+strlen(tKey)+1;
                        tKeyJSON = json_dumps(tValue, JSON_ENCODE_ANY);
                        tExpandableKeysPtr = (char*) realloc (tExpandableKeys, tLength);
                        if (tExpandableKeysPtr != NULL) {
                            tExpandableKeys = tExpandableKeysPtr;
                            strncat(tExpandableKeys, tKey, tLength);
                            strncat(tExpandableKeys, "\n", tLength);
                        }
                    } else {
                        tKeyJSON = getPrimitiveString(tValue);
                    }
                    tArray[i].buffer = tKeyJSON;
                    tArray[i].length = (int)strlen(tKeyJSON);
                    i++;
                }
                SetArray(p_arguments[1], tSize, tArray, tKeys, &success);
                if (success == EXTERNAL_FAILURE) {
                    LIVECODE_ERROR("could not set array");
                }
                
                for (i=0;i<tSize;i++) {
                    free((void *)tArray[i].buffer);
                    free(tKeys[i]);
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
    json_decref(tJSON);
    LIVECODE_RETURN_THIS_STRING(tExpandableKeys);
}


EXTERNAL_BEGIN_DECLARATIONS("mergJSON")
EXTERNAL_DECLARE_FUNCTION("mergJSONEncode", mergJSONEncode)
EXTERNAL_DECLARE_FUNCTION("mergJSONDecode", mergJSONDecode)
EXTERNAL_END_DECLARATIONS

#if __IPHONE_OS_VERSION_MIN_REQUIRED
   EXTERNAL_LIBINFO(mergJSON)
#endif
