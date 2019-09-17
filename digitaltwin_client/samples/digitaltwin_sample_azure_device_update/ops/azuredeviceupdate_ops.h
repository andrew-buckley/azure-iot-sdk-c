// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef AZUREDEVICEUPDATE_OPS_H
#define AZUREDEVICEUPDATE_OPS_H

#ifndef RESULT_OK
#define RESULT_OK 0
#endif
#ifndef RESULT_FAILURE
#define RESULT_FAILURE -1
#endif

#ifdef __cplusplus
extern "C"
{
#endif

int ADU_serializeCharData(const char* returnValue, char** result);
int ADU_serializeIntegerData(const int returnValue, char** result);
int ADU_serializeStringArrayData(const char** returnValue, char** result, int arraySize);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // AZUREDEVICEUPDATE_OPS_H