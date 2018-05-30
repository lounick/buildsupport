/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*
 * C AST Construction functions
 * This file contains all the functions used to build
 * the Abstact Syntax Tree (AST) of a system
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdbool.h>

#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"

/* Data shared by the functions of this module */

/* System AST: */
static System *system_ast = NULL;

/* Temporary AST construction units: */
static FV *fv = NULL;
static Interface *interface = NULL;
static Parameter *parameter = NULL;
static Aplc_binding *binding = NULL;
static char *binding_name = NULL;
static Process *process = NULL;
static Processor *processor = NULL;
static Bus *bus = NULL;
static Device *device = NULL;
static Connection *connection = NULL;
static char *search_name = NULL;
static Device_list *drivers = NULL;

/* Function returning a pointer to the system AST */
System *get_system_ast()
{
    return system_ast;
}

/* Set System name */
void Set_Root_Node(char *name, size_t len)
{
    build_string(&(system_ast->name), name, len);
}

void Set_Native_Encoding()
{
    if (NULL != parameter)
        parameter->encoding = native;
}

void Set_UPER_Encoding()
{
    if (NULL != parameter)
        parameter->encoding = uper;
}

void Set_ACN_Encoding()
{
    if (NULL != parameter)
        parameter->encoding = acn;
}


void Set_ASN1_BasicType_Sequence()
{
    if (NULL != parameter)
        parameter->basic_type = sequence;
}

void Set_ASN1_BasicType_SequenceOf()
{
    if (NULL != parameter)
        parameter->basic_type = sequenceof;
}

void Set_ASN1_BasicType_Enumerated()
{
    if (NULL != parameter)
        parameter->basic_type = enumerated;
}

void Set_ASN1_BasicType_Set()
{
    if (NULL != parameter)
        parameter->basic_type = set;
}

void Set_ASN1_BasicType_SetOf()
{
    if (NULL != parameter)
        parameter->basic_type = setof;
}

void Set_ASN1_BasicType_Integer()
{
    if (NULL != parameter)
        parameter->basic_type = integer;
}

void Set_ASN1_BasicType_Boolean()
{
    if (NULL != parameter)
        parameter->basic_type = boolean;
}

void Set_ASN1_BasicType_Real()
{
    if (NULL != parameter)
        parameter->basic_type = real;
}

void Set_ASN1_BasicType_Choice()
{
    if (NULL != parameter)
        parameter->basic_type = choice;
}

void Set_ASN1_BasicType_String()
{
    if (NULL != parameter)
        parameter->basic_type = string;
}

void Set_ASN1_BasicType_OctetString()
{
    if (NULL != parameter)
        parameter->basic_type = octetstring;
}

void Set_ASN1_BasicType_Unknown()
{
    if (NULL != parameter)
        parameter->basic_type = unknown_type;
}

void Set_Debug_Messages()
{
        if (NULL != (system_ast->context)) {
            system_ast->context->debug = 1;
        }
}

// Functions in AADL model may contain additional properties, set them in a list
void Set_Property (char *name, size_t name_len, char *val, size_t val_len)
{
    AADL_Property *property;

    property = (AADL_Property *) malloc (sizeof (AADL_Property));
    assert (NULL != fv && NULL != property && 0 < name_len && 0 < val_len);
    property->name = NULL;
    property->value = NULL;

    build_string (&(property->name), name, name_len);
    build_string (&(property->value), val, val_len);
    APPEND_TO_LIST (AADL_Property, fv->properties, property);
}

void Set_Language_To_SDL()
{
    if (NULL != fv)
        fv->language = sdl;
}

void Set_Language_To_CPP()
{
    if (NULL != fv)
        fv->language = cpp;
}

void Set_Language_To_VDM()
{
    if (NULL != fv)
        fv->language = vdm;
}

void Set_Language_To_OpenGEODE()
{
    if (NULL != fv)
        fv->language = opengeode;
}
void Set_Language_To_Simulink()
{
    if (NULL != fv)
        fv->language = simulink;
}

void Set_Language_To_BlackBox_Device()
{
    if (NULL != fv)
        fv->language = blackbox_device;
}

void Set_Language_To_QGenAda()
{
    if (NULL != fv)
        fv->language = qgenada;
}

void Set_Language_To_QGenC()
{
    if (NULL != fv)
        fv->language = qgenc;
}

