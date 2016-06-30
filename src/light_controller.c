/***************************************************************************************************
 * Copyright (c) 2016, Imagination Technologies Limited and/or its affiliated group companies
 * and/or licensors
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 *    and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file light_controller.c
 * @brief Light controller application observes the IPSO resource for motion sensor on constrained device.
 *        On receipt of AwaLWM2M notification, ci40 will glow on board led for a period of 5 seconds.
 *        If further notifications are observed during the 5 second period then time out is restarted.
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "awa/server.h"
#include "log.h"

/***************************************************************************************************
 * Definitions
 **************************************************************************************************/

/** Calculate size of array. */
#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

//! @cond Doxygen_Suppress
#define IPC_SERVER_PORT             (54321)
#define IP_ADDRESS                  "127.0.0.1"
#define MOTION_STR                  "SensorValue"
#define MOTION_DEVICE_STR           "MotionSensorDevice"
#define MOTION_OBJECT_ID            (3302)
#define MOTION_RESOURCE_ID          (5501)
#define MIN_INSTANCES               (0)
#define MAX_INSTANCES               (1)
#define OPERATION_TIMEOUT           (5000)
#define URL_PATH_SIZE               (16)
#define ALARM_PERIOD                (5)
#define LED_OFF                     "0"
#define LED_ON                      "1"
#define SENSOR_LED_INDEX            "1"
#define HEARTBEAT_LED_INDEX         "2"
//! @endcond

/***************************************************************************************************
 * Typedef
 **************************************************************************************************/

/**
 * A structure to contain resource information.
 */
typedef struct
{
    /*@{*/
    AwaResourceID id; /**< resource ID */
    AwaResourceInstanceID instanceID; /**< resource instance ID */
    AwaResourceType type; /**< type of resource e.g. bool, string, integer etc. */
    const char *name; /**< resource name */
    /*@}*/
}Resource;

/**
 * A structure to contain objects information.
 */
typedef struct
{
    /*@{*/
    char *clientID; /**< client ID */
    AwaObjectID id; /**< object ID */
    AwaObjectInstanceID instanceID; /**< object instance ID */
    const char *name; /**< object name */
    unsigned int numResources; /**< number of resource under this object */
    Resource *resources; /**< resource information */
    /*@}*/
}Object;

/***************************************************************************************************
 * Globals
 **************************************************************************************************/

/** Set default debug level to info. */
int g_debugLevel = LOG_INFO;
/** Set default debug stream to NULL. */
FILE *g_debugStream = NULL;
/** Sensor state on motion sensor constrained device. */
unsigned int g_sensorState = false;
/** Global variable for signal handling. */
static volatile int g_quit = 0;

/** Initializing objects. */
static Object objects[] =
{
    {
        MOTION_DEVICE_STR,
        MOTION_OBJECT_ID,
        0,
        "IlluminanceSensor",
        1,
        (Resource []){
                            {
                                MOTION_RESOURCE_ID,
                                0,
                                AwaResourceType_Integer,
                                MOTION_STR
                            },
                      }
    },
};

/***************************************************************************************************
 * Implementation
 **************************************************************************************************/

/**
 * @brief Signal handler for segfault and Ctrl-C handling.
 * @param dummy dummy variable
 */
static void CtrlCSignalHandler(int dummy)
{
    LOG(LOG_INFO, "Exit triggered");
    g_quit = 1;
}

/**
 * @brief Update heartbeat led status to on/off.
 * @param status led status.
 * @param isHeartbeat update for heartbeat led or sensor.
 */
