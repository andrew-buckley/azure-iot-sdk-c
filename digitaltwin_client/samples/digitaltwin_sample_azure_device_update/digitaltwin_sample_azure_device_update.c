#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#include "digitaltwin_sample_azure_device_update.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "../digitaltwin_sample_device_info/digitaltwin_sample_device_info.h"
#include "./ops/azuredeviceupdate_ops.h"
#include "parson.h"

typedef enum DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_ACTION_TAG
{
    download = 0,
    install,
    apply,
    cancel
} DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_ACTION;

typedef enum DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_STATE_TAG
{
    idle = 0,
    downloadStarted,
    downloadSucceeded,
    installStarted,
    installSucceeded,
    applyStarted,
    failed
} DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_STATE;

typedef enum DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATEFAILURE_TAG
{
    downloadFailure = 0,
    installFailure,
    applyFailure,
    failureCommanded
} DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATEFAILURE;

typedef enum DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT_TAG
{
    downloadFailedResult = 0,
    downloadSuccessfulResult,
    installationFailedResult,
    installationSuccessfulResult,
    applyFailedResult,
    applySuccessfulResult
} DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT;

typedef struct AZUREDEVICEUPDATE_EMULATOR_SCENARIO_PARAMS_TAG
{
    DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT download;
    DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT installation;
    DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT application;
} AZUREDEVICEUPDATE_EMULATOR_SCENARIO_PARAMS;

const char* FAILURE_MSG[] = { "Download Failure", "Install Failure", "Apply Failure", "Failure commanded" };

/**
 * Application state associated with the particular interface
 */
typedef struct DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE_TAG
{
    char** serializedFailures;
    int failures_size;
    char* targetVersion;
    char* files;
    DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_ACTION action;
    DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_STATE state;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceClientHandle;
    AZUREDEVICEUPDATE_EMULATOR_SCENARIO_PARAMS scenarioParameters;
    AZUREDEVICEUPDATE_EMULATOR_SCENARIO scenario;

} DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE;

// State for interface.  For simplicity we set this as a global and set during DigitalTwin_InterfaceClient_Create, but  
// callbacks of this interface don't reference it directly but instead use userContextCallback passed to them.
DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE digitaltwinSample_AzureDeviceUpdateApplicationState;

// DigitalTwin interface name from service perspective.
static const char digitaltwinSampleAzureDeviceUpdate_InterfaceId[] = "urn:azureiot:AzureDeviceUpdateCore:1";
static const char digitaltwinSampleAzureDeviceUpdate_InterfaceName[] = "azureDeviceUpdateCore";

// Device to cloud (read-only) property and field names (Client)
static const char digitaltwinSample_ClientProperty[] = "Client";
static const char digitaltwinSample_StateField[] = "State";
static const char digitaltwinSample_ResultCodeField[] = "ResultCode";
static const char digitaltwinSample_ExtendedResultCodeField[] = "ExtendedResultCode";

// Cloud to device (writable) property and field names (CloudOrchestrator)
static const char digitaltwinSample_OrchestratorProperty[] = "Orchestrator";
static const char digitaltwinSample_TargetVersionField[] = "TargetVersion";
static const char digitaltwinSample_ActionField[] = "Action";
static const char digitaltwinSample_FilesField[] = "Files";

void wait(int seconds)
{
    #ifdef _WIN32
    Sleep(1000*seconds);
    #else
    sleep(seconds);
    #endif
}

AZUREDEVICEUPDATE_EMULATOR_SCENARIO_PARAMS DigitalTwinSampleAzureDeviceUpdate_InitializeSimulationScenario(AZUREDEVICEUPDATE_EMULATOR_SCENARIO scenario)
{
    AZUREDEVICEUPDATE_EMULATOR_SCENARIO_PARAMS scenarioParameters = {downloadFailedResult, installationFailedResult, applyFailedResult};
    switch (scenario)
    {
        case allFailed:
            scenarioParameters.download = downloadFailedResult;
            scenarioParameters.installation = installationFailedResult;
            scenarioParameters.application = applyFailedResult;
            break;
        case downloadSuccessful:
            scenarioParameters.download = downloadSuccessfulResult;
            scenarioParameters.installation = installationFailedResult;
            scenarioParameters.application = applyFailedResult;
            break;
        case installationSuccessful:
            scenarioParameters.download = downloadSuccessfulResult;
            scenarioParameters.installation = installationSuccessfulResult;
            scenarioParameters.application = applyFailedResult;
            break;
        case allSuccessful:
            scenarioParameters.download = downloadSuccessfulResult;
            scenarioParameters.installation = installationSuccessfulResult;
            scenarioParameters.application = applySuccessfulResult;
            break;
        default:
            LogError("AZURE_DEVICE_UPDATE: Failed to initialize simulation scenario %d", scenario);
            break;
    }
    return scenarioParameters;
}

