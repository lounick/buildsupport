/* Buildsupport is (c) 2008-2016 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* Practical functions used by the backends */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>

#include "practical_functions.h"

/* Convert ASN.1 types: change '-' to '_' */
char *asn2underscore(char *s, size_t len)
{
    unsigned int i = 0;
    char *result = (char*) malloc(len + 1);

    assert(NULL != result);

    for (i = 0; i < (unsigned int)len; i++) {
        result[i] = ('-' == s[i])? '_': s[i];
    }
    result[len] = '\0';

    return result;
}

/* Convert a string to lowercase - create a new string */
char *string_to_lower(char *str)
{
    char *newstr;
    size_t len;
    unsigned int tmp;
    len = strlen(str);
    newstr = calloc(len + 1, 1);
    assert (NULL != newstr);
    for (tmp = 0; tmp < (unsigned int)len; tmp++) {
        newstr[tmp] = (char)tolower(str[tmp]);
    }
    newstr[len] = 0;
    return newstr;
}


/* Convert underscore ('_')to dash ('-') - create a new string */
char *underscore_to_dash(char *s, size_t len)
{
    char *out = (char *) malloc(len + 1);
    unsigned int i = 0;

    assert(NULL != out);

    for (i = 0; i < (unsigned int)len; i++)
        if ('_' == s[i])
            out[i] = '-';
        else
            out[i] = s[i];

    out[len] = '\0';

    return out;
}

/* Remove any _objXXX suffixes possibly added by TASTE-IV/TASTE-CV */
size_t remove_objXXX_suffix(char *str, size_t len)
{
    size_t objlen = len;
    unsigned int i = 0;

    /* 1) find the last occurence of '_' in the name string */
    for (i = 0; i < (unsigned int)len; i++)
        if ('_' == str[i])
            objlen = i;

    /* 2) check that it is followed by "obj" - if so, change the size of the str */
    if (objlen >= len - 3 || strncmp(str + objlen + 1, "obj", 3))
        objlen = (unsigned int)len;

    return objlen;
}


/* functions potentially called by the ADD_TO_SET macro */
int Compare_String (String *one, String *two)
{
        assert (NULL != one && NULL != two);
        return (!strcmp (one, two));
}

int Compare_Package(Package *one, Package *two)
{
    return Compare_String(one, two);
}

int Compare_Interface (Interface *one, Interface *two)
{
        char *name1 = one->name;
        char *name2 = two->name;

        assert (NULL != one && NULL != two);

        if (NULL != one -> distant_name) name1 = one->distant_name;
        if (NULL != two -> distant_name) name2 = two->distant_name;

        return (!strcmp (name1, name2));
}

int Compare_FV (FV *one, FV *two)
{
    return (!strcmp(one->name, two->name));
}

int Compare_Protected_Object_Name (Protected_Object_Name *one, Protected_Object_Name *two)
{
    assert (NULL != one && NULL != two);
    return (!strcmp (one, two));
}


int Compare_ASN1_Filename(ASN1_Filename * one, ASN1_Filename * two)
{
    assert (NULL != one && NULL != two);
    return (!strcmp(one, two));
}

int Compare_ASN1_Module(ASN1_Module * one, ASN1_Module * two)
{
  assert (NULL != one && NULL != two);
  return (!strcmp(one, two));
}

int Compare_ASN1_Type(ASN1_Type * one, ASN1_Type * two)
{
 assert (NULL != one && NULL != two);
 return (!strcmp(one->name, two->name));
}

int Compare_Port (Port *one, Port *two)
{
  assert (NULL != one && NULL != two);
  return (!strcmp(one, two));
}

void Clear_Port (Port **p)
{
    free (*p);
    (*p) = NULL;
}

void Print_FV (FV *fv)
{
        printf ("(%s) \"%s\"", 
                thread_runtime == fv->runtime_nature ? "THREAD" : "PASSIVE FUNCTION",
                fv->name);
}


/* 
 * Allocate a string and print to it (from http://linux.die.net/man/3/snprintf
 *
*/

char *make_string(const char *fmt, ...)
{
    /* Guess we need no more than 50 bytes. */
    int n;
    size_t size = 50;
    char *p, *np;
    va_list ap;

    if ((p = malloc(size)) == NULL)
        return NULL;

    while (1) {
        /* Try to print in the allocated space. */
        va_start(ap, fmt);
        n = vsnprintf(p, size, fmt, ap);
        va_end(ap);
        /* If that worked, return the string. */
        if (n > -1 && n < (int)size)
            return p;
        /* Else try again with more space. */
        if (n > -1)             /* glibc 2.1 */
            size = (size_t)n + 1; /* precisely what is needed */
        else                    /* glibc 2.0 */
            size *= 2;          /* twice the old size */
        if ((np = realloc(p, size)) == NULL) {
            free(p);
            return NULL;
        } else {
            p = np;
        }
    }
}


