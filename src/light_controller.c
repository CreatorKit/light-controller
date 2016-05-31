/***************************************************************************************************
 * Copyright (c) 2016, Imagination Technologies Limited
 * All rights reserved.
 *
 * Redistribution and use of the Software in source and binary forms, with or
 * without modification, are permitted provided that the following conditions are met:
 *
 *     1. The Software (including after any modifications that you make to it) must
 *        support the FlowCloud Web Service API provided by Licensor and accessible
 *        at  http://ws-uat.flowworld.com and/or some other location(s) that we specify.
 *
 *     2. Redistributions of source code must retain the above copyright notice, this
 *        list of conditions and the following disclaimer.
 *
 *     3. Redistributions in binary form must reproduce the above copyright notice, this
 *        list of conditions and the following disclaimer in the documentation and/or
 *        other materials provided with the distribution.
 *
 *     4. Neither the name of the copyright holder nor the names of its contributors may
 *        be used to endorse or promote products derived from this Software without
 *        specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 **************************************************************************************************/

/**
 * @file  temperature_gateway.c
 * @brief Light controller application observes the IPSO resource for motion sensor on constrained device.
 *        On receipt of awaLWM2M notification, ci40 will glow on board led for a period of 5 seconds.
 *        if further notifications are observed during the 5 second period then time out is restarted.           
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "awa/server.h"
#include "awa/client.h"
#include "log.h"

/***************************************************************************************************
 * Definitions
 **************************************************************************************************/

/** Calculate size of array. */
#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

//! @cond Doxygen_Suppress
#define IPC_SERVER_PORT             (54321)
#define IPC_CLIENT_PORT             (12345)
#define IP_ADDRESS                  "127.0.0.1"
#define MOTION_STR                  "SensorValue"
#define MOTION_OBJECT_ID            (3302)
#define MOTION_RESOURCE_ID          (5501)
#define MIN_INSTANCES               (0)
#define MAX_INSTANCES               (1)
#define OPERATION_TIMEOUT           (5000)
#define URL_PATH_SIZE               (16)
#define alarm_period                (5)
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
}RESOURCE_T;

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
    RESOURCE_T *resources; /**< resource information */
    /*@}*/
}OBJECT_T;

/**
 *  @brief To fill the Observecontext, which will be used later
 *         on, to grab the values for quiet, targets and numTargets
 */
typedef struct
{
    bool quiet;
    char *targets;
    int  numTargets;
} ObserveContext;

/***************************************************************************************************
 * Globals
 **************************************************************************************************/

/** Set default debug level to info. */
int debugLevel = LOG_INFO;
/** Set default debug stream to NULL. */
FILE *debugStream = NULL;
/** To hold the result of server session. */
AwaServerSession *serverSession = NULL;
/** To hold the temperature path /3303/0/5700. */
char motionResourcePath[URL_PATH_SIZE] = {0};
/** Initializing objects. */
static OBJECT_T objects[] =
{
    {
        "MotionSensorDevice",
        MOTION_OBJECT_ID,
        0,
        "MotionSensor",
        1,
        (RESOURCE_T []){
                        
                            MOTION_RESOURCE_ID,
                            0,
                            AwaResourceType_Integer,
                            MOTION_STR
                       },
    },
};

/***************************************************************************************************
 * Implementation
 **************************************************************************************************/

/**
 * @brief Prints temperature_gateway_appd usage.
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
 * @brief Parses command line arguments passed to temperature_gateway_appd.
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
                    debugLevel = tmp;
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
	system("echo 0 > /sys/class/leds/marduk\:red\:user2/brightness");
	LOG(LOG_INFO, "Turn OFF led on Ci40 board");
}

/**
 * @brief Turn on the light when notification is received.
 */
void TurnOnLight()
{
	signal(SIGALRM, TurnOffLight);
	system("echo 1 > /sys/class/leds/marduk\:red\:user2/brightness");
	LOG(LOG_INFO, "Turn ON led on Ci40 board\n");
	alarm(alarm_period);
}

/**
 * @brief To get the temperature data whenever there is a notification.
 * @param *changeset holds the information used to retrieve the observed value.
   @param *context contains the information of numtargets, quiet and targets.
 */