int DigitalTwinSampleAzureDeviceUpdate_SerializeData(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE *applicationState)
{
    //applicationState->actions_size =  sizeof(ACTIONS)/sizeof(ACTIONS[0]);
    //applicationState->states_size =  sizeof(STATES)/sizeof(STATES[0]);
    applicationState->failures_size =  sizeof(FAILURE_MSG)/sizeof(FAILURE_MSG[0]);

   // const char** actions_ptr = ACTIONS;
    //const char** states_ptr = STATES;
    const char** failures_ptr = FAILURE_MSG;

    //char **serialized_actions = (char**)malloc(5*sizeof(char*));
    //char **serialized_states = (char**)malloc(5*sizeof(char*));
    char **serialized_failures = (char**)malloc(5*sizeof(char*));

    // generate proper JSON payload
    if (/*(ADU_serializeStringArrayData(actions_ptr, serialized_actions, applicationState->actions_size) == RESULT_OK) && */
        /*(ADU_serializeStringArrayData(states_ptr, serialized_states, applicationState->states_size) == RESULT_OK) && */
        (ADU_serializeStringArrayData(failures_ptr, serialized_failures, applicationState->failures_size) == RESULT_OK))
    {
        LogInfo("AZURE_DEVICE_UPDATE: Serialized ACTIONS, STATES, and FAILURES");
        //applicationState->serializedActions = serialized_actions;
        //applicationState->serializedStates = serialized_states;
        applicationState->serializedFailures = serialized_failures;
    }
    else
    {
        LogError("AZURE_DEVICE_UPDATE: Failed to serialize ACTIONS or STATES or FAILURES arrays");
        return RESULT_FAILURE;
    }

    return RESULT_OK;
}

// DigitalTwinSampleDeviceInfo_PropertyCallback is invoked when a property is updated (or failed) going to server.
// In this sample, we route ALL property callbacks to this function and just have the userContextCallback set
// to the propertyName.  Product code will potentially have context stored in this userContextCallback.
void DigitalTwinSampleAzureDeviceUpdate_PropertyCallback(DIGITALTWIN_CLIENT_RESULT dtReportedStatus, void* userContextCallback)
{
    if (dtReportedStatus == DIGITALTWIN_CLIENT_OK)
    {
        LogInfo("AZURE_DEVICE_UPDATE: Property callback property=<%s> succeeds", (const char*)userContextCallback);
    }
    else
    {
        LogError("AZURE_DEVICE_UPDATE: Property callback property=<%s> fails, error=<%s>",  (const char*)userContextCallback, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtReportedStatus));
    }
}

//
// DigitalTwinSampleAzureDeviceUpdate_ReportProperty is a helper function to report an AzureDeviceUpdate's properties.
// It invokes underlying DigitalTwin API for reporting properties and sets up its callback on completion.
//
static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleAzureDeviceUpdate_ReportPropertyWithoutResponse(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle, const char* propertyName, const char* propertyData)
{
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(interfaceHandle, propertyName, 
                                                             propertyData, NULL,
                                                             DigitalTwinSampleAzureDeviceUpdate_PropertyCallback, (void*)propertyName);

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("AZURE_DEVICE_UPDATE: Reporting property=<%s> failed, error=<%s>", propertyName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
    }
    else
    {
        LogInfo("AZURE_DEVICE_UPDATE: Queued async report property for %s", propertyName);
    }

    return result;
}

