#pragma once
#ifndef _PmiDbg_H
#define _PmiDbg_H
//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
// Summary:
// This header provides the extensions interface for the MSMPI Process
// Management Interface (PMI) for debuggers and profiling tools.
//
// Loading and Initialization:
// The PMI infrastructure will load the registered extensions in each
// process created to support a job. Each created PMI process will
// enumerate all extensions and load these extensions.  Once all
// extensions are loaded, the PMIDBG_NOTIFY_INITIALIZE notification
// will be sent to all loaded extensions using the mechanism described
// in the Notifications section described below.
// NOTE:
// There are no implied or explicit ordering guarantees to the loading
// or notification system. The only guarantee provided is that the
// initialization function PmiDbgInitExtension provided by the
// extension will be the first function that is called and the
// notifications (if there is any) will be sent to all the loaded
// extension via the Notification mechanism
//
// Registration and Installation:
// The actual extension DLLs may be installed anywhere. A 32 bit and a
// 64 bit version should be provided. To register the extension to be
// loaded, the user, either manually or through some installer, should
// create a registry key under
// HKLM\Software\Microsoft\Mpi\PmiExtensions.  The default value of
// this key must be the path to the extension DLL.  If just the DLL
// name is provided, standard DLL load paths will apply.
//
// Notifications, Interogation, and Injection:
// Each extension will receive notifications through the "Notify"
// callback during the various phases of execution. This "Notify"
// callback should be provided by the extension.  During these
// callbacks, the extension can use the provided Control callback to
// exchange information with the host process.
//
// Versioning:
// The PMIDBG version is defined as a 16bit value where the high 8
// bits represent the major and the low 8 bits minor version. All
// versions are incremental and only additive. When the extension is
// loaded, it is given the local version information supported by the
// host process. The extension must do a min() of the two to determine
// the common version. For extensions supporting a lower version than
// the host, nothing special is required. For extensions supporting a
// higher version than the host, they must to restrict their
// extensions use of the PMIDBG APIs to those based on the actual
// version of the host.
//
//-----------------------------------------------------------------------------

#define PMIDBG_VERSION_1 MAKEWORD(1,0)
#define PMIDBG_VERSION_1_1 MAKEWORD(1,1)
#ifndef PMIDBG_VERSION
#  define PMIDBG_VERSION   PMIDBG_VERSION_1_1
#endif

//
// To register an extension, create a key under the below path in HKLM
// with a default value of the path to the dll.  The 64 bit registry
// root should point to the 64 bit version of the extension, and the
// 32 bit registry root should point to the 32 bit version.
//
#define PMIDBG_REG_PATH_A "Software\\Microsoft\\Mpi\\PmiExtensions"
#define PMIDBG_REG_PATH_W L##PMIDBG_REG_PATH_A
#define PMIDBG_REG_PATH   TEXT(PMIDBG_REG_PATH_A)

//
// Summary:
// Enumeration values used to identify the role of the host process
// that has loaded the extension.
//
typedef enum _PMIDBG_HOST_TYPE
{
    //
    // Signifies that the extension is being loaded by the controller
    // (MPIEXEC).  This extension will remain loaded until the entire
    // task completes or aborts. The extension will be loaded once per
    // task on a single machine.
    //
    PMIDBG_HOST_CONTROLLER      = 1,

    //
    // Signifies that the extension is being loaded by the node
    // manager (SMPD).  This extension will remain loaded until the
    // entire task completes or aborts.  The extension will be loaded
    // once on each machine that is involved in the job.
    //
    PMIDBG_HOST_MANAGER,

} PMIDBG_HOST_TYPE;