/* Build a textual string (ending with '\0') */
char *build_string(char **str, char *text_to_add, size_t length)
{
    char *old_string = NULL;

    if (NULL == *str) {
        *str = (char *) malloc(length + 1);
        if (NULL == *str) {
            return NULL;
        }
        memcpy(*str, text_to_add, length);
        (*str)[length] = '\0';
    } else {
        old_string = (char *) malloc(strlen(*str) + 1);
        if (NULL == old_string) {
            return NULL;
        }
        strcpy(old_string, *str);
        *str = (char *) realloc(*str, strlen(*str) + length + 1);
        if (NULL == *str) {
            return NULL;
        }
        strcpy(*str, old_string);
        strncat(*str, text_to_add, length);
        (*str)[strlen(old_string) + length] = '\0';
        free(old_string);
    }
    return *str;
}


/* Build a textual string made of elements separated by commas */
char *build_comma_string(char **str, char *text_to_add, size_t length)
{
    if (NULL != *str)
        build_string(str, ",", 1);
    build_string(str, text_to_add, length);

    return *str;
}

/* Creates a new file in a subdirectory */
int create_file(char *fv_name, char *file, FILE ** f)
{
    char *filename = NULL;
    char *current_dir = ".";

    if (NULL == fv_name) {
        fv_name = current_dir;
    }

    else {
        mkdir(fv_name, 0700);   // 00700 = S_IRWXU
    }

    filename = make_string ("%s/%s", fv_name, file);
    assert(NULL != filename);

    *f = fopen(filename, "w");

    assert(NULL != *f);

    free(filename);
    return 0;
}

void close_file(FILE ** f)
{
    if (NULL != *f) {
        fclose(*f);
        *f = NULL;
    }
}

/* Checks if a file exists */
int file_exists(char *fv_name, char *file)
{
    char *filename = NULL;
    FILE *f = NULL;

    filename = make_string ("%s/%s", fv_name, file);
    assert(NULL != filename);

    if (NULL == (f = fopen(filename, "r"))) {
        free(filename);
        return 0;
    }

    else {
        fclose(f);
    }
    free(filename);

    return 1;
}


// Create a new APLC binding type
void Create_Aplc_binding(Aplc_binding ** a)
{
    if (NULL != *a) {
        *a = NULL;
    }

    *a = (Aplc_binding *) malloc(sizeof(**a));

    if (NULL != *a) {
        (*a)->fv = NULL;
    }
}

// Free memory used by an APLC binding type
void Clear_Aplc_binding(Aplc_binding * a)
{
    if (NULL == a)
        return;

    if (NULL != a->fv) {
        a->fv = NULL;
    }
}

// Clear an Aplc_binding_list type
void Clear_Aplc_bindings_List(Aplc_binding_list * p)
{
    Aplc_binding_list *next = NULL;

    while (NULL != p) {

        Clear_Aplc_binding(p->value);
        free(p->value);
        p->value = NULL;

        next = p->next;
        free(p);
        p = next;
    }
}

// Allocate memory for a Process type
void Create_Process(Process ** p)
{
    if (NULL != *p) {
        *p = NULL;
    }

    *p = (Process *) malloc(sizeof(**p));
    if (NULL != *p) {
        (*p)->cpu = NULL;
        (*p)->name = NULL;
        (*p)->processor_board_name = NULL;
        (*p)->identifier = NULL;
        (*p)->bindings = NULL;
        (*p)->drivers = NULL;
        (*p)->connections = 0;
        (*p)->coverage = false;
    }
}

// Free memory used by a Process type
void Clear_Process(Process * p)
{
    if (NULL == p)
        return;

    if (NULL != p->name) {
        free(p->name);
        p->name = NULL;
    }

    if (NULL != p->processor_board_name) {
        free(p->processor_board_name);
        p->processor_board_name = NULL;
    }

    if (NULL != p->identifier) {
        free(p->identifier);
        p->name = NULL;
    }

    if (NULL != p->bindings) {
        Clear_Aplc_bindings_List(p->bindings);
    }

    if (p->cpu != NULL) {
        if (p->cpu->name != NULL) {
            free(p->cpu->name);
        }
        if (p->cpu->classifier != NULL) {
            free(p->cpu->classifier);
        }
        free(p->cpu);
    }

    if (NULL != p->drivers) {
        Clear_Devices_List (p->drivers);
    }
}