//
// DigitalTwinSampleAzureDeviceUpdate_ReportProperty is a helper function to report an AzureDeviceUpdate's properties.
// It invokes underlying DigitalTwin API for reporting properties and sets up its callback on completion.
//
static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleAzureDeviceUpdate_ReportProperty(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle, const char* propertyName, const char* propertyData, const DIGITALTWIN_CLIENT_PROPERTY_RESPONSE* dtResponse)
{
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(interfaceHandle, propertyName, 
                                                             propertyData, dtResponse,
                                                             DigitalTwinSampleAzureDeviceUpdate_PropertyCallback, (void*)propertyName);

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("AZURE_DEVICE_UPDATE: Reporting property=<%s> failed, error=<%s>", propertyName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
    }
    else
    {
        LogInfo("AZURE_DEVICE_UPDATE: Queued async report property for %s", propertyName);
    }

    return result;
}

static void ReportClientHelperWithoutResponse(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE* applicationState, const char* fieldName, int value)
{
    // Report the new update state property
    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);
    char* serialized_string = NULL;
    json_object_set_number(root_object, fieldName, value);
    serialized_string = json_serialize_to_string(root_value);

    DigitalTwinSampleAzureDeviceUpdate_ReportPropertyWithoutResponse(applicationState->interfaceClientHandle, digitaltwinSample_ClientProperty,
        serialized_string);

    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}

//
// DigitalTwinSampleAzureDeviceUpdate_SimulateUpdateProcesses simulates n update Download process requested through the UpdateCommand property by the cloud.
// It returns an update result (DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT).
//
static DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT DigitalTwinSampleAzureDeviceUpdate_SimulateDownload(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT simulationResult)
{
    LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: I am downloading...");
    wait(5);
    if (simulationResult == downloadSuccessfulResult)
    {
        LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Download successful.");
    } 
    else
    {
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Download failed.");
    }
    return simulationResult;
}

//
// DigitalTwinSampleAzureDeviceUpdate_SimulateUpdateProcesses simulates an update Installation process requested through the UpdateCommand property by the cloud.
// It returns an update result (DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT).
//
static DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT DigitalTwinSampleAzureDeviceUpdate_SimulateInstallation(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT simulationResult)
{
    LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: I am installing...");
    wait(10);
    if (simulationResult == installationSuccessfulResult)
    {
        LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Installation successful.");
    } 
    else
    {
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Installation failed.");
    }
    return simulationResult;
}

//
// DigitalTwinSampleAzureDeviceUpdate_SimulateUpdateProcesses simulates an update Application process requested through the UpdateCommand property by the cloud.
// It returns an update result (DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT).
//
static DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT DigitalTwinSampleAzureDeviceUpdate_SimulateApplication(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT simulationResult)
{
    LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: I am applying...");
    wait(5);
    if (simulationResult == applySuccessfulResult)
    {
        LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Application successful.");
    } 
    else
    {
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Application failed.");
    }
    return simulationResult;
}

//
// DigitalTwinSampleAzureDeviceUpdate_StateMachine_Fail implements the Fail logic of the Update State Machine by reporting its State, ErrorCode, and Message properties to the Twin. 
//
static void DigitalTwinSampleAzureDeviceUpdate_StateMachine_Fail(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE *applicationState, int resultCode, int extendedResultCode)
{
    applicationState->state = failed;
    ReportClientHelperWithoutResponse(applicationState, digitaltwinSample_StateField, applicationState->state);

    ReportClientHelperWithoutResponse(applicationState, digitaltwinSample_ResultCodeField, resultCode);

    ReportClientHelperWithoutResponse(applicationState, digitaltwinSample_ExtendedResultCodeField, extendedResultCode);
}

