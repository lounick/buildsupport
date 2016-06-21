/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
#ifndef PRACTICAL_FUNCTIONS
#define PRACTICAL_FUNCTIONS

#include "my_types.h"
#include "c_ast_construction.h"

#define USE_PO_HI_ADA(fv) (fv->system_ast->context->polyorb_hi_c == 0)
#define USE_PO_HI_C(fv) (fv->system_ast->context->polyorb_hi_c == 1)

#define C_DRIVER (get_context()->polyorb_hi_c)

/* If defined, return the output path, otherwise an empty string */
//#define OUTPUT_PATH(fv) (NULL!=fv->system_ast->context->output?fv->system_ast->context->output:".")
#define OUTPUT_PATH (NULL!=get_context()->output?get_context()->output:".")

/* Return a string representing the selected encoding rules for a Parameter */
#define BINARY_ENCODING(p) (native==p->encoding)?"NATIVE":(uper==p->encoding)?"UPER":(acn==p->encoding)?"ACN":"#ERROR#"

#define LANGUAGE(fv) ada==fv->language?"Ada": c==fv->language?"C": sdl==fv->language?"OG": rtds==fv->language?"RTDS": scade==fv->language?"SCADE6":simulink==fv->language?"SIMULINK": blackbox_device==fv->language? "C": rhapsody==fv->language? "C": vhdl==fv->language?"VHDL": system_c==fv->language?"SYSTEM_C":gui==fv->language?"GUI": qgenc==fv->language?"QGenC": qgenada==fv->language?"QGenAda": cpp==fv->language?"CPP": "UNSUPPORTED_LANGUAGE!"

#define RCM_KIND(IF) (protected==IF->rcm?"protected":unprotected==IF->rcm?"unprotected":sporadic==IF->rcm?"sporadic":cyclic==IF->rcm?"cyclic":variator==IF->rcm?"variator":"unknown")

#define NATURE(fv) thread_runtime==fv->runtime_nature?"thread":passive_runtime==fv->runtime_nature?"passive":"unknown"

#define DIRECTION(i) PI==i->direction?"PI":"RI" 

#define PARAM_DIRECTION(p) param_in==p->param_direction?"param_in":"param_out"

#define SYNCHRONISM(i) synch==i->synchronism?"synch":"asynch"

#define BASIC_TYPE(p) sequenceof==p->basic_type?"sequenceof":sequence==p->basic_type?"sequence":enumerated==p->basic_type?"enumerated":set==p->basic_type?"set":setof==p->basic_type?"setof":integer==p->basic_type?"integer":boolean==p->basic_type?"boolean":real==p->basic_type?"real":choice==p->basic_type?"choice":octetstring==p->basic_type?"octetstring":string==p->basic_type?"string":"None"

#define DEBUG_MSG (get_context()->debug)

/* Error reporting */
#define ERROR(...) fprintf(stderr, __VA_ARGS__)
#define WARNING ERROR
#define INFO ERROR

/* Convert ASN.1 types: change '-' to '_' */
char *asn2underscore(char *, size_t);

/* Convert underscore ('_')to dash ('-') - create a new string */
char *underscore_to_dash(char *s, size_t len);

/* Remove any _objXXX suffixes possibly added by TASTE-IV/TASTE-CV */
size_t remove_objXXX_suffix (char *string, size_t len);

/* create a new lowercase string */
char *string_to_lower(char *str);

/* Create a string including malloc */
char *make_string(const char *fmt, ...);

/* Build a textual string */
char *build_string(char **, char *, size_t);

/* Build a textual string made of elements separated by commas */
char *build_comma_string(char **, char *, size_t);

/* Build a call to QGen init function, if it exists in generated code*/
char *Build_QGen_Init(char **, char *, char *, Language);

/* Checks if the string str is found in the file filename (return 1) or not (return 0) */
int Find_Str_In_File(char *filename, char *str);

/* Create a new file in a subdirectory */
int create_file (char *, char *, FILE **);

/* Checks if a file exists in a subdirectory */
int file_exists(char *, char *);

/* Close a file and reset the FILE handler */
void close_file (FILE **);

/* Free memory used by a Parameter type */
void Clear_Parameter(Parameter *);