void Set_Language_To_Other()
{
    if (NULL != fv)
        fv->language = other;
}

void Set_Language_To_C()
{
    if (NULL != fv)
        fv->language = c;
}

void Set_Language_To_RTDS()
{
    if (NULL != fv)
        fv->language = rtds;
}

void Set_Language_To_Rhapsody()
{
    if (NULL != fv)
        fv->language = rhapsody;
}

void Set_Language_To_Scade()
{
    if (NULL != fv)
        fv->language = scade;
}

void Set_Language_To_Ada()
{
    if (NULL != fv)
        fv->language = ada;
}

void Set_Language_To_GUI()
{
    if (NULL != fv)
        fv->language = gui;
}

void Set_Language_To_VHDL()
{
    if (NULL != fv)
        fv->language = vhdl;
}

void Set_Language_To_VHDL_BRAVE()
{
    if (NULL != fv)
        fv->language = vhdl_brave;
}

void Set_Language_To_System_C()
{
    if (NULL != fv)
        fv->language = system_c;
}

void Set_Language_To_MicroPython()
{
    if (NULL != fv)
        fv->language = micropython;
}

void Set_Zipfile (char *file, size_t len)
{
    assert (NULL != file && 0 < len && NULL != fv);
    build_string (&(fv->zipfile), file, len);
}

void Set_Sync_IF()
{
    if (NULL != interface)
        interface->synchronism = synch;
}

void Set_ASync_IF()
{
    if (NULL != interface)
        interface->synchronism = asynch;
}

void Set_Unknown_IF()
{
    if (NULL != interface)
        interface->synchronism = unknown;
}

void Set_Cyclic_IF()
{
    if (NULL != interface)
        interface->rcm = cyclic;
}

void Set_Sporadic_IF()
{
    if (NULL != interface)
        interface->rcm = sporadic;
}

void Set_Variator_IF()
{
    if (NULL != interface)
        interface->rcm = sporadic;
}

void Set_Protected_IF()
{
    if (NULL != interface)
        interface->rcm = protected;
}

void Set_Unprotected_IF()
{
    if (NULL != interface)
        interface->rcm = unprotected;
}

void Set_UndefinedKind_IF()
{
    if (NULL != interface)
        interface->rcm = undefined;
}

void Set_Period(long long p)
{
    if (NULL != interface)
        interface->period = p;
}

void Set_Context_Variable (char *name, size_t len1, char *type, size_t len2,
                           char *def, size_t len3, char *mod, size_t len4,
                           char *file, size_t len5, char *fullNameWithCase)
{
 Context_Parameter *cp = NULL;
 unsigned int i = 0;

 cp = (Context_Parameter *) malloc (sizeof (Context_Parameter));

 assert (NULL != cp);

 cp->name = NULL;
 cp->fullNameWithCase = NULL;
 cp->type.name = NULL;
 cp->type.module = NULL;
 cp->type.asn1_filename = NULL;
 cp->value = NULL;

 /* First check the validity of the ASN.1 module name */
 if (!strncmp (mod, "nomodule", len4)) {
     ERROR ("[ERROR] The dataview you are using was generated using an old version\n");
     ERROR ("        of the toolchain. You must update it by calling taste-update-data-view\n");
     exit (-1);
 }
 /* Temporary for the SMP2 support until version of taste-IV withtout obj suffixes */
 size_t lenWithSuffix = len1;
 /* Remove any _objXXX suffix on the name field */
 len1 = remove_objXXX_suffix(name, len1);

 build_string (&(cp->name), name, len1);
 build_string (&(cp->fullNameWithCase), fullNameWithCase, lenWithSuffix);  // temp: remove when objXX disappears
 build_string (&(cp->type.name), type, len2);
 build_string (&(cp->value), def, len3);
 build_string (&(cp->type.module), mod, len4);
 build_string (&(cp->type.asn1_filename), file, len5);

 /* Convert '_' to '-' to be ASN.1-compliant */
 for (i=0; i<len4; i++) if ('_' == cp->type.module[i]) cp->type.module[i] = '-';

 APPEND_TO_LIST (Context_Parameter, fv->context_parameters, cp);
}