//
// DigitalTwinSampleAzureDeviceUpdate_StateMachine_Download implements the Download logic of the Update State Machine by validating the UpdateName, TargetVersion, UpdateManifest, and Files properties and initiating the "Download Started" state if valid. 
// If Download Failed, Update State Machine goes into "Failed" state and "Download Completed" state if Download succeeded.
//
static int DigitalTwinSampleAzureDeviceUpdate_StateMachine_Download(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE* applicationState)
{
    int result = 0;
    DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT updateResult;

     // Go to "Download Started" state only if currently in "Idle" state and properties UpdateName, TargetVersion, UpdateManifest, and Files are valid.
    if (applicationState->state == idle && applicationState->targetVersion != NULL && applicationState->files != NULL)
    {
        applicationState->state = downloadStarted;

        // Report the new update state property
        ReportClientHelperWithoutResponse(applicationState, digitaltwinSample_StateField, applicationState->state);

        // TODO: Process properties
        LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Target Version is %s", applicationState->targetVersion);
        LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Files are %s", applicationState->files);

        // Initiate Download
        updateResult = DigitalTwinSampleAzureDeviceUpdate_SimulateDownload(applicationState->scenarioParameters.download);

        if (updateResult == downloadFailedResult)
        {
            LogError("AZURE_DEVICE_UPDATE_INTERFACE: Download failed with failure=<%s>", applicationState->serializedFailures[downloadFailure]);
            int resultCode = 502;
            int extendedResultCode = 900;
            DigitalTwinSampleAzureDeviceUpdate_StateMachine_Fail(applicationState, resultCode, extendedResultCode);
        }
        else if (updateResult == downloadSuccessfulResult)
        {
            applicationState->state = downloadSucceeded;
            LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Download succeeded with state=<%d>", applicationState->state);
            // Report the new update state property
            ReportClientHelperWithoutResponse(applicationState, digitaltwinSample_StateField, applicationState->state);
        }
        else
        {
            result = MU_FAILURE;
        }
    }
    else
    {
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Make sure STATE is idle and TargetVersion or Files are not NULL");
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Update State is %d", applicationState->state);
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Target Version is %s", applicationState->targetVersion);
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Files are %s", applicationState->files);
        result = MU_FAILURE;
    }

    return result;
}

//
// DigitalTwinSampleAzureDeviceUpdate_StateMachine_Install implements the Install logic of the Update State Machine and initiates the "Installation Started" state. 
// If Installation Failed, Update State Machine goes into "Failed" state and "Installation Completed" state if Installation succeeded.
//
static int DigitalTwinSampleAzureDeviceUpdate_StateMachine_Install(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE *applicationState)
{
    int result = 0;
    DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT updateResult;

    // Go to "Installation Started" state only if currently in "Download Completed" state.
    if (applicationState->state == downloadSucceeded)
    {
        applicationState->state = installStarted;
        ReportClientHelperWithoutResponse(applicationState, digitaltwinSample_StateField, applicationState->state);

        // Initiate Install
        updateResult = DigitalTwinSampleAzureDeviceUpdate_SimulateInstallation(applicationState->scenarioParameters.installation);

        if (updateResult == installationFailedResult)
        {
            LogError("AZURE_DEVICE_UPDATE_INTERFACE: Installation failed with failure=<%s>", applicationState->serializedFailures[installFailure]);
            int resultCode = 503;
            int extendedResultCode = 900;
            DigitalTwinSampleAzureDeviceUpdate_StateMachine_Fail(applicationState, resultCode, extendedResultCode);
        }
        else if (updateResult == installationSuccessfulResult)
        {
            applicationState->state = installSucceeded;
            LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Installation succeeded with state=<%d>", applicationState->state);
            ReportClientHelperWithoutResponse(applicationState, digitaltwinSample_StateField, applicationState->state);

        }
        else 
        {
            result = MU_FAILURE;
        }
    }
    else
    {
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: not in downloadcompleted state, aborting...");
        result = MU_FAILURE;
    }

    return result;
}