// Clear a Processes_list type
void Clear_Processes_List(Process_list * p)
{
    Process_list *next = NULL;

    while (NULL != p) {

        Clear_Process(p->value);
        free(p->value);
        p->value = NULL;

        next = p->next;
        free(p);
        p = next;
    }
}

void Clear_Device(Device * dev)
{
    if (NULL != dev->name) {
        free(dev->name);
    }

    if (NULL != dev->classifier) {
        free(dev->classifier);
    }

    if (NULL != dev->configuration) {
        free(dev->configuration);
    }

    if (NULL != dev->asn1_filename) {
        free (dev->asn1_filename);
    }

    if (NULL != dev->associated_processor) {
        free(dev->associated_processor);
    }

    if (NULL != dev->accessed_bus) {
        free(dev->accessed_bus);
    }

    if (NULL != dev->access_port) {
        free(dev->access_port);
    }
}


void Clear_Devices_List(Device_list * p)
{
    Device_list *next = NULL;

    while (NULL != p) {

        Clear_Device(p->value);
        free(p->value);
        p->value = NULL;

        next = p->next;
        free(p);
        p = next;
    }
}


void Clear_Packages_List(Package_list * p)
{
    Package_list *next = NULL;

    while (NULL != p) {

        free(p->value);
        p->value = NULL;

        next = p->next;
        free(p);
        p = next;
    }
}

void Clear_Connection(Connection * conn)
{
    if (conn->src_system != NULL) {
        free(conn->src_system);
    }
    if (conn->src_port != NULL) {
        free(conn->src_port);
    }
    if (conn->bus != NULL) {
        free(conn->bus);
    }
    if (conn->dst_system != NULL) {
        free(conn->dst_system);
    }
    if (conn->dst_port != NULL) {
        free(conn->dst_port);
    }
}

void Clear_Connections_List(Connection_list * p)
{
    Connection_list *next = NULL;

    while (NULL != p) {

        Clear_Connection(p->value);
        free(p->value);
        p->value = NULL;

        next = p->next;
        free(p);
        p = next;
    }
}


// Remove a binding from a process
void Remove_Binding(FV * fv)
{
    Aplc_binding_list *l = NULL, *next = NULL;

    // First check that there is a binding for this FV. If not, return.
    if (NULL == fv->process)
        return;

    next = fv->process->bindings;
    l = next;

    while (NULL != next) {
        if (next->value->fv == fv) {
            if (next == l) {
                fv->process->bindings = l->next;
                free(next);
                next = NULL;
            } else {
                l->next = next->next;
                free(next);
                next = NULL;
            }
        } else {
            l = next;
            next = l->next;
        }
    }
}

// Clear a list of FV
void Clear_FV_List(FV_list * p)
{
    FV_list *next = NULL;

    while (NULL != p) {

        Clear_FV(p->value);
        free(p->value);
        p->value = NULL;

        next = p->next;
        free(p);
        p = next;
    }
}


/* Allocate memory for a Parameter type */
void Create_Parameter(Parameter ** p)
{
    if (NULL != *p) {
        *p = NULL;
    }

    *p = (Parameter *) malloc(sizeof(**p));
    assert(NULL != *p);

    (*p)->name = NULL;
    (*p)->type = NULL;
    (*p)->asn1_module = NULL;
    (*p)->asn1_filename = NULL;
    (*p)->encoding = default_encoding;
    (*p)->interface = NULL;
    (*p)->param_direction = param_in;
}

/* Free memory used by a Parameter type */
void Clear_Parameter(Parameter * p)
{
    if (NULL == p)
        return;

    if (NULL != p->name) {
        free(p->name);
        p->name = NULL;
    }

    if (NULL != p->type) {
        free(p->type);
        p->type = NULL;
    }
    /*
       if (NULL != p->asn1_module) {
       free(p->asn1_module);
       p->asn1_module = NULL;
       } */
}

/* Add an IN or OUT parameter to the list of parameters for the current PI or RI */
void Add_Param(Parameter ** parameter, Parameter_list ** ret)
{
    Parameter_list *p = NULL;
    Parameter_list *nav = NULL;

    p = (Parameter_list *) malloc(sizeof(*p));
    if (NULL != p) {

        if (NULL != parameter) {
            p->value = *parameter;
            //*parameter = NULL;  /* Don't clear it here, otherwise the 'encoding' property cannot be set */
        }

        p->next = NULL;

        if (NULL == *ret) {
            *ret = p;
        } else {
            nav = *ret;
            while (NULL != nav->next)
                nav = nav->next;
            nav->next = p;
        }
    }
}