/* build the WCET string used in VT. */
void Set_Compute_Time(uint64_t lower, char *unitlower, size_t len2,
                      uint64_t upper, char *unitupper, size_t len4)
{
    if (NULL == interface) {
        return;
    }

    interface->wcet_low = lower;
    interface->wcet_high = upper;
    build_string(&(interface->wcet_low_unit), unitlower, len2);
    build_string(&(interface->wcet_high_unit), unitupper, len4);
}

/* Context-related functions */

/* Function returning a pointer to the current system context */
Context *get_context()
{
    return system_ast->context;
}

void Set_Interfaceview (char *name, size_t len)
{
    if (NULL != (system_ast->context) && 0 < len && NULL != name) {
        build_string(&(system_ast->context)->ifview, name, len);
    }

}

void Set_Dataview (char *name, size_t len)
{
    if (NULL != (system_ast->context) && 0 < len && NULL != name) {
        build_string (&(system_ast->context)->dataview, name, len);
    }
}

void Set_Glue()
{
    if (NULL != (system_ast->context)) {
        system_ast->context->glue = 1;
    }
}

void Set_SMP2()
{
    if (NULL != (system_ast->context)) {
        system_ast->context->smp2 = true;
    }
}

void Set_Gateway()
{
    if (NULL != (system_ast->context)) {
        system_ast->context->gw = 1;
    }
}

void Set_keep_case ()
{
    if (NULL != (system_ast->context)) {
        system_ast->context->keep_case = 1;
    }
}

void Set_OnlyCV()
{
    if (NULL != (system_ast->context)) {
        system_ast->context->onlycv = 1;
    }
}

void Set_AADLV2()
{
    if (NULL != (system_ast->context)) {
        system_ast->context->aadlv2 = 1;
    }
}

void Set_Test()
{
    if (NULL != (system_ast->context)) {
        system_ast->context->test = 1;
    }
}

void Set_Timer_Resolution(char *val, size_t len) {
    errno = 0;
    if (NULL != (system_ast->context)) {
        char *str = make_string("%.*s", len, val);
        int time_res = (int) strtol(str, (char **)NULL, 10);
        if (0 != errno) {
            ERROR("[ERROR] Timer resolution must be a valid number\n");
        }
        else {
            printf("[INFO] Set timer resolution to %d\n", time_res);
            system_ast->context->timer_resolution = time_res;
        }
    }
}


/* Set Future - Flag that can be used for temporary purposes,
 * like when migrating from one TASTE version to another one when all
 * tools are not ready.
 */
void Set_Future()
{
    if (NULL != (system_ast->context)) {
        system_ast->context->future = 1;
    }
}

void Set_PolyorbHI_C()
{
    if (NULL != (system_ast->context)) {
        system_ast->context->polyorb_hi_c = 1;
    }
}

/* Set the root output directory in the context vector of the AST
 * and create the directory if possible */
void Set_OutDir(char *o, size_t len)
{
    if (NULL == (system_ast->context))
        return;

    build_string(&(system_ast->context)->output, o, len);

    if (((system_ast->context)->output)[len - 1] != '/'
        && ((system_ast->context)->output)[len - 1] != '\\') {
        build_string(&(system_ast->context)->output, "/", 1);
    }

    /* create the root output directory */
    if (-1 == mkdir((system_ast->context)->output, 0700)
        && EEXIST != errno) {
        ERROR("[ERROR] output directory could not be created!\n");
        ERROR("        Current directory will be used instead...\n");
        free((system_ast->context)->output);
        (system_ast->context)->output = NULL;
    }
}

/* Set the stack size used in generated AADL files */
void Set_Stack(char *val, size_t len)
{
    if (NULL == (system_ast->context))
        return;
    build_string(&(system_ast->context->stacksize), val, len);
}

void Set_Instance_Of(char *component, size_t len)
{
    assert (0 < len && NULL != fv);
    build_string (&(fv->instance_of), component, len);
}

void Set_Is_Component_Type()
{
    assert (NULL != fv);
    fv->is_component_type = true;
}

/* End of context-related functions */


/* Process-related functions */

