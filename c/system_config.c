/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*
 *
 * System configuration - Optimization
 * Create a .h file included by C_ASN1_Types.h with some #defines to
 * select which ASN.1 type encoder/decoders are needed
 *
 * 1st version by MP 12/09/2014
 * (c) ESA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include "my_types.h"

#include "practical_functions.h"

static FILE *syscfg;


void Process_Interface(Interface *i)
{

    FOREACH (param, Parameter, i->in, {
        fprintf(syscfg, "#define __NEED_%s_%s\n",
                         param->type,
                         BINARY_ENCODING(param));
    });
    FOREACH (param, Parameter, i->out, {
        fprintf(syscfg, "#define __NEED_%s_%s\n",
                         param->type,
                         BINARY_ENCODING(param));
    });
}


/* API Entry */
void System_Config(System *ast)
{
    create_file(OUTPUT_PATH, "system_config.h", &syscfg);
    assert(NULL != syscfg);

    fprintf(syscfg,
         "/* This file was generated automatically - DO NOT MODIFY IT ! */\n\n"
         "/* Configuration file used by C_ASN1_Types.h */\n\n");


    FOREACH(fv, FV, ast->functions, {
        FOREACH(i, Interface, fv->interfaces, {
            Process_Interface(i);
        });
    });


    close_file(&syscfg);

}

