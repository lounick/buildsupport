/* Buildsupport is (c) 2008-2016 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*

(mini-)Concurrency view backend: Generates one AADL file per functional view
in order to generate the glue code for each PI/RI using Semantix'AADL2GlueC tool

1st version written by Maxime Perrotin/ESA 01/04/2008
updated 8-8-8 with GUI language
updated 20/04/2009 to disable in case "-onlycv" flag is set

Copyright 2014-2015 IB Krates <info@krates.ee>
      QGenc code generator integration

*/

#define ID "$Id: concurrency_view.c 300 2009-07-15 12:47:25Z maxime1008 $"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "my_types.h"

#include "practical_functions.h"

#define ENCODING(i) (sdl==i->parent_fv->language)?"SDL":\
            (simulink==i->parent_fv->language)?"Simulink":\
            (micropython==i->parent_fv->language)?"MicroPython":\
            (qgenc==i->parent_fv->language)?"QGenC":\
            (ada==i->parent_fv->language)?"Ada":\
            (vdm==i->parent_fv->language)?"Vdm":\
            (qgenada==i->parent_fv->language)?"QGenAda":\
            (rtds==i->parent_fv->language)?"RTDS":\
            (scade==i->parent_fv->language)?"SCADE6":\
            (vhdl==i->parent_fv->language)?"VHDL":\
            (vhdl_brave==i->parent_fv->language)?"VHDL_BRAVE":\
            (system_c==i->parent_fv->language)?"SYSTEM_C":\
            (gui==i->parent_fv->language && PI==i->direction)?"GUI_PI":\
            (gui==i->parent_fv->language && RI==i->direction)?"GUI_RI":\
            (rhapsody==i->parent_fv->language)? "Rhapsody":"C"

static FILE *cv;

/* Create a new file for the Concurrency view */
void Init_MiniCV_Backend(FV * fv)
{
    char *path = NULL;
    if (NULL != fv->system_ast->context->output)
        build_string(&path, fv->system_ast->context->output,
                     strlen(fv->system_ast->context->output));
    build_string(&path, fv->name, strlen(fv->name));

    create_file(path, "mini_cv.aadl", &cv);
    free(path);

    if (NULL != cv) {
        fprintf(cv,
                "-- Automatically Generated - For use with AADL2GlueC only - Do NOT modify! --\n\n");
    }
}

/* Add a new AADL SUBPROGRAM to the Concurrency view*/
void Add_Subprogram(Interface * i)
{
    if (NULL == cv)
        return;

    fprintf(cv, "SUBPROGRAM %s\n", i->name);

    /* If the PI has parameters, add a FEATURE section */
    if (NULL != i->in || NULL != i->out) {
        fprintf(cv, "FEATURES\n");

        FOREACH(param, Parameter, i->in, {
            fprintf(cv,
                    "\t%s:%s PARAMETER DataView::%s {encoding=>%s;};\n",
                    param->name,
                    (PI == i->direction) ? "IN" : "OUT",
                    param->type,
                    BINARY_ENCODING(param));
        });


        FOREACH(param, Parameter, i->out, {
            fprintf(cv,
                    "\t%s:%s PARAMETER DataView::%s {encoding=>%s;};\n",
                    param->name,
                    (PI == i->direction) ? "OUT" : "IN",
                    param->type,
                    BINARY_ENCODING(param));
        });

    }

    fprintf(cv, "END %s;\n\n", i->name);

    fprintf(cv, "SUBPROGRAM IMPLEMENTATION %s.%s\nPROPERTIES\n",
            i->name, ENCODING(i));

    fprintf(cv, "\tFV_Name => \"%s\";\n", i->parent_fv->name);
    fprintf(cv, "\tSource_Language => %s;\nEND %s.%s;\n\n",
            ENCODING(i), i->name, ENCODING(i));
}

/* Close the Concurrency view */
void End_MiniCV_Backend()
{
    close_file(&cv);
}



/* Function to process one interface of the FV */
void GLUE_MiniCV_Interface(Interface * i)
{
    Add_Subprogram(i);
}

// External interface (the one and unique)
void GLUE_MiniCV_Backend(FV * fv)
{
    if (fv->system_ast->context->onlycv)
        return;

    Init_MiniCV_Backend(fv);

    FOREACH(i, Interface, fv->interfaces, {
        GLUE_MiniCV_Interface(i);
    })

    End_MiniCV_Backend();

}