/* Define a new process (partition in the deployment view) */
void New_Process(char *procname,
                 size_t length,
                 char *procid,
                 size_t lenid,
                 char *node,
                 size_t lennode,
                 bool coverage)
{
    Create_Process(&process);

    if (NULL != process) {
        build_string(&(process->name), procname, length);
        build_string(&(process->identifier), procid, lenid);
        build_string(&(process->processor_board_name), node, lennode);
        if (processor != NULL) {
            process->cpu = processor;
            /* Append the node name to the CPU name */
            char *old_cpu_name = process->cpu->name;
            process->cpu->name = make_string ("%s_%s",
                                              process->processor_board_name,
                                              old_cpu_name);
            //free(old_cpu_name);
        }
        process->coverage = coverage;
    }
}

void New_Processor (char *name,       size_t name_length,
                    char *classifier, size_t classifier_length,
                    char* platform,   size_t platform_length,
                    char *envvars,    size_t envvars_length,
                    char *cflags,     size_t cflags_length,
                    char *ldflags,    size_t ldflags_length)
{
    processor = malloc(sizeof(Processor));
    assert(NULL != processor);

    processor->name          = NULL;
    processor->classifier    = NULL;
    processor->platform_name = NULL;
    processor->envvars       = NULL;
    processor->user_cflags   = NULL;
    processor->user_ldflags  = NULL;

    build_string(&(processor->name), name, name_length);
    build_string(&(processor->classifier), classifier,
                 classifier_length);
    if (platform_length > 0) {
        build_string(&(processor->platform_name), platform, platform_length);
    }
    if (envvars_length > 0) {
        build_string(&(processor->envvars), envvars, envvars_length);
    }
    if (cflags_length > 0) {
        build_string(&(processor->user_cflags), cflags, cflags_length);
    }
    if (ldflags_length > 0) {
        build_string(&(processor->user_ldflags), ldflags, ldflags_length);
    }
}


void New_Bus(char *name, size_t name_length, char *classifier,
                   size_t classifier_length)
{
    bus = malloc(sizeof(Bus));
    assert (NULL != bus);

        bus->name = NULL;
        bus->classifier = NULL;
        build_string(&(bus->name), name, name_length);
        build_string(&(bus->classifier), classifier,
                     classifier_length);
}

/* Connections in the deployment view */
void New_Connection(char *src_system, size_t src_system_length,
                    char *src_port, size_t src_port_length,
                    char *busname, size_t bus_length,
                    char *dst_system, size_t dst_system_length,
                    char *dst_port, size_t dst_port_length)
{

   src_port_length = remove_objXXX_suffix (src_port, src_port_length);
   dst_port_length = remove_objXXX_suffix (dst_port, dst_port_length);

   /* If the first 3 characters of the dst_port name are "obj" we suppose
    * it is a name generated randomly by TASTE-IV. In that case we replace
    * the port name with the name of the source port.
    */
    if (dst_port_length >= 4 && !strncmp (dst_port, "obj", 3)) {
                  dst_port = src_port;
                  dst_port_length = src_port_length;
    }

   connection = malloc(sizeof(Connection));
   assert (NULL != connection);

   connection->src_system       = NULL;
   connection->src_port         = NULL;
   connection->bus              = NULL;
   connection->dst_system       = NULL;
   connection->dst_port         = NULL;

   build_string(&(connection->src_system), src_system, 
                src_system_length);
   build_string(&(connection->src_port), src_port, 
                src_port_length);
   build_string(&(connection->bus), busname, bus_length);
   build_string(&(connection->dst_system), dst_system, 
                dst_system_length);
   build_string(&(connection->dst_port), dst_port, 
                dst_port_length);
}


void New_Drivers_Section()
{
    drivers = NULL;
}

void End_Drivers_Section()
{
    if (NULL == process) return;
    process->drivers = drivers;
    APPEND_TO_LIST (Process, (system_ast->processes), process);
    drivers = NULL;
    process = NULL;
}

