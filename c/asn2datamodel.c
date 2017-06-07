/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*
 *  Call 'asn2dataModel' tool to complete the generation
 *  of function skeleton with the data types transformed
 *  from ASN.1 to the target language (C, Ada, RTDS, etc.).
 *   
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdbool.h>
#include <errno.h>

#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"

void Print_ASN1_Filename(ASN1_Filename * f)
{
    printf("%s", f);
}


void Call_asn2dataModel(FV * fv)
{
    //bool hasparam = false;
    char *command = NULL;
    char *dataviewpath = NULL;
    int ret = 0;

    /* There is no A mapper for VHDL/SystemC/GUI */
    if (vhdl == fv->language || system_c == fv->language
        || gui == fv->language)
        return;

    /* check if there is at least one parameter in one interface */
    // Removed - generate data types in any case so that user code
    // can have local variables...
    //FOREACH(i, Interface, fv->interfaces, {
    //        if (NULL != i->in || NULL != i->out) hasparam = true;}
    //);
    //if (false == hasparam)
    //    return;

    dataviewpath = make_string("%s/%s", OUTPUT_PATH, fv->name);

    /*
     * In case of "native" languages,  place the result in a subdirectory
     * called "dataview", so that the code is not mixed up with user code
     */
    if (c == fv->language
        || cpp == fv->language
        || qgenada == fv->language
        || qgenc == fv->language
        || ada == fv->language || blackbox_device == fv->language) {
        build_string(&dataviewpath, "/dataview", strlen("/dataview"));
    }

    ret = mkdir(dataviewpath, 0700);

    if (-1 == ret) {
        switch (errno) {
        case EEXIST:
            break;
        default:
            ERROR("Error creating directory %s\n", dataviewpath);
            add_error();
            return;
            break;
        }
    }

    char *dataview_uniq = getASN1DataView();
    char *dataview_path = getDataViewPath();

    if (!file_exists(dataview_path, dataview_uniq)) {
        printf
            ("[INFO] %s/%s not found. Checking for dataview-uniq.asn\n",
             dataview_path, dataview_uniq);
        free(dataview_uniq);
        dataview_uniq = make_string("dataview-uniq.asn");
    }

    if (!file_exists(dataview_path, dataview_uniq)) {
        ERROR("** Error: %s/%s not found\n", dataview_path, dataview_uniq);
        exit(-1);
    }

    command = make_string("asn2dataModel -o %s -to%s %s/%s",
                          dataviewpath,
                          LANGUAGE(fv), dataview_path, dataview_uniq);

    free(dataview_uniq);
    free(dataview_path);

    if (system(command)) {
        ERROR
            ("** Error: Check this command line and make sure all files are valid:\n%s\n", command);
        exit(-1);
    }
    free(command);
    free(dataviewpath);
}