/* Add an IN or OUT parameter to the list of parameters for the current PI or RI */
void Add_Param (Parameter **, Parameter_list **);

/* Free the memory of a Param_list type */
void Clear_Param_List (Parameter_list *);

/* Allocate memory for a new Interface */
void Create_Interface (Interface **);

/* Free the memory of an Interface data structure */
void Clear_Interface(Interface *);

/* Free the memory of an Interface_list chained list */
void Clear_Interfaces_List(Interface_list *);

/* Clear a FV_list type */
void Clear_FV_List (FV_list *p);

/* Allocate memory for a new FV */
void Create_FV (FV **);

/* Free the memory of an FV structure */
void Clear_FV(FV *);

void Clear_Device (Device *dev);
void Clear_Devices_List(Device_list * p);

void Create_APLC_binding(Aplc_binding **a);

/* Free memory used by an APLC binding type */
void Clear_APLC_binding(Aplc_binding *a);

/* Remove a binding from a process */
void Remove_Binding (FV *fv);

/* Allocate memory for a Process type */
void Create_Process (Process **p);

/* Free memory used by a Process type */
void Clear_Process(Process *p);

/* Clear a Processes_list type */
void Clear_Processes_List (Process_list *p);

/* Clear an Aplc_bindings_list type */
void Clear_Aplc_bindings_List (Aplc_binding_list *p);

/* Check if an interface has input or output parameters (can be used with ForEachWithParam)
 Like that:
  int hasparam=0;
  ForEachWithParam element of fv->interfaces, execute CheckForAsn1Params and update &hasparam thanks;
    if (hasparam){
      (...)
    }
*/
void CheckForAsn1Params (Interface *i, int *result);

/* Check if a list of input parameters match a list of output parameters. */
void CheckInOutParams (Parameter *p_in, Parameter_list *p_out);

/* Check if a FV has context parameters (exclude Directives, Timers, etc.) */
bool has_context_param(FV *fv);

/* Check if a FV contains a timer in its context parameters */
bool has_timer(FV *fv);

/* Return the list of timers of a given FV */
String_list *timers(FV *fv);

/* Return the number of Cyclic and Sporadic interfaces from a list */
int CountActivePI(Interface_list *interfaces);

/*
  ForEachWithParam function : Write to file the list of parameters in Ada 
 (form IN/OUT_paramName: interface.c.char_array, IN/OUT_paramNamesize: [access] Integer) 
*/
void List_Ada_Param_Types_And_Names(Parameter *p, FILE **file);

/*
   ForEachWithParam function : Write to file the list of parameters, separated by commas
*/
void List_Ada_Param_Names(Parameter *p, FILE **file);

/* 
  ForEachWithParam function : Write to file the list of parameters in Ada for QGen 
 (form IN/OUT_paramName: interface.c.char_array) 
*/
void List_QGen_Param_Types_And_Names(Parameter *p, FILE **file);

/*
   ForEachWithParam function : Write to file the list of parameters, separated by commas for QGen
*/
void List_QGen_Param_Names(Parameter *p, FILE **file, Language lang);

/* Create a string in the form "name.all",
 * as expected by QGencAda for arguments of the comp function*/
void Create_QGenAda_Argument(char * name, char **result, bool deref);

/* For a given sychronous RI, add "with RI_wrappers;" statement to a file
  Note: TODO: We should check if the line is unique
  (low priority: having several identical "with Package;" lines does not affect the compiler)
*/
void Add_With_RI_Wrapper_to_Ada(Interface *i, FILE **file);

/* Copy an interface */
Interface *Duplicate_Interface (IF_type direction, Interface *i, FV *fv);

/* Duplicate all the elements of a list of parameters */
Parameter_list *Duplicate_Param_List(Parameter_list *p_list, Interface *i);

/* Duplicate a parameter, with the exception of the "interface" field, which is context-dependent */
Parameter *Duplicate_Parameter (Parameter *p);

/* Write to file the list of parameters with their type, separated with commas
 * (form "const asn1SccTYPE1 IN_param1, ...") */
void List_C_Types_And_Params_With_Pointers(Parameter *p, FILE **file);