void New_Device   (char *name,
                  size_t   name_length,
                  char  *classifier,
                  size_t   classifier_length,
                  char  *associated_processor,
                  size_t   associated_processor_length,
                  char  *configuration,
                  size_t   configuration_length,
                  char  *accessed_bus,
                  size_t   accessed_bus_length,
                  char  *access_port,
                  size_t   access_port_length,
                  char  *asn1_filename,
                  size_t   asn1_filename_length,
                  char  *asn1_typename,
                  size_t   asn1_typename_length, 
                  char  *asn1_modulename,
                  size_t   asn1_modulename_length)
{
    device = malloc(sizeof(Device));
    assert (NULL != device);

    device->name                    = NULL;
    device->classifier              = NULL;
    device->associated_processor    = NULL;
    device->accessed_bus            = NULL;
    device->configuration           = NULL;
    device->access_port             = NULL;
    device->asn1_filename           = NULL;
    device->asn1_typename           = NULL;
    device->asn1_modulename         = NULL;

    /* Prefix the driver name with the name of the node */
    device->name = make_string ("%s_%.*s",
                                process->processor_board_name,
                                name_length,
                                name);

    if (strncmp (configuration, "noconf", configuration_length)) {
            build_string (&(device->configuration), configuration, 
                    configuration_length);
    }

    build_string (&(device->classifier), classifier,
                    classifier_length);
    /* Prefix CPU name with the name of the node */
    device->associated_processor = make_string ("%s_%.*s",
            process->processor_board_name,
            associated_processor_length,
            associated_processor);
    build_string (&(device->accessed_bus), accessed_bus,
                    accessed_bus_length);
    build_string (&(device->access_port), access_port,
                    access_port_length);

    if (strncmp (asn1_filename, "noconf", asn1_filename_length)) {
            build_string (&(device->asn1_filename), asn1_filename,
                    asn1_filename_length);
    }

    if (strncmp (asn1_typename, "notype", asn1_typename_length)) {
            build_string (&(device->asn1_typename), asn1_typename,
                    asn1_typename_length);
    }

    if (strncmp (asn1_modulename, "nomod", asn1_typename_length)) {
            build_string (&(device->asn1_modulename), asn1_modulename,
                    asn1_modulename_length);
    }
}



/* Packages added when parsing the deployment view
 * Used to generate list of WITH statements in process.aadl
*/
void Add_Package(char *pkg_name, size_t len)
{
   Package *pack = (Package *) make_string("%.*s", len, pkg_name);
   assert (NULL != pack);
   ADD_TO_SET(Package, system_ast->packages, pack);
}

void End_Process()
{
    // Nothing to do: The process can be added to the list only after
    // all its drivers have been processed (-> in End_Drivers_Section())
}


void End_Bus()
{
    APPEND_TO_LIST (Bus, (system_ast->buses), bus);
}


void End_Device()
{
    APPEND_TO_LIST (Device, drivers, device);
}

Connection *Get_Connection()
{
    return connection;
}

void End_Connection()
{
    APPEND_TO_LIST (Connection, (system_ast->connections), connection);
}



/* Set Current Process variable in order to give the possibility
 * to add/remove some bindings to it
*/
void Set_Current_Process(Process * p)
{
    process = p;
}

/* Set Current FV variable in order to give the possibility
 * to add/remove Interfaces to it */
void Set_Current_FV(FV * f)
{
    fv = f;
}


void PrintBinding(Aplc_binding * b)
{
    printf("\t%s\n", b->fv->name);
}


void Dump_Bindings()
{
    printf("process \"%s\" bindings: \n", process->name);
    FOREACH(b, Aplc_binding, process->bindings, {
            printf("\t%s\n", b->fv->name);})
}

void Add_Binding(char *b, size_t length)
{
    FV *result_fv = NULL;
    Create_Aplc_binding(&binding);

    if (NULL == binding) {
        ERROR ("[ERROR] in Add_Binding->Create_Aplc_binding\n");
        return;
    }

    /* Keep compatibility with 1.2 - remove objXXX suffix */
    length = remove_objXXX_suffix (b, length);

    binding_name = NULL;
    build_string(&binding_name, b, length);

    result_fv = FindFV(binding_name);

    /* Add the binding to the list (only for active functions
     * i.e. not an unprotected/protected one) */
    if (NULL != result_fv) {

        binding->fv = result_fv;
        APPEND_TO_LIST(Aplc_binding, process->bindings, binding);
        result_fv->process = process;
    } else {
        ERROR ("[ERROR] Binding references a non-existing FV...(%s)\n",
               binding_name);
        add_error();
    }

    if (NULL != binding_name) {
        free(binding_name);
        binding_name = NULL;
    }
}

/* End of process-related functions */

/* FV-related functions */

/* Create a new functional view (starting point) */
FV *New_FV(char *fv_name, size_t length, char *caseSensitive)
{
    Create_FV(&fv);
    assert (NULL != fv);

    build_string(&(fv->name), fv_name, length);
    build_string(&(fv->nameWithCase), caseSensitive, length);

    return fv;
}