//
// Summary:
//  Enumeration values used to identify the notification event that is
//  occurring.  This value is sent through the notification callback
//  provided by the extension.
//
typedef enum _PMIDBG_NOTIFY_TYPE
{
    //
    // This notification is sent in all host types after all
    // extensions have been loaded into the host process.  Extensions
    // should use this to collect global information and open any
    // communication ports that are required.
    //
    PMIDBG_NOTIFY_INITIALIZE = 0,


    //
    // This notification is sent when the controller is about to tell
    // all managers to create the worker processes.  Extensions can
    // use this get to the list of machines and world size
    // information.
    //
    PMIDBG_NOTIFY_BEFORE_CREATE_PROCESSES,

    //
    // This notification is sent when the controller has receive
    // confirmation that all worker processes have been created
    // successfully.
    //
    PMIDBG_NOTIFY_AFTER_CREATE_PROCESSES,


    //
    // This notification is sent when the manager is about to create
    // the worker process.  Extensions can obtain the program,
    // arguments, and rank information of the process that is about to
    // be created.
    //
    PMIDBG_NOTIFY_BEFORE_CREATE_PROCESS,

    //
    // This notification is sent when the manager has created a
    // suspended the worker process.  Extensions can obtain the
    // program, arguments, startup info, and rank information of the
    // process that is about to be created.  Additionally, they can
    // also get the process and thread handles and ids and override
    // the default behavior to call ResumeThread on the new thread
    // handle.
    //
    PMIDBG_NOTIFY_AFTER_CREATE_PROCESS,

    //
    // This notification is send before unloading the extension in any
    // role.  This notification is sent either at the end of the task,
    // or immediately following a notification where the Unload
    // callback was invoked because of an error.
    //
    PMIDBG_NOTIFY_FINALIZE,

} PMIDBG_NOTIFY_TYPE;


//
// Summary:
//  Identifies the various interogaton routines that can be performed
//  by extensions.
//
typedef enum _PMIDBG_OPCODE_TYPE
{
    //
    // This operation may be sent during any notification on any host
    // type.  The pBuffer argument must point to a PMIDBG_SYSTEM_INFO*
    // and cbBuffer must be greater than or equal to
    // sizeof(PMIDBG_SYSTEM_INFO*).
    //
    PMIDBG_OPCODE_GET_SYSTEM_INFO = 0,

    //
    // This operation may be sent during any notification on any host
    // type.  The pBuffer argument must point to a char* and cbBuffer
    // must be greater than or equal to sizeof(char*).
    //
    PMIDBG_OPCODE_GET_JOB_CONTEXT,

    //
    // This operation may only be sent during the
    // PMIDBG_NOTIFY_BEFORE_CREATE_PROCESSES notification on the
    // controller for the job.  The pBuffer argument must point to a
    // UINT and cbBuffer must be greater than or equal to
    // sizeof(UINT).
    //
    PMIDBG_OPCODE_GET_WORLD_SIZE,

    //
    // This operation may only be sent during the
    // PMIDBG_NOTIFY_BEFORE_CREATE_PROCESSES notification on the
    // controller for the job.  The pBuffer argument must point to a
    // PMIDBG_ENUM_WORLD_NODES structure and cbBuffer must be greater
    // than or equal to sizeof(PMIDBG_ENUM_WORLD_NODES).  To start the
    // enumeration, set the Context field of the
    // PMIDBG_ENUM_WORLD_NODES structure to PMIDBG_ENUM_BEGIN.  The
    // name of the machine can be obtained from the Hostname field of
    // the PMIDBG_ENUM_WORLD_NODES structure on return of the Control
    // callback.  The Context field will be set to PMIDBG_ENUM_END
    // when there are no more items in the list.
    //
    PMIDBG_OPCODE_ENUM_WORLD_NODES,

    //
    // This operation may only be sent during the
    // PMIDBG_NOTIFY_BEFORE_CREATE_PROCESS and
    // PMIDBG_NOTIFY_AFTER_CREATE_PROCESS notifications on the manager
    // for a machine.  The pBuffer argument must point to a char* and
    // cbBuffer must be greater than or equal to sizeof(char*).
    //
    PMIDBG_OPCODE_GET_PROCESS_COMMAND,

    //
    // This operation may only be sent during the
    // PMIDBG_NOTIFY_BEFORE_CREATE_PROCESS and
    // PMIDBG_NOTIFY_AFTER_CREATE_PROCESS notifications on the manager
    // for a machine.  The pBuffer argument must point to a char* and
    // cbBuffer must be greater than or equal to sizeof(char*).
    //
    PMIDBG_OPCODE_GET_PROCESS_ARGUMENTS,

    //
    // This operation may only be sent during the
    // PMIDBG_NOTIFY_BEFORE_CREATE_PROCESS and
    // PMIDBG_NOTIFY_AFTER_CREATE_PROCESS notifications on the manager
    // for a machine.  The pBuffer argument must point to a int and
    // cbBuffer must be greater than or equal to sizeof(int).
    //
    PMIDBG_OPCODE_GET_PROCESS_RANK,

    //
    // This operation may only be sent during the
    // PMIDBG_NOTIFY_AFTER_CREATE_PROCESS notification on the manager
    // for a machine.  The pBuffer argument must point to a
    // PROCESS_INFORMATION* and cbBuffer must be greater than or equal
    // to sizeof(PROCESS_INFORMATION*).
    //
    PMIDBG_OPCODE_GET_PROCESS_INFORMATION,

    //
    // This operation may only be sent during the
    // PMIDBG_NOTIFY_AFTER_CREATE_PROCESS notification on the manager
    // for a machine to prevent ResumeThread from being called on the
    // process that was just created.  This leaves the process in a
    // suspended state.  The extension can then use the handle of the
    // thread to control the startup of the worker process.  The
    // pBuffer and cbBuffer arguments are unused.
    //
    PMIDBG_OPCODE_OVERRIDE_PROCESS_RESUME,

    //
    // This operation may only be sent during the
    // PMIDBG_NOTIFY_BEFORE_CREATE_PROCESSES notification on the
    // controller for the job.  The pBuffer argument must point to an
    // int* and cbBuffer must be greater than or equal to sizeof(int*).
    //
    PMIDBG_OPCODE_GET_PROCSIZE_ADDR,

    //
    // This operation may only be sent during the
    // PMIDBG_NOTIFY_BEFORE_CREATE_PROCESSES notification on the
    // controller for the job.  The pBuffer argument must point to a
    // MPIR_PROCDESC* and cbBuffer must be greater
    // than or equal to sizeof(MPIR_PROCDESC*).
    //
    PMIDBG_OPCODE_GET_PROCTABLE_ADDR,

    //
    // This operation may only be sent during the
    // PMIDBG_NOTIFY_BEFORE_CREATE_PROCESSES notification on the
    // controller for the job. The pBuffer argument must point to an
    // int and cbBuffer must be greater than or equal to sizeof(int).
    // The possible returned values are described in the
    // MPIDBG_DBG_MODE enum
    //
    PMIDBG_OPCODE_GET_DEBUG_MODE,

} PMIDBG_OPCODE_TYPE;