// Clear a Parameter_list type
void Clear_Param_List(Parameter_list * p)
{
    Parameter_list *next = NULL;

    while (NULL != p) {

        Clear_Parameter(p->value);
        free(p->value);
        p->value = NULL;

        next = p->next;
        free(p);
        p = next;
    }
}

// this function allocates memory for a new Distant_QGen
void Create_Qgen(Distant_QGen ** i)
{
    assert (NULL == *i);

    *i = (Distant_QGen *) malloc(sizeof(**i));

    assert(NULL != *i);

    (*i)->fv_name = NULL;
    (*i)->language = other;
    (*i)->qgen_init = NULL;
}

// this function allocates memory for a new Interface
void Create_Interface(Interface ** i)
{
    assert (NULL == *i);

    *i = (Interface *) malloc(sizeof(**i));

    assert(NULL != *i);

    Distant_QGen *distant_qgen = NULL;

    Create_Qgen(&distant_qgen);

    (*i)->name = NULL;
    (*i)->port_name = NULL;
    (*i)->in = NULL;
    (*i)->out = NULL;
    (*i)->synchronism = unknown;
    (*i)->rcm = undefined;
    (*i)->period = 0;
    (*i)->queue_size = 1;
    (*i)->wcet_high_unit = NULL;
    (*i)->wcet_low_unit = NULL;
    (*i)->distant_fv = NULL;
    (*i)->distant_qgen = distant_qgen;
    (*i)->calling_threads = NULL;
    (*i)->distant_name = NULL;
    (*i)->calling_pis = NULL;
}

// this function clears up an Interface data structure
void Clear_Interface(Interface * i)
{

    assert(NULL != i);

    if (NULL != i->name) {
        free(i->name);
        i->name = NULL;
    }
    if (NULL != i->port_name) {
        free(i->port_name);
        i->port_name = NULL;
    }

    if (NULL != i->in) {
        Clear_Param_List(i->in);
    }

    if (NULL != i->out) {
        Clear_Param_List(i->out);
    }

    if (NULL != i->wcet_low_unit) {
        free(i->wcet_low_unit);
        i->wcet_low_unit = NULL;
    }

    if (NULL != i->wcet_high_unit) {
        free(i->wcet_high_unit);
        i->wcet_high_unit = NULL;
    }


    if (NULL != i->distant_fv) {
        free(i->distant_fv);
        i->distant_fv = NULL;
    }

    if (NULL != i->distant_qgen) {
        free(i->distant_qgen);
        i->distant_qgen = NULL;
    }


    if (NULL != i->distant_name) {
        free(i->name);
        i->name = NULL;
    }

    /* Don' t try to free the lists here */
    i->calling_threads = NULL;
    i->calling_pis = NULL;
}


// Clear memory of an Interface_list chained list
void Clear_Interfaces_List(Interface_list * l)
{

    Interface_list *next = NULL;

    while (NULL != l) {
        Clear_Interface(l->value);
        free(l->value);
        l->value = NULL;

        next = l->next;
        free(l);
        l = next;
    }
}

// Duplicate a parameter, with the exception of the "interface" field, which is context-dependent
Parameter *Duplicate_Parameter(Parameter * p)
{
    Parameter *new_p = NULL;

    Create_Parameter(&new_p);

    assert(NULL != new_p);

    build_string(&(new_p->name), p->name, strlen(p->name));
    build_string(&(new_p->type), p->type, strlen(p->type));
    build_string(&(new_p->asn1_module), p->asn1_module, strlen (p->asn1_module));
    build_string(&(new_p->asn1_filename), p->asn1_filename, strlen (p->asn1_filename));

    new_p->basic_type = p->basic_type;
    new_p->encoding = p->encoding;
    new_p->param_direction = p->param_direction;
    // The "interface" field has to be set separately


    return new_p;
}

// Duplicate all the elements of a list of parameters
Parameter_list *Duplicate_Param_List(Parameter_list * p_list,
                                     Interface * i)
{
    Parameter_list *new_param_list = NULL;
    Parameter *p = NULL;

    while (NULL != p_list) {

        p = Duplicate_Parameter(p_list->value);
        p->interface = i;

        Add_Param(&p, &new_param_list);

        p_list = p_list->next;
    }

    return new_param_list;
}