void Set_Interface_Queue_Size (const unsigned long long s)
{
   if (interface != NULL) {
      interface->queue_size = s;
   }
}

/* New interface: set the name and distant FV to which it is connected */
void New_Interface(char *name,         size_t length,
                   char *dist_fv,      size_t distant_length,
                   char *distant_name, size_t dist_name_length,
                   IF_type direction)
{
    interface = NULL;
    Create_Interface(&interface);
    assert (NULL != interface);

    build_string(&(interface->name), name, length);

    /* RI can have a local identifier which may be different from
     * the corresponding PI it is connected to. In that case we set
     * the "distant_name" field of the interface to make sure that
     *  the connection will be properly handled
     */
    if (NULL != distant_name) {
        build_string (&(interface->distant_name),
                      distant_name, dist_name_length);
    }

    /* Set the port name as the interface name, so that if later
     * the interface name is changed, we still know to which port we
     * have to connect it (in case of distributed systems */
    build_string (&(interface->port_name), name, length);

    if (distant_length > 0) {
        build_string (&(interface->distant_fv), dist_fv, distant_length);
    }
    else {
        interface->distant_fv = NULL;
    }
    interface->direction      = direction;
    interface->wcet_low       = 0;
    interface->wcet_high      = 0;
    interface->wcet_low_unit  = NULL;
    interface->wcet_high_unit = NULL;
    interface->queue_size     = 1;

    /* ignore params will be kept to true if all callers of a given PI
     * are located in the same node (binary) - in that case the parameters
     * will not be copied from the sender to the receiver through the orb
     * but made available directly through global pointers.
     */
    interface->ignore_params = true;
}

/* New Provided interface */
void Add_PI(char *pi, size_t length)
{
   length = remove_objXXX_suffix (pi, length);

   /* Parameters 3 is NULL because a PI's distant FV is irrelevant
    * (there can be several) and parameter 5 is NULL because a PI's
    *  distant name is also irrelevant since there can also
    *  be several callers (thus several different distant names).
   */
   New_Interface(pi, length, NULL, 0, NULL, 0, PI);
}

/* New Required Interface */
void Add_RI(char *ri, size_t length,
            char *dist_fv, size_t distant_length,
            char *dist_name, size_t dist_name_length)
{
   size_t tmp             = 0; 
   char   *local_name     = ri;

   assert (NULL != ri);
   assert (NULL != dist_fv);
   assert (NULL != dist_name);

   tmp = remove_objXXX_suffix (ri, length);
   dist_name_length = remove_objXXX_suffix (dist_name, dist_name_length);

   /* If the first 3 characters of the local RI name are "obj" we suppose
    * it is a name generated randomly by TASTE-IV. In that case we replace
    * the local RI name with the name of the remote PI.
    * This is kept only for backward compatibility
    */
   if (tmp >= 4 && !strncmp (ri, "obj", 3)) {
        local_name = dist_name;
        length = dist_name_length;
   }
   else length = tmp;

   New_Interface(local_name, length,
                 dist_fv, distant_length,
                 dist_name, dist_name_length,
                 RI);
}

/* add an IN parameter to the list (name respects the case) */
void Add_In_Param(char *name,     size_t l1,
                  char *type,     size_t l2,
                  char *module,   size_t l3,
                  char *filename, size_t l4)
{
    type = asn2underscore(type, l2);

    Create_Parameter(&parameter);
    assert (NULL != parameter && NULL != interface);
    char *param_name = name;

    if (0 == system_ast->context->keep_case) {
        // user does not want to keep case of the parameters
        param_name = string_to_lower(name);
    }

    build_string(&(parameter->name), param_name, l1);
    build_string(&(parameter->type), type, l2);
    build_string(&(parameter->asn1_module), module, l3);
    build_string(&(parameter->asn1_filename), filename, l4);
    parameter->interface = interface;
    parameter->param_direction = param_in;

    Add_Param(&parameter, &(interface->in));
}