/* ForEachWithParam function : Write to file the list of parameters separated with commas (form "IN_param1, IN_param2, &OUT_param1....") */
void List_C_Params(Parameter *p, FILE **file);


/* ForEachWithParam function : Write to file the list of parameters + size separated with commas (form "IN_param1, size_IN_param1,.., OUT_parami, &size_OUT_parami....") */
void List_C_Params_And_Size (Parameter *p, FILE **file);

/* Allocate memory for the system (AST root) */
void Create_System (System **s);

/* Free the memory of the System AST */
void Clear_System (System *s);

/* Count a number of FVs in a list */
void CountFV (FV *fv, int *count);

/* Create a new APLC binding type */
void Create_Aplc_binding(Aplc_binding **a);

/* Allocate memory for a Parameter type */
void Create_Parameter (Parameter **p);

/* Dump the model for debug purposes */
void Dump_model(System *);

/* Add an error the the top-level error count */
extern void add_error();



/* function potentially called by the ADD_TO_SET macro */
int Compare_Package(Package *one, Package *two);
int Compare_Protected_Object_Name(Protected_Object_Name *one, Protected_Object_Name *two);
int Compare_Interface (Interface *one, Interface *two);
int Compare_FV (FV *one, FV *two);
int Compare_String (String *one, String *two);
int Compare_ASN1_Filename(ASN1_Filename *one, ASN1_Filename *two);
int Compare_ASN1_Module (ASN1_Module *one, ASN1_Module *two);
int Compare_ASN1_Type (ASN1_Type *one, ASN1_Type *two);
int Compare_Port (Port *one, Port *two);
void Clear_Port (Port ** p);
void Print_FV (FV *fv);


/* New FOREACH macro.. Supports online statements!
Example of use:

        with "MyType_list *l;" and l containing elements

                FOREACH(tmp, MyType,l, {
                        printf("%s",tmp);
                        printf(" ");
                        }
                )

*/
#define FOREACH(var,type,list,stmt) {type##_list *varlist##var##type=list; type *var;while(varlist##var##type!=NULL) { var=varlist##var##type->value; stmt varlist##var##type=varlist##var##type->next; }}

#define CREATE_NEW_LIST(type, list) \
{\
assert(NULL == list);\
list=(type##_list *) malloc(sizeof(type##_list));\
}


#define APPEND_TO_LIST(type, list, val) \
{\
type##_list *newlist=NULL;\
assert(NULL != val);\
CREATE_NEW_LIST(type, newlist);\
assert(NULL!=newlist);\
newlist->value=val;\
newlist->next=NULL;\
if(NULL==list) list = newlist;\
else {\
type##_list *nav=list;\
while (NULL != nav->next) nav=nav->next;\
nav->next=newlist; }\
}


/* 
 * ADD_TO_SET : make sure the list contains UNIQUE elements 
 * The user has to provide the Compare_type function
 */
#define ADD_TO_SET(type, list, val) \
{\
int result = 0; \
FOREACH(elem, type, list, {\
if(Compare_##type(val,elem)) result = 1; \
});\
if (!result) APPEND_TO_LIST(type, list, val);\
}

/* return true if value of type sort is in set */
#define IN_SET(sort, set, val) \
__extension__({\
bool result = false; \
FOREACH(elem, sort, set, {\
if(Compare_##sort(val,elem)) result = true; \
});\
result;\
})

#define FREE_LIST(type, list) \
{\
type##_list *next=NULL;\
while(NULL != list) {\
Clear_##type(&(list->value));\
free(list->value);\
list->value=NULL;\
next=list->next;\
free(list);\
list=next;}\
}

#define REMOVE_FROM_LIST(type, list, val) \
{\
type##_list *tmp=list;\
if(list->value==val) list=list->next;\
else while (NULL != tmp) {\
if(tmp->next->value==val) {tmp->next=tmp->next->next;break;}\
tmp=tmp->next;}\
}

#define PRINT_LIST(type, list) \
{\
type##_list *next=list;\
do {\
printf("[");\
Print_##type(next->value);\
printf("]");\
next=next->next;\
}\
while (NULL != next);\
}
 

int concat_file (const char* source, const char* dest);

void create_enum_file(FV *fv);

#endif /* PRACTICAL_FUNCTIONS */