// Copy an interface
Interface *Duplicate_Interface(IF_type direction, Interface * i, FV * fv)
{
    Interface *new_if = NULL;

    assert(NULL != i);

    Create_Interface(&new_if);

    if (NULL != new_if) {

        if (NULL != i->name) {
            build_string(&(new_if->name), i->name, strlen(i->name));
        }
        if (NULL != i->port_name) {
            build_string(&(new_if->port_name), i->port_name,
                         strlen(i->port_name));
        }
        if (NULL != i->wcet_low_unit) {
            build_string(&(new_if->wcet_low_unit), i->wcet_low_unit,
                         strlen(i->wcet_low_unit));
        }
        if (NULL != i->wcet_high_unit) {
            build_string(&(new_if->wcet_high_unit), i->wcet_high_unit,
                         strlen(i->wcet_high_unit));
        }
        new_if->wcet_high = i->wcet_high;
        new_if->wcet_low = i->wcet_low;

        if (NULL != i->distant_fv)
            build_string(&(new_if->distant_fv), i->distant_fv,
                         strlen(i->distant_fv));
        if (NULL != i->in)
            new_if->in = Duplicate_Param_List(i->in, new_if);
        if (NULL != i->out)
            new_if->out = Duplicate_Param_List(i->out, new_if);
        if (NULL != i->distant_name)
            build_string(&(new_if->distant_name), i->distant_name,
                         strlen(i->distant_name));

        new_if->synchronism = i->synchronism;
        new_if->rcm = i->rcm;
        new_if->period = i->period;
        new_if->queue_size = i->queue_size;

        new_if->parent_fv = fv;
        new_if->direction = direction;
    }

    return new_if;
}



// Allocate memory for a Context data structure
void Create_Context(Context ** context)
{
    *context = (Context *) malloc(sizeof(**context));

    assert (NULL != *context);

        (*context)->output           = NULL;
        (*context)->ifview           = NULL;
        (*context)->dataview         = NULL;
        (*context)->gw               = 0;
        (*context)->smp2             = false;
        (*context)->glue             = 0;
        (*context)->keep_case        = 0;
        (*context)->onlycv           = 0;
        (*context)->aadlv2           = 0;
        (*context)->test             = 0;
        (*context)->debug            = 0;
        (*context)->polyorb_hi_c     = 0;
        (*context)->stacksize        = NULL;
        (*context)->needs_basictypes = false;
        (*context)->timer_resolution = 100;  // milliseconds
}

// Free the memory of a Context data structure
void Clear_Context(Context * context)
{
    if (NULL != context) {
        if (NULL != context->output)
            free(context->output);
        if (NULL != context->stacksize)
            free(context->stacksize);
        free(context);
    }
}


// Allocate memory for a new FV
void Create_FV(FV ** fv)
{
    *fv = (FV *) malloc(sizeof(**fv));

    if (NULL != *fv) {
        (*fv)->name = NULL;
        (*fv)->nameWithCase = NULL;
        (*fv)->system_ast = NULL;
        (*fv)->interfaces = NULL;
        (*fv)->language = other;
        (*fv)->zipfile = NULL;
        (*fv)->runtime_nature = unknown_runtime;
        (*fv)->calling_threads = NULL;
        (*fv)->thread_id = 0;
        (*fv)->process = NULL;
        (*fv)->context_parameters = NULL;
        /* artificial: for VT-created functions */
        (*fv)->artificial = false;
        /* original_name is set when artificial==true */
        (*fv)->original_name = NULL;
        /* is this function a timer manager? */
        (*fv)->timer = false;
        (*fv)->timer_list = NULL;
    }

}

// Free the memory of an FV structure
void Clear_FV(FV * fv)
{

    if (NULL == fv)
        return;

    if (NULL != fv->name) {
        free(fv->name);
        fv->name = NULL;
    }
    if (NULL != fv->nameWithCase) {
        free(fv->nameWithCase);
        fv->nameWithCase = NULL;
    }

    if (NULL != fv->interfaces) {
        Clear_Interfaces_List(fv->interfaces);
        fv->interfaces = NULL;
    }

    if (NULL != fv->zipfile) {
        free (fv->zipfile);
        fv->zipfile = NULL;
    }

    if (NULL != fv->calling_threads) {
        fv->calling_threads = NULL;
    }
    if (NULL != fv->process) {
        fv->process = NULL;
    }
}


// Allocate memory for the system (AST root)
void Create_System(System ** s)
{
    *s = (System *) malloc(sizeof(**s));

    if (NULL != *s) {
        (*s)->name = NULL;
        Create_Context(&((*s)->context));
        (*s)->functions = NULL;
        (*s)->buses = NULL;
        (*s)->processes = NULL;
        (*s)->packages = NULL;
        (*s)->connections = NULL;
    }
}