/* add an OUT parameter to the list */
void Add_Out_Param(char *name, size_t l1, 
                   char *type, size_t l2, 
                   char *module, size_t l3,
                   char *filename, size_t l4)
{
    type = asn2underscore(type, l2);

    Create_Parameter(&parameter);
    assert (NULL != parameter && NULL != interface);
    char *param_name = name;

    if (0 == system_ast->context->keep_case) {
        // user does not want to keep case of the parameters
        param_name = string_to_lower(name);
    }

    build_string(&(parameter->name), param_name, l1);
    build_string(&(parameter->type), type, l2);
    build_string(&(parameter->asn1_module), module, l3);
    build_string(&(parameter->asn1_filename), filename, l4);
    parameter->interface = interface;
    parameter->param_direction = param_out;

    Add_Param(&parameter, &(interface->out));
}

void End_IF()
{
    if (NULL == fv || NULL == interface)
        return;
    /*
       Determine if the interface is immediate or deffered
       It is deduced fromm the RCMOperationKind attribute
     */

    if (unknown == interface->synchronism) {
        switch (interface->rcm) {
        case cyclic:
        case sporadic:
        case variator:
            interface->synchronism = asynch;
            break;
        case protected:
        case unprotected:
            interface->synchronism = synch;
            break;
        default:
            if (NULL == interface->out) {
                interface->synchronism = asynch;
            } else {
                interface->synchronism = synch;
            }
            break;
        }
    }

    interface->parent_fv = fv;

    /* Add new interface to the list contained in the Function definition */
    APPEND_TO_LIST (Interface, fv->interfaces, interface);
}

/* Last function to call: add the new FV to the System AST */
void End_FV()
{
    assert (NULL != fv);

    /* Check if FV has timers - store in list */
    fv->timer_list = timers(fv);

    fv->system_ast = system_ast;

    /* Determine if the function is a thread or a passive function
     * (thread if only one single PI, and this PI is cyclic or sporadic) */
    int count_pis = 0;
    int count_async_pis = 0;

    FOREACH(i, Interface, fv->interfaces, {
        if (PI == i->direction) {
            count_pis ++;
        }
        if (sporadic == i->rcm || cyclic == i->rcm) {
            count_async_pis ++;
        }
    });
    if (1 == count_pis && 1 == count_async_pis) {
        fv->runtime_nature = thread_runtime;
    }
    else {
        fv->runtime_nature = passive_runtime;
    }

    APPEND_TO_LIST(FV, system_ast->functions, fv);

    fv = NULL;
}

/* End of FV-related functions */


/* AST helper functions */


/*  Functions to find a FV or an Interface based on a char string */
void SetSearchName(char *name)
{
    search_name = name;
}

/* Find FV functions */
void CompareFVname(FV * fv_local, FV ** result)
{
    if (NULL == fv_local->name || NULL == search_name)
        return;

    if (!strcmp(fv_local->name, search_name)) {
        *result = fv_local;
        SetSearchName(NULL);
    }
}

/* main function to be called */
FV *FindFV(char *fv_name)
{
    FV *result_fv = NULL;
    SetSearchName(fv_name);
    FOREACH(f, FV, system_ast->functions, {
        CompareFVname(f, &result_fv);
    });
    SetSearchName(NULL);

    return result_fv;
}

/* End Find FV functions */

/* FindInterface of a given FV */
void CompareIFname(Interface * i, Interface ** result)
{
    if (NULL == i->name || NULL == search_name)
        return;

    if (!strcmp(i->name, search_name)) {
        *result = i;
        SetSearchName(NULL);
    }
}

Interface *FindInterface(FV * function, char *interface_name)
{
    Interface *result = NULL;
    SetSearchName(interface_name);
    FOREACH(i, Interface, function->interfaces, {
            CompareIFname(i, &result);})
        SetSearchName(NULL);

    return result;
}

/* FindParameter of a given Interface */
void CompareParName(Parameter * p, Parameter ** result)
{
    if (NULL == p->name || NULL == search_name)
        return;

    if (!strcmp(p->name, search_name)) {
        *result = p;
        SetSearchName(NULL);
    }
}

Parameter *FindInParameter(Interface *i, char *param_name)
{
    Parameter *result = NULL;
    SetSearchName(param_name);
    FOREACH(p, Parameter, i->in, {
            CompareParName(p, &result);})
        SetSearchName(NULL);

    return result;
}

Parameter *FindOutParameter(Interface * i, char *param_name)
{
    Parameter *result = NULL;
    SetSearchName(param_name);
    FOREACH(p, Parameter, i->out, {
            CompareParName(p, &result);})
        SetSearchName(NULL);

    return result;
}

