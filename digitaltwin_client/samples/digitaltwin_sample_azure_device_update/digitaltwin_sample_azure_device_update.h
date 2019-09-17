// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Header file for sample for manipulating DigitalTwin Interface for ADU.

#ifndef DIGITALTWIN_SAMPLE_AZURE_DEVICE_UPDATE
#define DIGITALTWIN_SAMPLE_AZURE_DEVICE_UPDATE

#include "digitaltwin_interface_client.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum AZUREDEVICEUPDATE_EMULATOR_SCENARIO_TAG
{
    allFailed,
    downloadSuccessful,
    installationSuccessful,
    allSuccessful
} AZUREDEVICEUPDATE_EMULATOR_SCENARIO;

// Creates a new DIGITALTWIN_INTERFACE_CLIENT_HANDLE for this interface.
DIGITALTWIN_INTERFACE_CLIENT_HANDLE DigitalTwinSampleAzureDeviceUpdate_CreateInterface(AZUREDEVICEUPDATE_EMULATOR_SCENARIO scenario);

// Property update callback
void DigitalTwinSampleAzureDeviceUpdate_ProcessPropertyUpdate(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext);

// Closes down resources associated with ADU interface
void DigitalTwinSampleAzureDeviceUpdate_Close(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle);

#ifdef __cplusplus
}
#endif

#endif // DIGITALTWIN_SAMPLE_AZURE_DEVICE_UPDATE