// Free the memory of the System AST
void Clear_System(System * s)
{
    if (NULL == s)
        return;

    if (NULL != s->name) {
        free(s->name);
        s->name = NULL;
    }

    Clear_Context(s->context);

    Clear_FV_List(s->functions);

    Clear_Packages_List(s->packages);

    Clear_Connections_List(s->connections);

}

/* Check if a FV has context parameters (exclude Directives, Timers, etc.) */
bool has_context_param(FV *fv)
{
    FOREACH (cp, Context_Parameter, fv->context_parameters, {
        if (strcmp (cp->type.name, "Taste-directive") &&
            strcmp (cp->type.name, "Simulink-Tunable-Parameter") &&
            strcmp (cp->type.name, "Timer"))
               return true;
    });
    return false;
}

/* Check if a FV contains a timer in its context parameters */
bool has_timer(FV *fv)
{
    FOREACH (cp, Context_Parameter, fv->context_parameters, {
        if (!strcmp (cp->type.name, "Timer")) {
            return true;
        }
    });
    return false;
}

/* Return the list of timers of a given FV */
String_list *timers(FV *fv)
{
    String_list *timers_list = NULL;
    FOREACH (cp, Context_Parameter, fv->context_parameters, {
        if (!strcmp (cp->type.name, "Timer")) {
            APPEND_TO_LIST (String, timers_list, cp->fullNameWithCase);
        }
    });
    return timers_list;

}


/* Return the number of RCM-Visible (SPO/CYC/PRO) interfaces from a list */
int CountActivePI(Interface_list *interfaces)
{
    int res = 0;
    FOREACH(pi, Interface, interfaces, {
        if(PI == pi->direction && (asynch == pi->synchronism || protected==pi->rcm)) res ++;
    });
    return res;
}

/* Check if a list of input parameters match a list of output parameters.
 * IMPORTANT : it must be checked first that the number of IN and OUT
 * parameters are the same (using CountParams)
 */
void CheckInOutParams(Parameter * p_in, Parameter_list * p_out)
{
    if (NULL == p_out)
        return;

    if (0 == strcmp(p_in->type, p_out->value->type))
        p_out = p_out->next;
    else
        p_out = NULL;
}

/* 
 * Check if an interface has input or output parameters
 */
void CheckForAsn1Params(Interface * i, int *result)
{
    if (NULL != i->in || NULL != i->out) {
        *result = 1;
    }
}

/* 
 * Write to file the list of parameters in Ada, using the following formatting:
 * IN/OUT_paramName: interface.c.char_array,
 * IN/OUT_paramNamesize: [access] Integer)
 */
void List_Ada_Param_Types_And_Names(Parameter * p, FILE ** file)
{
    unsigned int separator = 0;

    /* Determine if it is the first parameter or not to put either a '(' or a ';' as a separator */
    separator =
        ((p->param_direction == param_in && p != p->interface->in->value)
         || (p->param_direction == param_out
             && (NULL != p->interface->in
                 || p != p->interface->out->value)));

    fprintf(*file, "%s%s_%s: Interfaces.C.char_array; %s_%s_size:%s Integer", separator ? "; " : " ",   /* space instead of "(" */
            (param_in == p->param_direction) ? "IN" : "OUT",
            p->name,
            (param_in == p->param_direction) ? "IN" : "OUT",
            p->name, (param_out == p->param_direction) ? "access" : "");
}

/* Write to file the list of parameters, separated by commas (used for a call function) */
void List_Ada_Param_Names(Parameter * p, FILE ** file)
{
    unsigned int separator = 0;

    // Determine if it is the first parameter or not to put either a '(' or a ';' as a separator
    separator =
        ((p->param_direction == param_in && p != p->interface->in->value)
         || (p->param_direction == param_out
             && (NULL != p->interface->in
                 || p != p->interface->out->value)));

    fprintf(*file, "%s%s_%s, %s_%s_size", separator ? ", " : " ",       // space instead of "("
            (param_in == p->param_direction) ? "IN" : "OUT",
            p->name,
            (param_in == p->param_direction) ? "IN" : "OUT", p->name
            // (param_out==p->param_direction)?"\'access": ""
        );
}

/* Write to file the list of parameters, separated by commas (used for a call function) */
void List_QGen_Param_Types_And_Names(Parameter * p, FILE ** file)
{
    unsigned int separator = 0;

    // Determine if it is the first parameter or not to put either a '(' or a ';' as a separator
    separator =
        ((p->param_direction == param_in && p != p->interface->in->value)
         || (p->param_direction == param_out
             && (NULL != p->interface->in
                 || p != p->interface->out->value)));

    fprintf(*file, "%s%s: access asn1Scc%s", separator ? "; " : " ", p->name, p->type);
}