static void UpdateLed(bool status, bool isHeartbeat)
{
    int tmp = 0;

    if (status)
    {
        if (isHeartbeat)
        {
            tmp = system("/usr/bin/set_led.sh " HEARTBEAT_LED_INDEX " " LED_ON);
        }
        else
        {
            tmp = system("/usr/bin/set_led.sh " SENSOR_LED_INDEX " " LED_ON);
        }
    }
    else
    {
        if (isHeartbeat)
        {
            tmp = system("/usr/bin/set_led.sh " HEARTBEAT_LED_INDEX " " LED_OFF);
        }
        else
        {
            tmp = system("/usr/bin/set_led.sh " SENSOR_LED_INDEX " " LED_OFF);
        }
    }

    if (tmp != 0)
    {
        LOG(LOG_WARN, "Setting led failed.");
    }
}

/**
 * @brief Prints light_controller_appd usage.
 * @param *program holds application name.
 */
static void PrintUsage(const char *program)
{
    printf("Usage: %s [options]\n\n"
            " -l : Log filename.\n"
            " -v : Debug level from 1 to 5\n"
            "      fatal(1), error(2), warning(3), info(4), debug(5) and max(>5)\n"
            "      default is info.\n"
            " -h : Print help and exit.\n\n",
            program);
}

/**
 * @brief Parses command line arguments passed to light_controller_appd.
 * @return -1 in case of failure, 0 for printing help and exit, and 1 for success.
 */
static int ParseCommandArgs(int argc, char *argv[], const char **fptr)
{
    int opt, tmp;
    opterr = 0;

    while (1)
    {
        opt = getopt(argc, argv, "l:v:");
        if (opt == -1)
        {
            break;
        }

        switch (opt)
        {
            case 'l':
                *fptr = optarg;
                break;
            case 'v':
                tmp = strtoul(optarg, NULL, 0);
                if (tmp >= LOG_FATAL && tmp <= LOG_DBG)
                {
                    g_debugLevel = tmp;
                }
                else
                {
                    LOG(LOG_ERR, "Invalid debug level");
                    PrintUsage(argv[0]);
                    return -1;
                }
                break;
            case 'h':
                PrintUsage(argv[0]);
                return 0;
            default:
                PrintUsage(argv[0]);
                return -1;
        }
    }

    return 1;
}

/**
 * @brief Turn off the light when 5 second timer run out signal is received.
 */
void TurnOffLight(int signal)
{
    UpdateLed(false, false);
	LOG(LOG_INFO, "Turn OFF led on Ci40 board");
}

/**
 * @brief Turn on the light when notification is received.
 */
void TurnOnLight()
{
	signal(SIGALRM, TurnOffLight);
    UpdateLed(true, false);
	LOG(LOG_INFO, "Turn ON led on Ci40 board\n");
	alarm(ALARM_PERIOD);
}

/**
 * @brief Observe callback gets called when there is change in sensor status.
 * @param *context a pointer to any data passed from callback registration function.
 * @param *changeSet a pointer to a valid ChangeSet.
 */
void ObserveCallback(const AwaChangeSet *changeSet, void *context)
{
    char path[URL_PATH_SIZE] = {0};
    const AwaInteger *value = NULL;
    const AwaServerSession *serverSession = AwaChangeSet_GetServerSession(changeSet);

    if (AwaAPI_MakeResourcePath(path, URL_PATH_SIZE, MOTION_OBJECT_ID, 0, MOTION_RESOURCE_ID) != AwaError_Success)
    {
        LOG(LOG_INFO, "Couldn't generate all object and resource paths.");
    }

    if (path != NULL)
    {
        AwaObjectID objectID = AWA_INVALID_ID;
        AwaObjectInstanceID objectInstanceID = AWA_INVALID_ID;
        AwaResourceID resourceID = AWA_INVALID_ID;

        AwaServerSession_PathToIDs(serverSession, path, &objectID, &objectInstanceID, &resourceID);

        if (AwaChangeSet_GetValueAsIntegerPointer(changeSet, path, &value) == AwaError_Success)
        {
            g_sensorState = *value;
            LOG(LOG_INFO, "Received observe callback for sensor object[%d/0/%d] with value %d", MOTION_OBJECT_ID, MOTION_RESOURCE_ID, g_sensorState);
        }
    }
}