//
// Summary:
//  This is the callback function provided by the system to extensions
//  to allow the extension to get and set information from within host
//  process.
//
// Parameters:
//  type        - The type of operation requested by the extension.
//  pData       - The pointer to the data buffer provided in the
//                notification callback.
//                Note: This must be the pData argument from the Notify
//                callback.
//  pBuffer     - This is a pointer to a buffer that is used by the
//                operation type. See the details of the specific
//                operation to know the type of data to pass here.
//  cbBuffer    - This is the size of the buffer pointed to by pBuffer
//
// Returns:
//  An HRESULT indicating status. Callers should use SUCCEEDED and
//  FAILED macros to test for error conditions.
//
typedef HRESULT ( __stdcall FN_PmiDbgControl ) (
    __in PMIDBG_OPCODE_TYPE              type,
    __in void*                           pData,
    __inout_bcount(cbBuffer) void*       pBuffer,
    __in SIZE_T                          cbBuffer
    );


//
// Summary:
//  This callback function can be used during notification events to
//  signify that the extension has entered an error state and needs to
//  be unloaded.  Once the notification returns, the extension will be
//  finalized and unloaded.
//
typedef HRESULT ( __stdcall FN_PmiDbgUnload)();


//
// Summary:
//  This callback is provided by the extension to receive notification
//  from the host process.
//
// Parameters:
//  type        - The type of notification that is being sent.
//  pData       - The opaque data buffer that can be used for
//                Control operations.
//
typedef VOID ( __stdcall FN_PmiDbgNotify)(
    __in PMIDBG_NOTIFY_TYPE     type,
    __in void*                  pData
    );