//
// DigitalTwinSampleAzureDeviceUpdate_StateMachine_Apply implements the Apply logic of the Update State Machine and initiates the "Apply Started" state.
// If Apply Failed, Update State Machine goes into "Failed" state and "Idle" state if Apply succeeded. 
//
static int DigitalTwinSampleAzureDeviceUpdate_StateMachine_Apply(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE *applicationState)
{
    int result = 0;
    DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_UPDATERESULT updateResult;

    // Go to "Apply Started" state only if currently in "Installation Completed" state.
    if (applicationState->state == installSucceeded)
    {
        applicationState->state = applyStarted;
        ReportClientHelperWithoutResponse(applicationState, digitaltwinSample_StateField, applicationState->state);

        // Initiate Apply
        updateResult = DigitalTwinSampleAzureDeviceUpdate_SimulateApplication(applicationState->scenarioParameters.application);

        if (updateResult == applyFailedResult)
        {
            LogError("AZURE_DEVICE_UPDATE_INTERFACE: Application failed with failure=: %s", applicationState->serializedFailures[applyFailure]);
            int resultCode = 504;
            int extendedResultCode = 900;
            DigitalTwinSampleAzureDeviceUpdate_StateMachine_Fail(applicationState, resultCode, extendedResultCode);
        }
        else if (updateResult == applySuccessfulResult)
        {
            applicationState->state = idle;
            LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Application succeeded with state=<%d>", applicationState->state);
            ReportClientHelperWithoutResponse(applicationState, digitaltwinSample_StateField, applicationState->state);

            /** Update "swVersion" of the Device Information interface. 
            TODO: Note that this procedure wouldn't be necessary on a real device
            */
            JSON_Value* root_value = json_value_init_string(applicationState->targetVersion);
            char* serialized_string = json_serialize_to_string(root_value);
            LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Calling DeviceInformation interface to update property swVersion with <%s>", serialized_string);
            DeviceInfo_updateSwVersion(serialized_string);
            json_free_serialized_string(serialized_string);
            json_value_free(root_value);
        }
        else 
        {
            result = MU_FAILURE;
        }
    }
    else
    {
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: not in installationcompleted state, aborting...");
        result = MU_FAILURE;
    }

    return result;
}

//
// DigitalTwinSampleAzureDeviceUpdate_StateMachine_Cancel implements the Cancel logic of the Update State Machine by returning to the "Idle" state under any conditions. 
//
static void DigitalTwinSampleAzureDeviceUpdate_StateMachine_Cancel(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE *applicationState)
{
    applicationState->state = idle;
    ReportClientHelperWithoutResponse(applicationState, digitaltwinSample_StateField, applicationState->state);
}

//
// DigitalTwinSampleAzureDeviceUpdate_ValidateAction validates an Action property update (Download, Install, Apply, or Cancel)
//
static int DigitalTwinSampleAzureDeviceUpdate_ValidateAction(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE* applicationState, DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_ACTION action)
{
    int result = 0;

    // Parsing if the desired Action is a valid Enum value
    if (action > cancel || action < 0)
    {
        LogError("Could not parse data=<%d> invalid", (int) action);
        result = MU_FAILURE;
    }
    else
    {
        applicationState->action = action;
    }

    return result;
}

//
// DigitalTwinSampleAzureDeviceUpdate_ProcessAction parses an Action property update and initiates approapriate Update State Machine logic (Download, Install, Apply, Fail, or Cancel)
//
static int DigitalTwinSampleAzureDeviceUpdate_ProcessAction(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE* applicationState)
{
    int result = 0;

    switch (applicationState->action)
    {
        case download:
            applicationState->action = download;
            LogInfo("AZURE_DEVICE_UPDATE: current state=<%d>", applicationState->state);
            result = DigitalTwinSampleAzureDeviceUpdate_StateMachine_Download(applicationState);
            break;
        case install:
            applicationState->action = install;
            LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: current state=<%d>", applicationState->state);
            result = DigitalTwinSampleAzureDeviceUpdate_StateMachine_Install(applicationState);
            break;
        case apply:
            applicationState->action = apply;
            LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: current state=<%d>", applicationState->state);
            result = DigitalTwinSampleAzureDeviceUpdate_StateMachine_Apply(applicationState);
            break;
        case cancel:
            applicationState->action = cancel;
            LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: current state=<%d>", applicationState->state);
            DigitalTwinSampleAzureDeviceUpdate_StateMachine_Cancel(applicationState);
            break;
        default:
            result = MU_FAILURE;
            break;
    }

    return result;
}

