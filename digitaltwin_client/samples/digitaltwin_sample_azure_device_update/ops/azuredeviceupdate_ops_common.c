// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "azuredeviceupdate_ops.h"
#include "azure_c_shared_utility/xlogging.h"

int ADU_serializeCharData(const char* returnValue, char** result)
{
    char* resultValue = NULL;

    size_t neededSize = snprintf(NULL, 0, "\"%s\"", returnValue);

    if (neededSize > 0)
    {
        resultValue = (char*)calloc(neededSize + 1, sizeof(char));
    }

    if (resultValue == NULL)
    {
        LogError("Failed to build char payload string");
        return RESULT_FAILURE;
    }
    else
    {
        snprintf(resultValue, neededSize + 1, "\"%s\"", returnValue);
    }

    *result = resultValue;
    return RESULT_OK;
}

int ADU_serializeIntegerData(const int returnValue, char** result)
{
    char* resultValue = NULL;

    size_t neededSize = snprintf(NULL, 0, "%ul", returnValue);

    if (neededSize > 0)
    {
        resultValue = (char*)calloc(neededSize + 1, sizeof(char));
    }

    if (resultValue == NULL)
    {
        LogError("Failed to build long int payload string");
        return RESULT_FAILURE;
    }
    else
    {
        snprintf(resultValue, neededSize, "%ul", returnValue);
    }

    *result = resultValue;
    return RESULT_OK;
}

int ADU_serializeStringArrayData(const char** returnValue, char** result, int arraySize)
{
    for (int i = 0; i < arraySize; i++) 
    {
        if(ADU_serializeCharData(returnValue[i], &result[i]) != RESULT_OK)
        {
            return RESULT_FAILURE;
        }
        
    }
    
    return RESULT_OK;
}