void ObserveCallback(const AwaChangeSet *changeSet, void *context)
{
    const AwaServerSession *session = AwaChangeSet_GetServerSession(changeSet);
    ObserveContext *observeContext = (ObserveContext *)context;
    const AwaInteger *value = NULL;
    
    AwaChangeSet_GetValueAsIntegerPointer(changeSet, observeContext->targets, &value);
    
    if (value != NULL)
    {
        fflush(stdout);
        LOG(LOG_INFO, "\nValue observed : %lld", *value);
        TurnOnLight();
    }
}

/** 
 * @brief Wait infinite for receiving notification.
 * @param *session holds the server session result.
 */
static void Wait(AwaServerSession *session)
{
    while(1)
    {
        if (AwaServerSession_Process(session, 1000 /* 1 second */) != AwaError_Success)
        {
             LOG(LOG_ERR, "AwaServerSession_Process() failed\n");
             break;
        }
        
        AwaServerSession_DispatchCallbacks(session);
    }
}

/**
 * @brief Observe the Motion resource and notifies whenever
 *        there is a change in reading.
 * @param *session holds the server session result
 * @return false if observe operation is failed on /3303/0/5700 of TemperatureSensorDevice.
 */
bool StartObservingTemperatureResource(AwaServerSession *session)
{
    int count = 0, i, validCount;
    AwaServerObserveOperation *operation;
    AwaError error = AwaError_Unspecified;
    
    if (session != NULL)
    {
        operation = AwaServerObserveOperation_New(session);
        
        if (operation == NULL)
        {
            LOG(LOG_ERR, "Failed to create observe operation\n");
            return false;
        }
        if (AwaAPI_MakeResourcePath(motionResourcePath,
                                    URL_PATH_SIZE,
                                    MOTION_OBJECT_ID,
                                    0, MOTION_RESOURCE_ID) == AwaError_Success)
        {
            ObserveContext observeContext;
            observeContext.numTargets = 1;
            /*pass quiet parameter as 1 to hide extra information from observed data*/
            observeContext.quiet = 1;      
            observeContext.targets = motionResourcePath;
            for (i = 0; i < ARRAY_SIZE(objects); i++)
            {
                AwaServerObservation *observation = AwaServerObservation_New(objects[i].clientID, motionResourcePath, ObserveCallback, &observeContext);
            
                if (observation != NULL)
                {
                    if (AwaServerObserveOperation_AddObservation(operation, observation) == AwaError_Success)
                    {
                        ++count;
                    }
                    else
                    {
                        LOG(LOG_ERR, "AwaServerObserveOperation_AddObservation failed\n");
                        return false;
                    }
                }
                else
                {
                    LOG(LOG_ERR, "AwaServerObservation_New failed\n");
                    return false;
                }
            }
        }
        else
        {
            LOG(LOG_INFO, "Couldn't generate all object and resource paths.\n");
            return false;
        }
        if (AwaServerObserveOperation_Perform(operation, OPERATION_TIMEOUT) != AwaError_Success)
        {
            LOG(LOG_ERR, "Failed to perform observe operation\n");
        }
        
        AwaServerObserveOperation_Free(&operation);
        validCount=count;
        
        if (count > 0)
        {
            if (validCount > 0)
            {
                Wait(session);
            }
        }
        }
    else
    {
        LOG(LOG_INFO, "awa observer session is Null");
        return false;
    }
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
    bool success = false;
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
                        success = true;
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
            LOG(LOG_ERR, "AwaServerListClientsOperation_Perform failed\n"
                "error: %s", AwaError_ToString(error));
        }

        if ((error = AwaServerListClientsOperation_Free(&operation)) != AwaError_Success)
        {
            LOG(LOG_ERR, "AwaServerListClientsOperation_Free failed\n"
                "error: %s", AwaError_ToString(error));
        }
    }
    else
    {
        LOG(LOG_ERR, "AwaServerListClientsOperation_New failed");
    }
    return success;
}

/**
 * @brief Add all resource definitions belongs to object.
 * @param *object whose resources are to be defined.
 * @return pointer to flow object definition.
 */