// DigitalTwinSampleAzureDeviceUpdate_InterfaceRegisteredCallback is invoked when this interface
// is successfully or unsuccessfully registered with the service, and also when the interface is deleted.
static void DigitalTwinSampleAzureDeviceUpdate_InterfaceRegisteredCallback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void* userInterfaceContext)
{
    DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE *applicationState = (DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE*)userInterfaceContext;
    if (dtInterfaceStatus == DIGITALTWIN_CLIENT_OK)
    {
        // Once the interface is registered, send our reported properties to the service.  
        // It *IS* safe to invoke most DigitalTwin API calls from a callback thread like this, though it 
        // is NOT safe to create/destroy/register interfaces now.
        LogInfo("AZURE_DEVICE_UPDATE: Interface successfully registered.");

        // serialize legal JSON payloads
        // TODO: Might not need
        DigitalTwinSampleAzureDeviceUpdate_SerializeData(applicationState);

        // Create initial update client json string
        JSON_Value* root_value = json_value_init_object();
        JSON_Object* root_object = json_value_get_object(root_value);
        char* serialized_string = NULL;
        json_object_set_number(root_object, digitaltwinSample_StateField, idle);
        json_object_set_number(root_object, digitaltwinSample_ResultCodeField, 0);
        json_object_set_number(root_object, digitaltwinSample_ExtendedResultCodeField, 0);
        serialized_string = json_serialize_to_string(root_value);

        //Initialize device in "Idle" state
        DigitalTwinSampleAzureDeviceUpdate_ReportPropertyWithoutResponse(applicationState->interfaceClientHandle, digitaltwinSample_ClientProperty,
                                                      serialized_string);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
        
        applicationState->state = idle;
    }
    else if (dtInterfaceStatus == DIGITALTWIN_CLIENT_ERROR_INTERFACE_UNREGISTERING)
    {
        // Once an interface is marked as unregistered, it cannot be used for any DigitalTwin SDK calls.
        LogInfo("AZURE_DEVICE_UPDATE: Interface received unregistering callback.");
    }
    else 
    {
        LogError("AZURE_DEVICE_UPDATE: Interface received failed, status=<%s>.", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus));
    }
}

//
// Process a property field update, which the server initiated, for the TargetVersion field.
//
static void DigitalTwinSampleAzureDeviceUpdate_TargetVersionPropertyCallback(const char* targetVersion, const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE* applicationState)
{
    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;

    LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Orchestrator property field TargetValue invoked with data=<%.*s>", (int) sizeof(targetVersion), targetVersion);

    propertyResponse.version = DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1;
    propertyResponse.responseVersion = dtClientPropertyUpdate->desiredVersion;

    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);
    char* partial_property_update_string = NULL;
    json_object_set_string(root_object, digitaltwinSample_TargetVersionField, targetVersion);
    partial_property_update_string = json_serialize_to_string(root_value);

    free(applicationState->targetVersion);
    if (mallocAndStrcpy_s(&(applicationState->targetVersion), targetVersion) != 0)
    {
        // Indicates failure with additional human readable information about status.
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Out of memory updating TargetVersion.");
        propertyResponse.statusCode = 500;
        propertyResponse.statusDescription = "Out of memory";
    }
    else
    {
        // Indicates success with optional additional human readable information about status.
        LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: TargetVersion successfully updated.");
        propertyResponse.statusCode = 200;
        propertyResponse.statusDescription = "Property updated successfully";
    }

    DigitalTwinSampleAzureDeviceUpdate_ReportProperty(applicationState->interfaceClientHandle, digitaltwinSample_OrchestratorProperty, partial_property_update_string, &propertyResponse);
    json_free_serialized_string(partial_property_update_string);
    json_value_free(root_value);
}

static void DigitalTwinSampleAzureDeviceUpdate_ActionPropertyCallback(DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_ACTION action, const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE* applicationState)
{
    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;

    LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Orchestrator property field Action invoked with data=<%d>", action);

    propertyResponse.version = DIGITALTWIN_CLIENT_PROPERTY_UPDATE_VERSION_1;
    propertyResponse.responseVersion = dtClientPropertyUpdate->desiredVersion;

    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);
    char* partial_property_update_string = NULL;
    json_object_set_number(root_object, digitaltwinSample_ActionField, action);
    partial_property_update_string = json_serialize_to_string(root_value);

    // Process the action 
    if (DigitalTwinSampleAzureDeviceUpdate_ValidateAction(applicationState, action) != 0)
    {
        // Indicates failure with additional human readable information about status.
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Invalid Action=<%d> specified", action);
        propertyResponse.statusCode = 505;
        propertyResponse.statusDescription = "Invalid Action";
    }
    else if (DigitalTwinSampleAzureDeviceUpdate_ProcessAction(applicationState) != 0)
    {
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Invalid Action=<%d> specified", action);
        propertyResponse.statusCode = 506;
        propertyResponse.statusDescription = "Error processing Action";
    }
    else
    {
        // Indicates success with optional additional human readable information about status.
        LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Action successfully updated.");
        propertyResponse.statusCode = 200;
        propertyResponse.statusDescription = "Property updated successfully";
    }

    DigitalTwinSampleAzureDeviceUpdate_ReportProperty(applicationState->interfaceClientHandle, digitaltwinSample_OrchestratorProperty, partial_property_update_string, &propertyResponse);
    json_free_serialized_string(partial_property_update_string);
    json_value_free(root_value);
}