/**
 * @brief Observe sensor status on server and call for update in case of changes.
 * @param *session holds server session.
 * @return true if observing sensor has been set successfully, else false.
 */
static bool StartObservingSensor(const AwaServerSession *session)
{
    AwaServerObserveOperation *operation = NULL;
    char sensorResourcePath[URL_PATH_SIZE] = {0};
    const AwaPathResult *pathResult = NULL;
    bool result = true;

    operation = AwaServerObserveOperation_New(session);
    if (operation == NULL)
    {
        LOG(LOG_ERR, "Failed to create observe operation");
        return false;
    }

    if (AwaAPI_MakeResourcePath(sensorResourcePath, URL_PATH_SIZE, MOTION_OBJECT_ID, 0, MOTION_RESOURCE_ID) != AwaError_Success)
    {
        LOG(LOG_INFO, "Couldn't generate all object and resource paths");
        return false;
    }

    AwaServerObservation *observation = AwaServerObservation_New(MOTION_DEVICE_STR, sensorResourcePath, ObserveCallback, NULL);

    if (observation != NULL)
    {
        if (AwaServerObserveOperation_AddObservation(operation, observation) != AwaError_Success)
        {
            LOG(LOG_ERR, "AwaServerObserveOperation_AddObservation failed");
            result = false;
        }

        if (AwaServerObserveOperation_Perform(operation, OPERATION_TIMEOUT) != AwaError_Success)
        {
            LOG(LOG_ERR, "Failed to perform observe operation");
            result = false;
        }

        const AwaServerObserveResponse *response = NULL;
        response = AwaServerObserveOperation_GetResponse(operation, MOTION_DEVICE_STR);

        pathResult = AwaServerObserveResponse_GetPathResult(response, sensorResourcePath);
        if (AwaPathResult_GetError(pathResult) != AwaError_Success)
        {
            LOG(LOG_ERR, "AwaServerObserveResponse_GetPathResult failed\n");
            result = false;
        }

        LOG(LOG_INFO, "Successfully added observe operation for sensor object[%d/0/%d]", MOTION_OBJECT_ID, MOTION_RESOURCE_ID);

        AwaServerObserveOperation_Free(&operation);
    }

    return result;
}

/**
 * @brief Check to see if a constrained device by the name endPointName has registered
 *        itself with the server on the gateway or not.
 * @param *session holds server session.
 * @param *endPointName holds client name.
 * @return true if constrained device is in client list i.e. registered, else false.
 */
static bool CheckConstrainedRegistered(const AwaServerSession *session, const char *endPointName)
{
    bool result = false;
    AwaError error;

    AwaServerListClientsOperation *operation = AwaServerListClientsOperation_New(session);
    if (operation != NULL)
    {
        if ((error = AwaServerListClientsOperation_Perform(operation, OPERATION_TIMEOUT)) == AwaError_Success)
        {
            AwaClientIterator *clientIterator = NULL;
            clientIterator = AwaServerListClientsOperation_NewClientIterator(operation);
            if (clientIterator != NULL)
            {
                while(AwaClientIterator_Next(clientIterator))
                {
                    const char *clientID = AwaClientIterator_GetClientID(clientIterator);

                    if (!strcmp(endPointName, clientID))
                    {
                        LOG(LOG_INFO, "Constrained device %s registered", endPointName);
                        result = true;
                        break;
                    }
                }
                AwaClientIterator_Free(&clientIterator);
            }
            else
            {
                LOG(LOG_ERR, "AwaServerListClientsOperation_NewClientIterator failed");
            }
        }
        else
        {
            LOG(LOG_ERR, "AwaServerListClientsOperation_Perform failed\nerror: %s", AwaError_ToString(error));
        }

        if ((error = AwaServerListClientsOperation_Free(&operation)) != AwaError_Success)
        {
            LOG(LOG_ERR, "AwaServerListClientsOperation_Free failed\nerror: %s", AwaError_ToString(error));
        }
    }
    else
    {
        LOG(LOG_ERR, "AwaServerListClientsOperation_New failed");
    }
    return result;
}

