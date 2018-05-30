/* Buildsupport is (c) 2008-2016 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* Types used for description of the C AST of TASTE systems */
/* 1st version written by Maxime Perrotin/ESA 14/01/2008 */

#ifndef _MY_TYPES_H_
#define _MY_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

#define PROCESSOR_NULL {NULL,NULL}

static const char do_not_modify_warning[] =
         "/* This file was generated automatically: DO NOT MODIFY IT ! */\n\n";

/* Macro available to declare chained list in a standard format */
#define DECLARE_LIST(type) \
typedef struct t_##type##_list {\
        type *value;\
        struct t_##type##_list *next;\
} type##_list;

/* Generic type for a string and a list of strings */
typedef char String;
DECLARE_LIST (String)


/* Interface type : Provided (PI) or Required (RI) */
typedef enum {
    PI,
    RI
} IF_type;

/* Interface kind : immediate/synchronous (sync)
 * or deferred/asynchronous (async) */
typedef enum {
    synch,
    asynch,
    unknown
} IF_kind;

/* Parameter direction (in or out) */
typedef enum {
    param_in,
    param_out
} Param_kind;

/* Supported encoding rules: uPER, Native (no encoding), ACN (User-defined) */
typedef enum {
    uper,
    native,
    acn,
    default_encoding
} Encoding;

/* List of supported languages */
typedef enum {
    sdl,
    simulink,
    qgenc,
    qgenada,
    c,
    cpp,
    opengeode,
    ada,
    rhapsody,
    gui,
    scade,
    rtds,
    vhdl,
    vhdl_brave,
    system_c,
    blackbox_device,
    vdm,
    micropython,
    other
} Language;

/* Interface property */
typedef enum {
   cyclic,
   sporadic,
   variator,
   protected,
   unprotected,
   undefined
} RCM;

/* Basic ASN.1 type */
typedef enum {
    sequenceof,
    sequence,
    enumerated,
    set,
    setof,
    integer,
    boolean,
    real,
    choice,
    octetstring,
    string,
    unknown_type
} ASN1_basic_type;


/* Function nature: thread or shared resource */
typedef enum {
    thread_runtime,
    passive_runtime,
    unknown_runtime
} Nature;

/* Type used to store a Protected Object name */
typedef char Protected_Object_Name;
DECLARE_LIST (Protected_Object_Name)

/* Type used to store a process port name */
typedef char Port;
DECLARE_LIST (Port)

/* Type used to store an ASN.1 module name */
typedef char ASN1_Module;
DECLARE_LIST (ASN1_Module)

/* Type used to store an ASN.1 filename */
typedef char ASN1_Filename;
DECLARE_LIST (ASN1_Filename)

/* Type used to store an ASN.1 Type name */
typedef char ASN1_Typename;

/* Type used to store an ASN.1 Type definition */
typedef struct t_asntype {
    ASN1_Typename       *name;
    ASN1_Module         *module;
    ASN1_Filename       *asn1_filename;
    ASN1_basic_type     basic_type;
} ASN1_Type;

DECLARE_LIST (ASN1_Type)

/* Type used to store an AADL property */
typedef struct t_property {
    String *name;
    String *value;
} AADL_Property;

DECLARE_LIST (AADL_Property)

/* Type used to define a context parameter (functional state) */
typedef struct t_contextparam {
  char          *name;
  char          *fullNameWithCase;
  ASN1_Type     type;
  char          *value;
} Context_Parameter;

DECLARE_LIST (Context_Parameter)

/* 
  Type used to define a parameter's attributes (name, type, encoding)
*/
typedef struct t_parameter {
  char                  *name;
  char                  *type;
  ASN1_Module           *asn1_module;
  ASN1_basic_type       basic_type;
  ASN1_Filename         *asn1_filename;
  Encoding              encoding;
  struct t_interface    *interface;
  Param_kind            param_direction;
} Parameter;

DECLARE_LIST (Parameter)
/*
   Type used in build_gui_glue to list the PI/RI of a FV
   (Internal to GUI glue from Cyril) - should use a standard type instead
*/
typedef struct T_name_list
{
  char          *name;
  struct        T_name_list *next;
} T_RI_PI_NAME_LIST;

