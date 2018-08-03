/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*
 *  Generate ASN.1 modules containing the declaration of
 *  context parameters, based on the "functional state" variables
 *  declared in the interface view, then call asn1.exe to
 *  compile it and make it part of the code skeletons.
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>

#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"



void Process_Context_Parameters(FV *fv)
{
    ASN1_Module_list *module_list   = NULL;
    ASN1_Type_list   *type_list     = NULL;
    char    *command                = NULL;
    FILE    *asn                    = NULL;
    char    *fv_no_underscore       = NULL;
    char    *cp_no_underscore       = NULL;
    int     will_fail               = 0 ;
    char    *dataview_uniq          = NULL;
    char    *dataview_path          = NULL;

    ASN1_Filename *filename = NULL;

    /* For the moment context parameters only apply to C/C++, Ada and
     * Blackbox_Device (= C) languages
     */
    if (c != fv->language && ada != fv->language && cpp != fv->language &&
            blackbox_device != fv->language && qgenada != fv->language) {
        return;
    }

    if (NULL == fv->context_parameters || false == has_context_param(fv)) {
        return;
    }

    /* 
     * Build lists (sets) of ASN.1 input files, modules, and types
    */
    FOREACH (cp, Context_Parameter, fv->context_parameters, {
         if (strcmp (cp->type.name, "Taste-directive") &&
             strcmp (cp->type.name, "Simulink-Tunable-Parameter") &&
             strcmp(cp->type.name, "Timer")) {
                 ADD_TO_SET (ASN1_Module, module_list,
                     cp->type.module);
                 ADD_TO_SET (ASN1_Type, type_list, &(cp->type));
         }
    });

    /* 
     * Make sure the FV name has no underscore (to be ASN.1-friendly)
     */
    fv_no_underscore = underscore_to_dash (fv->name, strlen(fv->name));

    /* 
     * Create the ASN.1 file to store the context parameters 
     */
    filename = make_string ("Context-%s.asn", fv_no_underscore);
    asn = fopen (filename, "wt");
    assert (NULL != asn);

    fprintf(asn, "Context-%s DEFINITIONS ::=\n", fv_no_underscore);
    fprintf(asn, "BEGIN\n");

    /* 
     * Import the ASN.1 modules containing the type definitions 
     */
    fprintf(asn, "IMPORTS ");
    FOREACH (type, ASN1_Type, type_list, {
       fprintf(asn, "%s FROM %s\n", type->name, type->module);
    });
    fprintf(asn, ";\n");

    /*
     * Create a type that wraps all context parameters to form a namespace.
     * In the future this namespace shall comme automatically from Asn1Scc
     */
    fprintf(asn, "\nContext-%s ::= SEQUENCE {", fv_no_underscore);
    bool first = true;
    FOREACH (cp, Context_Parameter, fv->context_parameters, {
            if (strcmp (cp->type.name, "Taste-directive") &&
                strcmp (cp->type.name, "Simulink-Tunable-Parameter") &&
                strcmp(cp->type.name, "Timer")) {
                cp_no_underscore = underscore_to_dash
                                        (cp->name, strlen(cp->name));
                if (false == first) {
                    fprintf(asn, ",");
                }
                first = false;
                fprintf(asn, "\n\t%s %s",
                    cp_no_underscore,
                    cp->type.name);
                free (cp_no_underscore);
            }
    })
    fprintf(asn,"\n}\n");

    /* 
     * And finally, declare a variable of the Context parameter type.
     */
    fprintf(asn, "\n%s-ctxt Context-%s ::= {",
            fv_no_underscore,
            fv_no_underscore);
    first = true;
    FOREACH (cp, Context_Parameter, fv->context_parameters, {
        if (strcmp (cp->type.name, "Taste-directive") &&
            strcmp (cp->type.name, "Simulink-Tunable-Parameter") &&
            strcmp(cp->type.name, "Timer")) {
            cp_no_underscore = underscore_to_dash
                                    (cp->name, strlen(cp->name));
            if (false == first) {
                fprintf(asn, ",");
            }
            first = false;
            fprintf(asn, "\n\t%s %s",
                cp_no_underscore,
                cp->value);
            free (cp_no_underscore);
        }
    })
    fprintf(asn, "\n}\n");

    fprintf(asn, "END\n");
    fclose (asn);

    /* 
     * Next part: call asn1.exe with the newly-created ASN.1 file 
    */

    dataview_uniq = getASN1DataView();
    dataview_path = getDataViewPath();

    if (!file_exists (dataview_path, dataview_uniq)) {
        ERROR ("[INFO] %s/%s not found."
               " Checking for dataview-uniq.asn\n",
               dataview_path, dataview_uniq);
        free (dataview_uniq);
        dataview_uniq = make_string ("dataview-uniq.asn");
    }

    if (!file_exists (dataview_path, dataview_uniq)) {
        ERROR ("[ERROR] %s/%s not found\n",
               dataview_path, dataview_uniq);
        exit (-1);
    }

    command = make_string
            ("mono $(which asn1.exe) -%s -typePrefix asn1Scc -o %s/%s %s %s/%s",
            ada==fv->language? "Ada": "c",
            OUTPUT_PATH,
            fv->name,
            filename,
            dataview_path, 
            dataview_uniq);

    ERROR ("%s %s\n", 
            will_fail? "Because of the above warning(s), "
                       "the following command cannot be executed :\n":
            "Executing",
            command);

    if (will_fail) {
        ERROR ("Fix the warnings and restart buildsupport, "
               "or run it manually, you need it!\n");
        exit (-1);
    }

    if (!will_fail && system (command)) {
        ERROR ("[ERROR] The command failed. Try it yourself "
               "(correct paths, access to files, etc.)\n");
        exit (-1);
    }
    else if (!get_context()->test) {
        remove (filename);
    }

    free (filename);
    free (command);
    free (fv_no_underscore);
    free (dataview_uniq);
    free (dataview_path);

}