static void DigitalTwinSampleAzureDeviceUpdate_FilesPropertyCallback(JSON_Value* files, const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE* applicationState)
{
    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;
    char* files_serialized = json_serialize_to_string(files);

    LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Orchestrator property field Files invoked with data=<%s>", files_serialized);

    propertyResponse.version = DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1;
    propertyResponse.responseVersion = dtClientPropertyUpdate->desiredVersion;

    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);
    char* partial_property_update_string = NULL;
    json_object_set_value(root_object, digitaltwinSample_FilesField, files);
    partial_property_update_string = json_serialize_to_string(root_value);

    free(applicationState->files);
    if (mallocAndStrcpy_s(&(applicationState->files), files_serialized) != 0)
    {
        // Indicates failure with additional human readable information about status.
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Out of memory updating Files.");
        propertyResponse.statusCode = 500;
        propertyResponse.statusDescription = "Out of memory";
    }
    else
    {
        // Indicates success with optional additional human readable information about status.
        LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Files successfully updated.");
        propertyResponse.statusCode = 200;
        propertyResponse.statusDescription = "Property updated successfully";
    }

    DigitalTwinSampleAzureDeviceUpdate_ReportProperty(applicationState->interfaceClientHandle, digitaltwinSample_OrchestratorProperty, partial_property_update_string, &propertyResponse);
    json_free_serialized_string(files_serialized);
    json_free_serialized_string(partial_property_update_string);
    json_value_free(root_value);
}

// 
// Process a property update, which the server initiated, for the Orchestrator property.
//
static void DigitalTwinSampleAzureDeviceUpdate_UpdateOrchestratorPropertyCallback(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext)
{
    DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE* applicationState = (DIGITALTWIN_SAMPLE_AZUREDEVICEUPDATE_APPLICATIONSTATE*)userInterfaceContext;
    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;

    LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Orchestrator property invoked with data=<%.*s>", (int)dtClientPropertyUpdate->propertyDesiredLen, dtClientPropertyUpdate->propertyDesired);

    // Version of this structure for C SDK.
    propertyResponse.version = DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1;
    propertyResponse.responseVersion = dtClientPropertyUpdate->desiredVersion;

    // Get all properties
    JSON_Value* root_value = json_parse_string((const char*)(dtClientPropertyUpdate->propertyDesired));
    JSON_Object* root_object = json_value_get_object(root_value);

    // target version
    const char* targetVersion = NULL;
    if((targetVersion = json_object_get_string(root_object, digitaltwinSample_TargetVersionField)) != NULL)
    {
        DigitalTwinSampleAzureDeviceUpdate_TargetVersionPropertyCallback(targetVersion, dtClientPropertyUpdate, applicationState);
    }

    // files
    JSON_Value* files = NULL;
    if (json_object_has_value(root_object, digitaltwinSample_FilesField))
    {
        files = json_object_get_value(root_object, digitaltwinSample_FilesField);
        DigitalTwinSampleAzureDeviceUpdate_FilesPropertyCallback(files, dtClientPropertyUpdate, applicationState);
    }

    // update action
    double action = -1;
    if (json_object_has_value(root_object, digitaltwinSample_ActionField))
    {
        action = json_object_get_number(root_object, digitaltwinSample_ActionField);
        DigitalTwinSampleAzureDeviceUpdate_ActionPropertyCallback(action, dtClientPropertyUpdate, applicationState);
    }

    json_value_free(root_value);

    // Update all properties
    // Update target version
    // free(state->targetVersion);
    // if ((state->targetVersion = (char*)malloc(dtClientPropertyUpdate->proper)))
}