/* Write to file the list of parameters, separated by commas (used for a call function) */
void List_QGen_Param_Names(Parameter * p, FILE ** file, Language lang)
{
    unsigned int separator = 0;

    // Determine if it is the first parameter or not to put either a '(' or a ';' as a separator
    separator =
        ((p->param_direction == param_in && p != p->interface->in->value)
         || (p->param_direction == param_out
             && (NULL != p->interface->in
                 || p != p->interface->out->value)));

    if ((qgenada == lang) ||
        (qgenc == lang && integer != p->basic_type && real != p->basic_type && boolean != p->basic_type))
        fprintf(*file, "%s%s_%s", separator ? ", " : " ",
            (param_in == p->param_direction) ? "IN" : "OUT", p->name);
    else
        fprintf(*file, "%s%s_%s", separator ? ", " : " ",
            (param_in == p->param_direction) ? "*IN" : "OUT", p->name);
}

/* Create a string in the form "name.all",
 * as expected by QGencAda for arguments of the comp function*/
void Create_QGenAda_Argument(char * name, char **result, bool deref)
{
    if (NULL != *result) {
        build_string(result, ", ", 2);
    }
    build_string(result, name, strlen(name));
    if (deref) {
        build_string(result, ".all", strlen(".all"));
    }
}

// For a given sychronous RI, add "with RI_wrappers;" statement to a file
// Note: TODO: We should check if the line is unique
// (low priority: having several identical "with Package;" lines does not affect the compiler)
void Add_With_RI_Wrapper_to_Ada(Interface * i, FILE ** file)
{
    if (RI == i->direction && synch == i->synchronism)
        fprintf(*file, "with %s_wrappers;\n", i->distant_fv);
}


/*
 * Write to file the list of parameters with their type, separated with commas:
 * const asn1SccTYPE1 *IN_param1, ..., asn1SccTYPEi *OUT_parami
 */
void List_C_Types_And_Params_With_Pointers(Parameter * p, FILE ** file)
{
    unsigned int comma = 0;

    /* Determine if a comma is needed prior to the item to be put in the list. */
    comma =
        ((p->param_direction == param_in && p != p->interface->in->value)
         || (p->param_direction == param_out
             && (NULL != p->interface->in
                 || p != p->interface->out->value)));

    fprintf(*file, "%s%sasn1Scc%s *%s_%s",
            comma ? ", " : "",
            (param_in == p->param_direction) ? "const " : "",
            p->type,
            (param_in == p->param_direction) ? "IN" : "OUT", p->name);
}

/* 
 * Write to file the list of parameters separated with commas
 * form "IN_param1, IN_param2, &OUT_param1....
 */
void List_C_Params(Parameter * p, FILE ** file)
{
    unsigned int comma = 0;

    /* Determine if a comma is needed prior to the item to be put in the list.  */
    comma =
        ((p->param_direction == param_in && p != p->interface->in->value)
         || (p->param_direction == param_out
             && (NULL != p->interface->in
                 || p != p->interface->out->value)));

    fprintf(*file, "%s%s_%s",
            comma ? ", " : "",
            (param_in == p->param_direction) ? "IN" : "&OUT", p->name);
}

// ForEachWithParam function : Write to file the list of parameters + size separated with commas (form "IN_buf_param1, size_IN_buf_param1,.., OUT_buf_parami, &size_OUT_buf_parami....") 
void List_C_Params_And_Size(Parameter * p, FILE ** file)
{
    unsigned int comma = 0;

    // Determine if a comma is needed prior to the item to be put in the list. 
    comma =
        ((p->param_direction == param_in && p != p->interface->in->value)
         || (p->param_direction == param_out
             && (NULL != p->interface->in
                 || p != p->interface->out->value)));

    fprintf(*file, "%s%s_buf_%s, %s_buf_%s",
            comma ? ", " : "",
            (param_in == p->param_direction) ? "IN" : "OUT",
            p->name,
            (param_in == p->param_direction) ? "size_IN" : "&size_OUT",
            p->name);
}