/**
 * @brief Add all resource definitions belongs to object.
 * @param *object whose resources are to be defined.
 * @return pointer to flow object definition.
 */
static AwaObjectDefinition *AddResourceDefinitions(Object *object)
{
    int i;

    AwaObjectDefinition *objectDefinition = AwaObjectDefinition_New(object->id, object->name, MIN_INSTANCES, MAX_INSTANCES);
    if (objectDefinition != NULL)
    {
        // define resources
        for (i = 0; i < object->numResources; i++)
        {
            if (object->resources[i].type == AwaResourceType_Integer)
            {
                if( AwaObjectDefinition_AddResourceDefinitionAsInteger(
                                                                objectDefinition,
                                                                object->resources[i].id,
                                                                object->resources[i].name,
                                                                true,
                                                                AwaResourceOperations_ReadWrite,
                                                                0) != AwaError_Success)
                {
                    LOG(LOG_ERR,
                            "Could not add resource definition (%s [%d]) to object definition.",
                            object->resources[i].name,
                            object->resources[i].id);
                    AwaObjectDefinition_Free(&objectDefinition);
                }
            }
            else if (object->resources[i].type == AwaResourceType_Boolean)
            {
                if( AwaObjectDefinition_AddResourceDefinitionAsBoolean(
                                                                objectDefinition,
                                                                object->resources[i].id,
                                                                object->resources[i].name,
                                                                true,
                                                                AwaResourceOperations_ReadWrite,
                                                                NULL) != AwaError_Success)
                {
                    LOG(LOG_ERR,
                            "Could not add resource definition (%s [%d]) to object definition.",
                            object->resources[i].name,
                            object->resources[i].id);
                    AwaObjectDefinition_Free(&objectDefinition);
                }
            }

        }
    }
    return objectDefinition;
}

/**
 * @brief Define all objects and its resources with server deamon.
 * @param *session holds server session.
 * @return true if object is successfully defined on server, else false.
 */
bool DefineServerObjects(AwaServerSession *session)
{
    unsigned int i;
    unsigned int definitionCount = 0;
    bool result = true;

    if (session == NULL)
    {
        LOG(LOG_ERR, "Null parameter passsed to %s()", __func__);
        return false;
    }

    AwaServerDefineOperation *handler = AwaServerDefineOperation_New(session);
    if (handler == NULL)
    {
        LOG(LOG_ERR, "Failed to create define operation for session on server");
        return false;
    }

    for (i = 0; (i < ARRAY_SIZE(objects)) && result; i++)
    {
        LOG(LOG_INFO, "Defining %s[%d] object on awalwm2m server", objects[i].name, objects[i].id);

        if (AwaServerSession_IsObjectDefined(session, objects[i].id))
        {
            LOG(LOG_DBG, "%s[%d] object already defined on server", objects[i].name, objects[i].id);
            continue;
        }

        AwaObjectDefinition *objectDefinition = AddResourceDefinitions(&objects[i]);

        if (objectDefinition != NULL)
        {
            if (AwaServerDefineOperation_Add(handler, objectDefinition) != AwaError_Success)
            {
                LOG(LOG_ERR, "Failed to add object definition to define operation on server");
                result = false;
            }
            definitionCount++;
            AwaObjectDefinition_Free(&objectDefinition);
        }
    }

    if (result && definitionCount != 0)
    {
        if (AwaServerDefineOperation_Perform(handler, OPERATION_TIMEOUT) != AwaError_Success)
        {
            LOG(LOG_ERR, "Failed to perform define operation on server");
            result = false;
        }
    }
    if (AwaServerDefineOperation_Free(&handler) != AwaError_Success)
    {
        LOG(LOG_WARN, "Failed to free define operation object on server");
    }
    return result;
}

/**
 * @brief Create a fresh session with server.
 * @param port server's IPC port number.
 * @param *address ip address of server daemon.
 * @return pointer to server's session.
 */