//
// Summary:
//  This structure provides the information about the host process.
//
// Fields:
//  Version         - The current supported version of the host process
//  Host            - The role of the current host process.
//  AppName         - The simple text name for the host process.
//  LocalName       - The hostname of the local machine.
//  Control         - The callback function to get and set information
//                    within the host process
//  Unload          - The callback function used to trigger an
//                    unload of the current extension
//
typedef struct _PMIDBG_SYSTEM_INFO
{
    ULONG                   Version;
    PMIDBG_HOST_TYPE        Host;
    const char*             AppName;
    const char*             LocalName;
    FN_PmiDbgControl*       Control;
    FN_PmiDbgUnload*        Unload;

} PMIDBG_SYSTEM_INFO;


//
// Summary:
//  This structure provides the information about the extension and
//  the its supported callbacks.
//
// Fields:
//  Version     - The min version of the host process and the extension.
//  Notify      - The notification callback invoked when events occur
//                in the host process.
//
// Remarks:
//  The value specified in the Version field is the maximum supported
//  version by the extension, it may not be fully supported by the
//  PMI Host process, so the extension must inspect the Version field
//  of the PMIDBG_SYSTEM_INFO struct during the Initialize callback to
//  determine the actual version of the interface being supported.
//
typedef struct _PMIDBG_FUNCTIONS
{
    ULONG                   Version;
    FN_PmiDbgNotify*        Notify;

} PMIDBG_FUNCTIONS;


//
// Summary:
//  This is the export provided by all extensions.  It is called after
//  the DLL is loaded into the host process.  If the extension returns
//  FALSE, the extension is immediately removed from the list and
//  unloaded.
//
typedef BOOL ( __stdcall FN_PmiDbgInitExtension)(
    __in HKEY                               hKey,
    __in const PMIDBG_SYSTEM_INFO*          pInfo,
    __out PMIDBG_FUNCTIONS*                 pFunctions
    );

#define PMIDBG_INIT_EXTENSION_FN_NAME "PmiDbgInitExtension"


//
// Summary:
//  This structure is used during the PMIDBG_OPCODE_ENUM_WORLD_NODES
//  control operation to access the list of nodes involved in a job.
//
// Fields:
//  Context         - Opaque context value to identify the current
//                    element in the list.  To begin the iteration,
//                    set the value to PMIDBG_ENUM_BEGIN. This field
//                    will be set to PMIDBG_ENUM_END when there are
//                    no more items in the list.
//  Hostname        - On return from the Control callback, this
//                    contains the hostname of the node.
//
typedef struct _PMIDBG_ENUM_WORLD_NODES
{
    LONG_PTR            Context;
    const char*         Hostname;

} PMIDBG_ENUM_WORLD_NODES;


//
// Summary:
//  This structure is used during the PMIDBG_OPCODE_GET_PROCTABLE_ADDR
//  operation to access the information about the MPI processes.
//
// Fields:
//  hostname        - The name of the host where the process lives in
//  executable_name - The executable name of the process
//  pid             - The pid of the process
//
typedef struct
{
    char* host_name;
    char* executable_name;
    int   pid;
} MPIR_PROCDESC;


//
// Summary:
//  This enum describe the possible returned values for the
//  PMIDBG_OPCODE_GET_DEBUG_MODE operation
//
// Values:
//  MPIDBG_DBG_LAUNCH - The job was launched under the debugger
//  MPIDBG_DBG_ATTACH - The job was not launched under the debugger.
//                      The debugger has attached to the processes
//  MPIDBG_DBG_DUMP   - The job is a debugging job for dump files
//
typedef enum  _MPIDBG_DBG_MODE
{
    MPIDBG_DBG_LAUNCH = 0,
    MPIDBG_DBG_ATTACH,
    MPIDBG_DBG_DUMP
} MPIDBG_DBG_MODE;


//
// Values for the debug_state, this seems to be all we need at the moment
// but that may change...
//
#define MPIR_NULL            0
#define MPIR_DEBUG_SPAWNED   1
#define MPIR_DEBUG_ABORTING  2


#define PMIDBG_ENUM_BEGIN ((LONG_PTR)0)
#define PMIDBG_ENUM_END   ((LONG_PTR)-1)

#endif //#ifndef _PmiDbg_H