/* For debug: print all model */
void Dump_model(System * s)
{

    FOREACH(fv, FV, s->functions, {
            printf("*** FV \"%s\"\n", fv->name);
            printf(" - runtime nature = %s ; language = %s\n",
                   thread_runtime ==
                   fv->runtime_nature ? "thread" : passive_runtime ==
                   fv->runtime_nature ? "passive" : "unknown",
                   sdl == fv->language ? "sdl" : simulink ==
                   fv->language ? "simulink" : qgenc ==
                   fv->language ? "qgenc" : c ==
                   fv->language ? "c" : ada ==
                   fv->language ? "ada" : qgenada ==
                   fv->language ? "qgenada" : gui ==
                   fv->language ? "gui" : scade ==
                   fv->language ? "scade" : rtds ==
                   fv->language ? "rtds" : blackbox_device ==
                   fv->language ? "blackbox_device" : "unknown");
            printf(" - process : %s\n",
                   NULL != fv->process ? fv->process->name : "unknown");
            printf(" - calling threads:");
            FOREACH(thread, FV, fv->calling_threads, {
                    printf(" %s", thread->name);
                    })
            printf("\n"); FOREACH(i, Interface, fv->interfaces, {
                                  printf(" - %s interface %s\n",
                                         PI ==
                                         i->direction ? "Provided" :
                                         "Required", i->name);
                                  printf("  * %s  - %s\n",
                                         synch ==
                                         i->synchronism ? "sync" : asynch ==
                                         i->synchronism ? "async" :
                                         "unknown sync",
                                         cyclic ==
                                         i->rcm ? "cyclic" : sporadic ==
                                         i->rcm ? "sporadic" : variator ==
                                         i->rcm ? "variator" : protected ==
                                         i->rcm ? "pro" : unprotected ==
                                         i->rcm ? "unpro" : "unknown rcm");
                                  printf
                                  ("  * distant_name = %s - distant_fv = %s\n",
                                   i->distant_name, i->distant_fv);
                                  printf("  * calling threads:");
                                  FOREACH(thread, FV, i->calling_threads, {
                                          printf(" %s", thread->name);
                                          })
                                  printf("\n");})
            })

}


/* 
 * Create a header file with an enumerated type listing all the interfaces
 * of the block. This can be used to create a choice determinant to distinguish
 * the PI/RI when sending a message to the driver.
 */
void create_enum_file(FV *fv)
{
    FILE *f = NULL;
    char *path = NULL;
    path = make_string("%s/%s", OUTPUT_PATH, fv->name);

    create_file(path, "interface_enum.h", &f);
    fprintf (f, 
       "/* This file was generated automatically: DO NOT MODIFY IT ! */\n\n"
       "/*\n"
       " *  Use the enum values at your convenience, if you need them to encode/decode\n"
       " *  messages for your driver with a header field that identifies the sender/receiver\n"
       "*/\n\n");

    bool coma = false;
    int pi_iter=0, ri_iter=0;
    fprintf (f, "enum {\n");
    FOREACH (i, Interface, fv->interfaces, {
       fprintf (f, 
           "%s    i_%s_%s_%s = %d",
               true == coma ? ",\n": "",
               fv->name,
               PI == i->direction? "PI":"RI",
               i->name,
               PI == i->direction? pi_iter++: ri_iter++);
       coma = true;
    })
    fprintf (f, "\n};\n");

    fclose(f);
    free(path);
}

/* Build a call to QGen init function, if it exists in generated code*/
char *Build_QGen_Init(char ** build_str, char *fv_name, char *i_name, Language lang)
{
    DIR             *d = NULL;
    struct dirent   *dir = NULL;
    char            *filename = NULL;
    char            *dirname = NULL;
    char            *qgeninit = NULL;

    dirname = make_string ("../%s", fv_name);
    d = opendir(dirname);
    if (d) {
        while ((dir = readdir (d)) != NULL) {
            filename = make_string("%s/%s", dirname, dir->d_name);
            assert(NULL != filename);
            if ((qgenc == lang) && (strcmp (dir->d_name, make_string("%s.h", string_to_lower(i_name))))) {
                if (Find_Str_In_File (filename, make_string("%s_init", i_name))) {
                    qgeninit = make_string("%s_init()", i_name);
                    }

                free(filename);
            }
            else if ((qgenada == lang) && (strcmp (dir->d_name, make_string("%s.ads", string_to_lower(i_name))))) {
                if (Find_Str_In_File (filename, "procedure init"))
                    qgeninit = make_string("vm_QGen_Init_%s()", i_name);

                free(filename);
            }
        }
    }

    if (qgeninit != NULL)
        build_string (build_str, qgeninit, strlen(qgeninit));

    return *build_str;
}

/* Checks if the string str is found in the file filename (return 1) or not (return 0) */
int Find_Str_In_File(char *filename, char *str)
{
    FILE    *f = NULL;
    int     found = 0;
    char    tmp[512];

    f = fopen(filename, "r");
    assert(NULL != f);

    while(fgets(tmp, 512, f) != NULL) {
        if ((strstr(tmp, str)) != NULL)
            found++;
    }

    fclose(f);
    return found;
}