/* Given a PI (from any function) and a known caller, find the corresponding
 * RI in the remote function
*/
Interface *FindCorrespondingRI(FV *remote, Interface *pi)
{
    FOREACH (i, Interface, remote->interfaces, {
        if (RI == i->direction &&
            !strcmp (i->distant_name, pi->name) &&
            !strcmp (i->distant_fv, pi->parent_fv->name)) {
            return i;
        }
    });
    return NULL;
}

/* End Find Interface functions */

/* Return the list of FV calling a given PI
   the conditions are, for each RI of each FV that :
        - RI's distant name = PI's name
        - RI's distant FV = PI's parent FV
*/
FV_list *Find_All_Calling_FV(Interface * i)
{
    FV_list *result = NULL;
    bool match = false;

    FOREACH (function, FV, system_ast->functions, {
        match = false;
        if (false == function->is_component_type) {
            FOREACH(iface, Interface, function->interfaces, {
                if (false == match) {
                    if (RI == iface->direction) {
                        if (iface->distant_name == NULL || iface->distant_fv == NULL) {
                            printf("[ERROR] Required Interface %s in function %s is not "
                                   "connected. Glue code cannot be generated.\n",
                                   iface->name, function->name);
                            add_error();
                        }
                    }
                    if (NULL != iface->distant_name && NULL != i->name &&
                        NULL != iface->distant_fv && NULL != i->parent_fv &&
                        RI == iface->direction &&
                        !strcmp (iface->distant_name, i->name) &&
                        !strcmp (iface->distant_fv, i->parent_fv->name))
                            match = true;
                }
            })
        }
        if (true == match) {
            APPEND_TO_LIST (FV, result, function);
        }
    })
    if (get_context()->test && NULL != result) {
        printf("\n[Interface %s in FV %s] is called by:\n",
                i->name,
                i->parent_fv->name);
        PRINT_LIST (FV, result);
    }

    return result;
}

/* End Find Calling PI functions */

/* First function to call to initialize the AST */
void C_Init()
{
    Create_System(&system_ast);
}

/* Remove the current system AST from memory */
void Delete_System_AST()
{
    Clear_System(system_ast);
}

/* Get dataview file name based (same as dataview.aadl
 * passed with -d but replace with .asn extension
*/
char *getASN1DataView()
{
    char *p = strrchr (get_context()->dataview,'/');
    if (NULL == p) p = get_context()->dataview - 1;

    return make_string ("%.*s.asn", strlen (p + 1) - strlen (".aadl"), p + 1);
}

/* Get dataView path */
char *getDataViewPath()
{
    char *p = strrchr (get_context()->dataview,'/');

    if (NULL == p) return make_string (".");

    else return make_string
                ("%.*s",
                p - get_context()->dataview,
                get_context()->dataview);
}

/* Debug functions */
void Print_Interface(Interface * i)
{
    printf("*** %s interface \"%s\"\n",
           (PI == i->direction) ? "provided" : "required", i->name);
    printf("\tparent_fv = %s\n", i->parent_fv->name);
    printf("\tsynchronism = %s\n",
           (synch == i->synchronism) ? "sync" : "async");
    printf("\trcm = ");
    switch (i->rcm) {
    case cyclic:
        printf("cyclic\n");
        break;
    case sporadic:
        printf("sporadic\n");
        break;
    case variator:
        printf("variator\n");
        break;
    case protected:
        printf("protected\n");
        break;
    case unprotected:
        printf("unprotected\n");
        break;
    case undefined:
        printf("undefined\n");
        break;
    }
    printf("\tperiod/miat = %lld\n", i->period);
    printf("\tdistant_fv = %s\n", i->distant_fv);
    printf("\tdistant_name = %s\n", i->distant_name);
    printf("\twcet = %llu %s .. %llu %s\n",
           (unsigned long long) i->wcet_low,
           i->wcet_low_unit,
           (unsigned long long) i->wcet_high,
           i->wcet_high_unit);
   printf("\tignore params = %s\n\n", i->ignore_params ? "true": "false");
}

void Dump_Interfaces(Interface_list * l)
{
    FOREACH(i, Interface, l, {
            Print_Interface(i);})
}
/* End AST builder functions */
