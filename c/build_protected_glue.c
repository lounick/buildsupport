/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* build_protected_glue.c

  this backend generated the glue code for protected and unprotected objects
  (Ada only)

 */

#define ID "$Id$"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "my_types.h"
#include "practical_functions.h"

static FILE *sync_adb = NULL, *sync_ads = NULL;


/* Add a protected provided interface. */
void add_sync_PI(Interface * i)
{
    if (qgenc == i->parent_fv->language || qgenada == i->parent_fv->language) {
        return;
    }
    /* Count the number of calling threads for this passive function */
    int count = 0;
    FOREACH(t, FV, i->parent_fv->calling_threads, {
            (void) t; count++;});

    fprintf(sync_adb,
            "\t------------------------------------------------------\n");
    fprintf(sync_adb, "\t--  %srotected Provided Interface \"%s\"\n",
            (protected == i->rcm) ? "P" : "Unp", i->name);
    fprintf(sync_adb,
            "\t------------------------------------------------------\n");
    fprintf(sync_ads,
            "\t------------------------------------------------------\n");
    fprintf(sync_ads, "\t--  %srotected Provided Interface \"%s\"\n",
            (protected == i->rcm) ? "P" : "Unp", i->name);
    fprintf(sync_ads,
            "\t------------------------------------------------------\n");

    /* Declare the Ada procedure in the source and body files */
    fprintf(sync_ads, "\tprocedure %s (calling_thread: integer", i->name);
    fprintf(sync_adb, "\tprocedure %s (calling_thread: integer", i->name);

    if (NULL != i->in || NULL != i->out) {
        fprintf(sync_ads, ";");
        fprintf(sync_adb, ";");
    }
    /* add IN and OUT parameters */
    FOREACH(p, Parameter, i->in, {
        List_Ada_Param_Types_And_Names(p, &sync_ads);
        List_Ada_Param_Types_And_Names(p, &sync_adb);
    })
    FOREACH(p, Parameter, i->out, {
        List_Ada_Param_Types_And_Names(p, &sync_ads);
        List_Ada_Param_Types_And_Names(p, &sync_adb);
    })
    fprintf(sync_ads, ");\n");
    fprintf(sync_adb, ")");

    fprintf(sync_adb, " \n\tis\n");

    /* Declare the external C function in the body */
    fprintf(sync_adb, "\t\tprocedure C_%s", i->name);

    if (NULL != i->in || NULL != i->out)
        fprintf(sync_adb, "(");

    /* add IN and OUT parameters */
    FOREACH(p, Parameter, i->in, {
        List_Ada_Param_Types_And_Names(p, &sync_adb);
    })
    FOREACH(p, Parameter, i->out, {
        List_Ada_Param_Types_And_Names(p, &sync_adb);
    })
    
    if (NULL != i->in || NULL != i->out)
        fprintf(sync_adb, ")");
    fprintf(sync_adb, ";\n");

    /* Import the C function in the body */
    fprintf(sync_adb, "\t\tpragma import (C, C_%s, \"%s_%s\");\n\n",
            i->name, i->parent_fv->name, i->name);

    fprintf(sync_adb, "\t\tbegin\n");
    if (count > 1) {
        fprintf(sync_adb, "\t\t\tcallinglist.push(calling_thread);\n");     /* Put the calling thread in the stack */
    }

    /* Call the C function in the body */
    fprintf(sync_adb, "\t\t\tC_%s", i->name);

    if (NULL != i->in || NULL != i->out)
        fprintf(sync_adb, "(");

    /* add IN and OUT parameters */
    FOREACH(p, Parameter, i->in, {
        List_Ada_Param_Names(p, &sync_adb);
    })
    FOREACH(p, Parameter, i->out, {
        List_Ada_Param_Names(p, &sync_adb);
    })

    if (NULL != i->in || NULL != i->out) {
        fprintf(sync_adb, ")");
    }

    fprintf(sync_adb, ";\n");
    
    /* Remove the calling thread from the stack before returning*/
    if (count > 1) {
        fprintf(sync_adb, "\t\t\tcallinglist.pop;\n");
    }

    /* End of the procedure body */
    fprintf(sync_adb, "\tend %s;\n\n", i->name);

}

/* Function to process a protected PI */
void Protected_Interface(Interface * i)
{
    if (NULL == i)
        return;
    if (protected == i->rcm && PI == i->direction)
        add_sync_PI(i);

}

/* Function to process an unprotected PI */
void Unprotected_Interface(Interface * i)
{
    if (NULL == i)
        return;
    if (unprotected == i->rcm && PI == i->direction)
        add_sync_PI(i);

}

/* If the function has protected interfaces, add the code to support them in the wrapper file. */
void Add_Protected_Interfaces(FV * fv, FILE * pro_ads, FILE * pro_adb)
{
    bool haspro = false;
    if (fv->system_ast->context->onlycv)
        return;

    FOREACH(i, Interface, fv->interfaces, {
        if (PI == i->direction && protected == i->rcm) {
            haspro = true;
        }
    })

    if (false == haspro)
        return;

    sync_ads = pro_ads;
    sync_adb = pro_adb;

    fprintf(sync_ads,
            "-- Protected object to guarantee mutual exclusion between the protected interfaces of the function\n\n");
    fprintf(sync_adb,
            "-- Protected object to guarantee mutual exclusion between the protected interfaces of the function\n\n");

    fprintf(sync_ads, "protected protected_%s is\n", fv->name);
    fprintf(sync_adb, "protected body protected_%s is\n", fv->name);

    FOREACH(i, Interface, fv->interfaces, {
        Protected_Interface(i);
    })

    fprintf(sync_ads, "end protected_%s;\n", fv->name);
    fprintf(sync_adb, "end protected_%s;\n", fv->name);
}

/* If the function has unprotected provided interfaces, add the code to support them in the wrapper file. */
void Add_Unprotected_Interfaces(FV * fv, FILE * unpro_ads,
                                FILE * unpro_adb)
{
    bool hasunpro = false;
    if (fv->system_ast->context->onlycv)
        return;

    FOREACH(i, Interface, fv->interfaces, {
        if (PI == i->direction && unprotected == i->rcm) {
            hasunpro = true;
        }
    })

    if (false == hasunpro)
        return;

    sync_ads = unpro_ads;
    sync_adb = unpro_adb;
    /*ForEach element of fv->interfaces,
        execute Unprotected_Interface thanks;*/
    FOREACH(i, Interface, fv->interfaces, {
        Unprotected_Interface (i);
    })
    
}