AwaServerSession *Server_EstablishSession(unsigned int port, const char *address)
{
    /* Initialise Device Management session */
    AwaServerSession * session;
    session = AwaServerSession_New();

    LOG(LOG_INFO, "Establish server session for port:%u and address:%s", port, address);

    if (session != NULL)
    {
        /* call set IPC as UDP, pass address and port */
        if (AwaServerSession_SetIPCAsUDP(session, address, port) == AwaError_Success)
        {
            if (AwaServerSession_Connect(session) == AwaError_Success)
            {
                LOG(LOG_INFO, "Server session established\n");
            }
            else
            {
                LOG(LOG_ERR, "AwaServerSession_Connect() failed\n");
                AwaServerSession_Free(&session);
            }
        }
        else
        {
            LOG(LOG_ERR, "AwaServerSession_SetIPCAsUDP() failed\n");
            AwaServerSession_Free(&session);
        }
    }
    else
    {
        LOG(LOG_ERR, "AwaServerSession_New() failed\n");
    }
    return session;
}

/**
 * @brief Light controller application observes the IPSO resource for motion sensor on
 *        constrained device, and set the led on Ci40 board if any change observed.
 */
int main(int argc, char **argv)
{
    int i, ret;
    FILE *configFile;
    const char *fptr = NULL;

    ret = ParseCommandArgs(argc, argv, &fptr);
    if (ret <= 0)
    {
        return ret;
    }

    signal(SIGINT, CtrlCSignalHandler);
    signal(SIGTERM, CtrlCSignalHandler);

    if (fptr)
    {
        configFile = fopen(fptr, "w");
        if (configFile != NULL)
        {
            g_debugStream  = configFile;
        }
        else
        {
            LOG(LOG_ERR, "Failed to create or open %s file", fptr);
        }
    }

    AwaServerSession *serverSession = NULL;

    LOG(LOG_INFO, "Light Controller Application");
    LOG(LOG_INFO, "------------------------\n");

    serverSession = Server_EstablishSession(IPC_SERVER_PORT, IP_ADDRESS);
    if (serverSession == NULL)
    {
        LOG(LOG_ERR, "Failed to establish server session\n");
    }

    if (DefineServerObjects(serverSession))
    {
        for (i = 0; i < ARRAY_SIZE(objects); i++)
        {
            LOG(LOG_INFO, "Waiting for constrained device '%s' to be up",objects[i].clientID);
            while (CheckConstrainedRegistered(serverSession, objects[i].clientID) == false)
            {
                sleep(1 /*second*/);
            }
        }

        if (StartObservingSensor(serverSession))
        {
            unsigned int cachedSensorState = g_sensorState;
            while(!g_quit)
            {
                UpdateLed(false, true);
                if (AwaServerSession_Process(serverSession, 1000 /* 1 second */) != AwaError_Success)
                {
                    LOG(LOG_ERR, "AwaServerSession_Process() failed");
                    break;
                }
                AwaServerSession_DispatchCallbacks(serverSession);

                /* Check if sensor state is changed */
                if (g_sensorState != cachedSensorState)
                {
                    LOG(LOG_INFO, "Sensor state has changed");
                    TurnOnLight();
                    cachedSensorState = g_sensorState;
                }
                UpdateLed(true, true);
            }
        }
        else
        {
            LOG(LOG_ERR, "StartObservingSensor failed");
        }
    }

    /* Should never come here */
    UpdateLed(false, true);

    if (configFile)
    {
        fclose(configFile);
    }

    if (AwaServerSession_Disconnect(serverSession) != AwaError_Success)
    {
        LOG(LOG_ERR, "Failed to disconnect server session");
    }

    if (AwaServerSession_Free(&serverSession) != AwaError_Success)
    {
        LOG(LOG_WARN, "Failed to free server session");
    }

    LOG(LOG_INFO, "Light Controller Application Failure");

    return -1;
}