/*
   Type used to describe an interface (provided or required)
*/
typedef struct t_interface {
  char                    *name;
  Port                    *port_name;
  struct t_fv             *parent_fv;
  IF_type                 direction;
  Parameter_list          *in;
  Parameter_list          *out;
  IF_kind                 synchronism;
  RCM                     rcm;
  long long               period;
  uint64_t                wcet_low;
  char                    *wcet_low_unit;
  uint64_t                wcet_high;
  char                    *wcet_high_unit;
  char                    *distant_fv;
  struct t_qgen           *distant_qgen;
  struct t_FV_list        *calling_threads;
  char                    *distant_name;
  unsigned long long      queue_size;
  bool                    ignore_params;
  struct t_Interface_list *calling_pis; // only set in RIs of passive functions
} Interface;

DECLARE_LIST (Interface)

/*
 * Type used to store context-dependent parameters (eg. output directory)
 * (initialized in practical_functions.c, update it when you add a field)
*/
typedef struct t_context {
  char  *output;
  char  *ifview;
  char  *dataview;
  int   glue;
  bool  smp2;
  int   gw;
  int   keep_case;
  int   onlycv;
  int   aadlv2;
  int   debug;
  int   test;
  bool  future;
  char  *stacksize;
  int   polyorb_hi_c;
  bool  needs_basictypes;
  int   timer_resolution;
} Context;

/*
 * Type used to describe a function
*/
typedef struct t_fv {
  char                   *name;
  char                   *nameWithCase;
  struct                 t_system *system_ast;
  Nature                 runtime_nature;
  Language               language;
  char                   *zipfile;
  Interface_list         *interfaces;
  struct t_process       *process;
  struct t_FV_list       *calling_threads;
  int                    thread_id;
  Context_Parameter_list *context_parameters;
  AADL_Property_list     *properties;
  bool                   artificial;
  char                   *original_name;
  bool                   timer;
  struct t_String_list   *timer_list;
  bool                   is_component_type;
  char                   *instance_of;
} FV;

DECLARE_LIST(FV)

/*
 * Type used to define an APLC binding (needed to build concurrency view)
*/
typedef struct t_aplc_binding {
  FV    *fv;
} Aplc_binding;

DECLARE_LIST(Aplc_binding)

/* 
 * Type used to define a processor (used in deployment view)
 */
typedef struct t_processor {
   char *name;
   char *classifier;
   char *platform_name;
   char *envvars;
   char *user_cflags;
   char *user_ldflags;
} Processor;

/*
 * Type used to define a bus (used in deployment view)
 */
typedef struct t_bus {
   char *name;
   char *classifier;
} Bus;

DECLARE_LIST(Bus)

/*
 * Type used to define a device (used in deployment view)
 */
typedef struct t_device {
   char                 *name;
   char                 *classifier;
   char                 *associated_processor;
   char                 *configuration;
   ASN1_Filename        *asn1_filename;
   ASN1_Typename        *asn1_typename;
   ASN1_Module          *asn1_modulename;
   char                 *accessed_bus;
   Port                 *access_port;
} Device;

DECLARE_LIST(Device)

/*
 * Type used to declare a connection between two functions
 * of the interface view, through a port
 */
typedef struct t_connection {
   char *src_system; 
   Port *src_port;
   char *bus;
   char *dst_system;
   Port *dst_port;
} Connection;

DECLARE_LIST(Connection)
/*
  Type used to define a PROCESS (Partition in deployment view)
*/
typedef struct t_process {
  char                  *name;
  char                  *identifier;
  char                  *processor_board_name;
  Processor             *cpu;
  Interface_list        *interfaces;
  Aplc_binding_list     *bindings;
  Device_list           *drivers;
  unsigned int          connections;
  bool                  coverage;
} Process;

DECLARE_LIST(Process)

typedef char Package;

DECLARE_LIST(Package)

/*
 Type used to store the entire system (root of the Interface & Deployment AST)
*/
typedef struct t_system
{
    char              *name; // always "deploymentview"
    Context           *context;
    FV_list           *functions;
    Process_list      *processes;
    Bus_list          *buses;
    Package_list      *packages;
    Connection_list   *connections;
} System;

/*
 * Type used to store information about connected QGen subsystems
 */
typedef struct t_qgen {
   char                 *fv_name;
   Language             language;
   char                 *qgen_init;
} Distant_QGen;

#endif