//
// Processes property updates and initiates specific property update callbacks
//
void DigitalTwinSampleAzureDeviceUpdate_ProcessPropertyUpdate(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext)
{
    const char *propertyName = (const char*) dtClientPropertyUpdate->propertyName;    
    const char *propertyDesired = (const char*) dtClientPropertyUpdate->propertyDesired;
    LogInfo("AZURE_DEVICE_UPDATE_INTERFACE: Property callback succeeds. Updating desired propertyName=<%s> with value=<%s>", propertyName, propertyDesired);

    if (strcmp(dtClientPropertyUpdate->propertyName, digitaltwinSample_OrchestratorProperty) == 0)
    {
        DigitalTwinSampleAzureDeviceUpdate_UpdateOrchestratorPropertyCallback(dtClientPropertyUpdate, userInterfaceContext);
    }
    else
    {
        // If the property is not implemented by this interface, presently we only record a log message but do not have a mechanism to report back to the service
        LogError("AZURE_DEVICE_UPDATE_INTERFACE: Property name <%s> is not associated with this interface", dtClientPropertyUpdate->propertyName);
    }
}

DIGITALTWIN_INTERFACE_CLIENT_HANDLE DigitalTwinSampleAzureDeviceUpdate_CreateInterface(AZUREDEVICEUPDATE_EMULATOR_SCENARIO scenario)
{
    DIGITALTWIN_CLIENT_RESULT result;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle;

    memset(&digitaltwinSample_AzureDeviceUpdateApplicationState, 0, sizeof(digitaltwinSample_AzureDeviceUpdateApplicationState));

    // Initialize this device emulator with the desired parameters for a specific scenario to simulate
    digitaltwinSample_AzureDeviceUpdateApplicationState.scenarioParameters = DigitalTwinSampleAzureDeviceUpdate_InitializeSimulationScenario(scenario);

    if ((result = DigitalTwin_InterfaceClient_Create(digitaltwinSampleAzureDeviceUpdate_InterfaceId, 
        digitaltwinSampleAzureDeviceUpdate_InterfaceName, 
        DigitalTwinSampleAzureDeviceUpdate_InterfaceRegisteredCallback, 
        &digitaltwinSample_AzureDeviceUpdateApplicationState, &interfaceHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("AZURE_DEVICE_UPDATE: Unable to allocate interface client handle for interfaceId=<%s>, interfaceName=<%s>, error=<%s>", digitaltwinSampleAzureDeviceUpdate_InterfaceId, digitaltwinSampleAzureDeviceUpdate_InterfaceName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        interfaceHandle = NULL;
    }
    else if ((result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(interfaceHandle, DigitalTwinSampleAzureDeviceUpdate_ProcessPropertyUpdate)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("AZURE_DEVICE_UPDATE: DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback failed. error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        DigitalTwinSampleAzureDeviceUpdate_Close(interfaceHandle);
        interfaceHandle = NULL;
    }
    else
    {
        digitaltwinSample_AzureDeviceUpdateApplicationState.interfaceClientHandle = interfaceHandle;
        LogInfo("AZURE_DEVICE_UPDATE: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE.  interfaceId=<%s>, interfaceName=<%s>, handle=<%p>", digitaltwinSampleAzureDeviceUpdate_InterfaceId, digitaltwinSampleAzureDeviceUpdate_InterfaceName, interfaceHandle);
    }

    return interfaceHandle;
}

// DigitalTwinSampleAzureDeviceUpdate_Close is invoked when the sample device is shutting down.
// Although currently this just frees the interfaceHandle, had the interface
// been more complex this would also be a chance to cleanup additional resources it created.
void DigitalTwinSampleAzureDeviceUpdate_Close(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    // On shutdown, in general the first call made should be to DigitalTwin_InterfaceClient_Destroy.
    // This will block if there are any active callbacks in this interface, and then
    // mark the underlying handle such that no future callbacks shall come to it.
    DigitalTwin_InterfaceClient_Destroy(interfaceHandle);

    // After DigitalTwin_InterfaceClient_Destroy returns, it is safe to assume
    // no more callbacks shall arrive for this interface and it is OK to free
    // resources callbacks otherwise may have needed.
    // This is a no-op in this simple sample.

    free(digitaltwinSample_AzureDeviceUpdateApplicationState.targetVersion);
    digitaltwinSample_AzureDeviceUpdateApplicationState.targetVersion = NULL;

    free(digitaltwinSample_AzureDeviceUpdateApplicationState.files);
    digitaltwinSample_AzureDeviceUpdateApplicationState.files = NULL;
}