/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* build_driver_skeletons.c

  this program generates empty C functions used to implement a driver 
  for TASTE systems. the main difference with standard C skeletons is
  that the input data is kept in encoded format. the driver can use this
  raw value to send directly to the device. symetrically, the required
  interface expect parameters that are already encoded.

  driver code does not need vm_if.c 

  encoded format is specified in the interface view, among uPER, ACN and Native.

 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdbool.h>

#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"

static FILE *driver_c = NULL, 
            *driver_h = NULL;

/* Adds header to user_code.h and (if new) user_code.c */
void driver_gw_preamble(FV * fv)
{
    int hasparam = 0;

    /* Check if any interface needs ASN.1 types */
    FOREACH(i, Interface, fv->interfaces, {
    CheckForAsn1Params(i, &hasparam);
    })

    /* a. driver.h preamble */
    fprintf(driver_h,
        "/* This file was generated automatically: DO NOT MODIFY IT ! */\n\n");
    fprintf(driver_h,
        "/* Declaration of the functions that have to be provided by the user */\n\n");
    fprintf(driver_h,
        "#ifndef __DRIVER_CODE_H_%s__\n#define __DRIVER_CODE_H_%s__\n\n",
        fv->name, fv->name);

    fprintf(driver_h, "#include <stdlib.h>\n\n");

    fprintf(driver_h, "void init_%s();\n\n", fv->name);

    /* b. driver.c preamble (if applicable) */
    if (NULL != driver_c) {
    fprintf(driver_c,
        "/* Functions to be filled by the user (never overwritten by buildsupport tool) */\n\n");
    fprintf(driver_c, "#include \"%s.h\"\n\n", fv->name);

    fprintf(driver_c, "void init_%s()\n{\n", fv->name);

    fprintf(driver_c,
        "\t/* Write your initialization code here,\n\t   but do not make any call to a required interface!! */\n}\n\n");
    }
}

/* Creates driver_code.h, and if necessary driver_code.c (if it did not already exist) */
int Init_Driver_GW_Backend(FV * fv)
{
    char *path = NULL;
    char *filename = NULL;

    if (NULL != get_context()->output) {
    build_string(&path, get_context()->output,
             strlen(get_context()->output));
    }
    build_string(&path, fv->name, strlen(fv->name));

    build_string(&filename, fv->name, strlen(fv->name));
    build_string(&filename, ".h", strlen(".h"));
    create_file(path, filename, &driver_h);

    filename[strlen(filename) - 1] = 'c';   /* change from .h to .c */

    if (!file_exists(path, filename))
    create_file(path, filename, &driver_c);
    else { 
        ERROR ("** Information: driver code not overwritten\n");
        driver_c = NULL;
    }

    free(path);

    /* We only check if the .h file is OK, not the C file since
       it might already pre-exist, in which case we do not touch it */  
    assert (NULL != driver_h); 

    driver_gw_preamble(fv);

    return 0;
}


void close_driver_gw_files()
{
    fprintf(driver_h, "\n#endif\n");
    close_file(&driver_h);
    
    close_file(&driver_c);
}

/* Add a Provided interface. Can contain in and out parameters
 * Write in driver.h the declaration of the user functions
 * and if driver.c is new, copy these declarations there too.
 */
void add_PI_to_driver_gw(Interface * i)
{

    fprintf(driver_h, "void %s_%s(", i->parent_fv->name, i->name);
    if (NULL != driver_c)
    fprintf(driver_c, "void %s_%s(", i->parent_fv->name,
        i->name);

    FOREACH(p, Parameter, i->in, {

    fprintf(driver_h, "%svoid *, size_t",
        (p != i->in->value) ? ", " : "");
    if (NULL != driver_c)
        fprintf(driver_c, "%svoid *IN_%s, size_t size_IN_%s",
            (p != i->in->value) ? ", " : "",
            p->name, 
            p->name);
    });

    FOREACH(p, Parameter, i->out, {

    fprintf(driver_h, "%svoid *, size_t *",
        (p != i->out->value
         || (p == i->out->value
             && NULL != i->in)) ? ", " : "");
    if (NULL != driver_c)
        fprintf(driver_c, "%svoid *OUT_%s, size_t *size_OUT_%s",
            (p != i->out->value
             || (p == i->out->value
             && NULL != i->in)) ? ", " : "", 
            p->name,
            p->name);
    });

    fprintf(driver_h, ");\n\n");
    if (NULL != driver_c) {
    fprintf(driver_c, ")\n{\n\t/*\n\t * Write your code here! \n");
    if (NULL != i->in) {
        fprintf(driver_c, "\t * Note: the parameters are encoded following the rules you specified\n");
        fprintf(driver_c, "\t * in the interface view (Native, uPER, or ACN).\n");
    }
    fprintf(driver_c, "\t */\n");
    fprintf(driver_c, "}\n\n");
    }
}

/* Declaration of the RI in user_code.h */
void add_RI_to_driver_gw(Interface * i)
{
    fprintf(driver_h, "\n/* Required interface \"%s\" */\n", i->name);
    fprintf(driver_h, "extern void vm_%s%s_%s(", 
            asynch == i->synchronism? "async_": "",
            i->parent_fv->name,
        i->name);


    FOREACH(p, Parameter, i->in, {
    fprintf(driver_h, "%svoid *IN_%s, size_t IN_%s_size",
        (p != i->in->value) ? ", " : "", 
        p->name, 
        p->name);
    });


    FOREACH(p, Parameter, i->out, {
    fprintf(driver_h, "%svoid *OUT_%s, size_t *OUT_%s_size",
        (p != i->out->value
         || (p == i->out->value
             && NULL != i->in)) ? ", " : "", 
        p->name,
        p->name);
    });

    fprintf(driver_h, ");\n\n");
}



/* Function to process one interface of the FV */
void GW_Driver_Interface(Interface * i)
{
    switch (i->direction) {
    case PI:
    add_PI_to_driver_gw(i);
    break;

    case RI:
    /* 
     * There can be duplicate RI name but one sync, the other async
     * In that case, discard the async one (generated by VT to allow
     * a mix of sync and async PI in one block
     */
    if (asynch == i->synchronism) {
        FOREACH(interface, Interface, i->parent_fv->interfaces, {
        if (RI == interface->direction &&
            !strcmp (interface->name, i->name) &&
            synch == interface->synchronism) {
            return;
            }

        })
    }
    add_RI_to_driver_gw(i);
    break;
    default:
    break;
    }
}


/* External interface (the one and unique) */
void GW_Driver_Backend(FV * fv)
{
    if (get_context()->onlycv)
    return;

    if (blackbox_device == fv->language) {
        Init_Driver_GW_Backend(fv);
        FOREACH(i, Interface, fv->interfaces, {
            GW_Driver_Interface(i);
        })
        create_enum_file(fv);
        close_driver_gw_files();
    }
}
