/*PVAServerRegister.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.07.24
 */

/* Author: Marty Kraimer */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <iostream>

#include <cantProceed.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>
#include <iocsh.h>
#include <initHooks.h>
#include <epicsExit.h>

#include <pv/pvAccess.h>
#include <pv/serverContext.h>

#include <epicsExport.h>

using std::cout;
using std::endl;
namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

static pvd::Mutex the_server_lock;
static pva::ServerContext::shared_pointer the_server;

static void startitup() {
    the_server = pva::ServerContext::create(pva::ServerContext::Config()
                                            .config(pva::ConfigurationBuilder()
                                                    // default to all providers instead of just "local"
                                                    .add("EPICS_PVAS_PROVIDER_NAMES", pva::PVACCESS_ALL_PROVIDERS)
                                                    .push_map()
                                                    // prefer to use EPICS_PVAS_PROVIDER_NAMES
                                                    // from environment
                                                    .push_env()
                                                    .build()));
}

static const iocshArg startPVAServerArg0 = { "providerNames", iocshArgString };
static const iocshArg *startPVAServerArgs[] = {
    &startPVAServerArg0};

static const iocshFuncDef startPVAServerFuncDef = {
    "startPVAServer", 1, startPVAServerArgs
};
static void startPVAServer(const iocshArgBuf *args)
{
    try {
        char *names = args[0].sval;
        if(names && names[0]!='\0') {
            printf("Warning: startPVAServer() no longer accepts provider list as argument.\n"
                       "         Instead place the following before calling startPVAServer() and iocInit()\n"
                       "  epicsEnvSet(\"EPICS_PVAS_PROVIDER_NAMES\", \"%s\")\n",
                    names);
        }
        pvd::Lock G(the_server_lock);
        if(the_server) {
            std::cout<<"PVA server already running\n";
            return;
        }
        startitup();
    }catch(std::exception& e){
        std::cout<<"Error: "<<e.what()<<"\n";
    }
}

static const iocshArg *stopPVAServerArgs[1] = {};
static const iocshFuncDef stopPVAServerFuncDef = {
    "stopPVAServer", 0, stopPVAServerArgs
};
static void stopPVAServer(const iocshArgBuf *args)
{
    try {
        pvd::Lock G(the_server_lock);
        if(!the_server) {
            std::cout<<"PVA server not running\n";
            return;
        }
        the_server.reset();
    }catch(std::exception& e){
        std::cout<<"Error: "<<e.what()<<"\n";
    }
}

static const iocshArg *statusPVAServerArgs[1] = {};
static const iocshFuncDef statusPVAServerFuncDef = {
    "statusPVAServer", 0, statusPVAServerArgs
};
static void statusPVAServer(const iocshArgBuf *args)
{
    try {
        pvd::Lock G(the_server_lock);
        if(!the_server) {
            std::cout<<"PVA server not running\n";
            return;
        } else {
            the_server->printInfo();
        }
    }catch(std::exception& e){
        std::cout<<"Error: "<<e.what()<<"\n";
    }
}

static void initStartPVAServer(initHookState state)
{
    pvd::Lock G(the_server_lock);
    if(state==initHookAfterIocRunning && !the_server) {
        startitup();

    } else if(state==initHookAtIocPause) {
        the_server.reset();
    }
}


static void registerStartPVAServer(void)
{
    iocshRegister(&startPVAServerFuncDef, startPVAServer);
    iocshRegister(&stopPVAServerFuncDef, stopPVAServer);
    iocshRegister(&statusPVAServerFuncDef, statusPVAServer);
    initHookRegister(&initStartPVAServer);
}

extern "C" {
    epicsExportRegistrar(registerStartPVAServer);
}