static AwaObjectDefinition *AddResourceDefinitions(OBJECT_T *object)
{
    int i;
    AwaObjectDefinition *objectDefinition = AwaObjectDefinition_New(object->id, object->name, 
                                                                    MIN_INSTANCES, MAX_INSTANCES);
    if (objectDefinition != NULL)
    {
        /*define resources*/
         for (i = 0; i < object->numResources; i++)
         {
             if (object->resources[i].type == AwaResourceType_Float)
             {
                 if( AwaObjectDefinition_AddResourceDefinitionAsFloat(objectDefinition,
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
             else
             {
                 if( AwaObjectDefinition_AddResourceDefinitionAsInteger(objectDefinition,
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
         }
    }
    return objectDefinition;
}

/**
 * @brief Define all objects and its resources with client daemon.
 * @param *session holds client session.
 * @return true if object is successfully defined on client, else false.
 */
bool DefineClientObjects(AwaClientSession *session)
{
    unsigned int i;
    unsigned int definitionCount = 0;
    bool success = true;

    if (session == NULL)
    {
        LOG(LOG_ERR, "Null parameter passsed to %s()", __func__);
        return false;
    }

    AwaClientDefineOperation *handler = AwaClientDefineOperation_New(session);
    
    if (handler == NULL)
    {
        LOG(LOG_ERR, "Failed to create define operation for session on client");
        return false;
    }

    for (i = 0; (i < ARRAY_SIZE(objects)) && success; i++)
    {
        if (AwaClientSession_IsObjectDefined(session, objects[i].id))
        {
            LOG(LOG_DBG, "%s object already defined on client", objects[i].name);
            continue;
        }

        AwaObjectDefinition *objectDefinition = AddResourceDefinitions(&objects[i]);

        if (objectDefinition != NULL)
        {
            if (AwaClientDefineOperation_Add(handler, objectDefinition) != AwaError_Success)
            {
                LOG(LOG_ERR, "Failed to add object definition to define operation on client");
                success = false;
            }
            
            definitionCount++;
            AwaObjectDefinition_Free(&objectDefinition);
        
        }
    }

    if (success && definitionCount != 0)
    {
        if (AwaClientDefineOperation_Perform(handler, OPERATION_TIMEOUT) != AwaError_Success)
        {
            LOG(LOG_ERR, "Failed to perform define operation on client");
            success = false;
        }
    }
    
    if (AwaClientDefineOperation_Free(&handler) != AwaError_Success)
    {
        LOG(LOG_WARN, "Failed to free define operation object on client");
    }
    return success;
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
    bool success = true;

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

    for (i = 0; (i < ARRAY_SIZE(objects)) && success; i++)
    {
        if (AwaServerSession_IsObjectDefined(session, objects[i].id))
        {
            LOG(LOG_DBG, "%s object already defined on server", objects[i].name);
            continue;
        }

        AwaObjectDefinition *objectDefinition = AddResourceDefinitions(&objects[i]);

        if (objectDefinition != NULL)
        {
            if (AwaServerDefineOperation_Add(handler, objectDefinition) != AwaError_Success)
            {
                LOG(LOG_ERR, "Failed to add object definition to define operation on server");
                success = false;
            }
            definitionCount++;
            AwaObjectDefinition_Free(&objectDefinition);
        }
    }

    if (success && definitionCount != 0)
    {
        if (AwaServerDefineOperation_Perform(handler, OPERATION_TIMEOUT) != AwaError_Success)
        {
            LOG(LOG_ERR, "Failed to perform define operation on server");
            success = false;
        }
    }
    if (AwaServerDefineOperation_Free(&handler) != AwaError_Success)
    {
        LOG(LOG_WARN, "Failed to free define operation object on server");
    }
    return success;
}

/**
 * @brief Create a fresh session with client.
 * @param port client's IPC port number.
 * @param *address ip address of client daemon.
 * @return pointer to client's session.
 */
AwaClientSession *Client_EstablishSession(unsigned int port, const char *address)
{
    AwaClientSession * session;
    session = AwaClientSession_New();

    if (session != NULL)
    {
        if (AwaClientSession_SetIPCAsUDP(session, address, port) == AwaError_Success)
        {
            if (AwaClientSession_Connect(session) != AwaError_Success)
            {
                LOG(LOG_ERR, "AwaClientSession_Connect() failed\n");
                AwaClientSession_Free(&session);
            }
        }
        else
        {
            LOG(LOG_ERR, "AwaClientSession_SetIPCAsUDP() failed\n");
            AwaClientSession_Free(&session);
        }
    }
    else
    {
        LOG(LOG_ERR, "AwaClientSession_New() failed\n");
    }
    return session;
}

/**
 * @brief Create a fresh session with server.
 * @param port server's IPC port number.
 * @param *address ip address of server daemon.
 * @return pointer to server's session.
 */
AwaServerSession *Server_EstablishSession(unsigned int port, const char *address)
{
    AwaServerSession * session;
    session = AwaServerSession_New();

    if (session != NULL)
    {
        if (AwaServerSession_SetIPCAsUDP(session, address, port) == AwaError_Success)
        {
            if (AwaServerSession_Connect(session) == AwaError_Success)
            {
                LOG(LOG_INFO, "server session established\n");
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
 * @brief  Light controller application observes the IPSO resource for motion sensor
 *	   on constrained device and turn on led when notification is received. 
 */
int main(int argc, char **argv)
{
    int i=0, ret;
    FILE *configFile;
    const char *fptr = NULL;
    
    ret = ParseCommandArgs(argc, argv, &fptr);
    
    if (ret <= 0)
    {
        return ret;
    }
    
    if (fptr)
    {
        configFile = fopen(fptr, "w");
        
        if (configFile != NULL)
        {
            debugStream  = configFile;
        }
        else
        {
            LOG(LOG_ERR, "Failed to create or open %s file", fptr);
        }
    }

    AwaClientSession *clientSession = NULL;

    LOG(LOG_INFO, "Light controller Application");

    LOG(LOG_INFO, "------------------------\n");

    clientSession = Client_EstablishSession(IPC_CLIENT_PORT, IP_ADDRESS);
    
    if (clientSession != NULL)
    {
        LOG(LOG_INFO, "client session established\n");
    }

    serverSession = Server_EstablishSession(IPC_SERVER_PORT, IP_ADDRESS);
    
    if (serverSession == NULL)
    {
        LOG(LOG_ERR, "Failed to establish server session\n");
    }
    
    if (DefineServerObjects(serverSession) && DefineClientObjects(clientSession))
    {
        for (i = 0; i < ARRAY_SIZE(objects); i++)
        {
            LOG(LOG_INFO, "\nWaiting for constrained device '%s' to be up", objects[i].clientID);
            
            while (CheckConstrainedRegistered(serverSession, objects[i].clientID) == false)
            {
                sleep(1 /*second*/);
            }
        }
    }
    
    while (1)
    {
        if (StartObservingTemperatureResource(serverSession))
        {
            if (AwaServerSession_Disconnect(serverSession) != AwaError_Success)
            {
                LOG(LOG_ERR, "Failed to disconnect server session");
            }

            if (AwaServerSession_Free(&serverSession) != AwaError_Success)
            {
                LOG(LOG_WARN, "Failed to free server session");
            }
            
            sleep(1);
            serverSession = Server_EstablishSession(IPC_SERVER_PORT, IP_ADDRESS);
        
        }
        else
        {
            break;
        }
    }

    /* Should never come here */

    if (AwaServerSession_Disconnect(serverSession) != AwaError_Success)
    {
        LOG(LOG_ERR, "Failed to disconnect server session");
    }

    if (AwaServerSession_Free(&serverSession) != AwaError_Success)
    {
        LOG(LOG_WARN, "Failed to free server session");
    }

    if (AwaClientSession_Disconnect(clientSession) != AwaError_Success)
    {
        LOG(LOG_ERR, "Failed to disconnect client session");
    }

    if (AwaClientSession_Free(&clientSession) != AwaError_Success)
    {
        LOG(LOG_WARN, "Failed to free client session");
    }

    LOG(LOG_INFO, "Light controller Application Failure");
    return -1;
}

