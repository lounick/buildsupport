/* Buildsupport is (c) 2008-2016 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* build_vhdl_skeletons.c
 *
 * VHDL Code skeletons - generate mini-cv.addl and call aadl2glueC
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <assert.h>

#include "my_types.h"
#include "practical_functions.h"
#include "backends.h"

void vhdl_skeletons(FV *fv)
{
    char *cmd = NULL;
    cmd = make_string("cp %s/dataview-uniq.asn %s/dataview-uniq-v1.aadl %s/%s",
                      OUTPUT_PATH,
                      OUTPUT_PATH,
                      OUTPUT_PATH,
                      fv->name);
    if (system(cmd)) {
        ERROR ("[ERROR] Missing dataview-uniq.asn or dataview-uniq-v1.aadl\n");
        exit(-1);
    }
    free(cmd);
    printf("[INFO] Generating VHDL skeleton code for function %s\n", fv->name);
    cmd = make_string("cd %s/%s && aadl2glueC dataview-uniq-v1.aadl mini_cv.aadl",
                      OUTPUT_PATH,
                      fv->name);
    if (system(cmd)) {
        ERROR ("[ERROR] Failed generating VHDL code!\n");
        free(cmd);
    }
    else {
        free(cmd);
        cmd = make_string("cd %s/%s && rm -f mini_cv.aadl dataview-uniq.asn "
                          "dataview-uniq-v1.aadl C_ASN1_Types.? ESA_FPGA.h "
                          "*_VHDL.VHDL.[ch] *_vhdl.ad?",
                          OUTPUT_PATH,
                          fv->name,
                          fv->name,
                          fv->name);
        system(cmd);
        free(cmd);
    }

}

/* External interface (the one and unique) */
void GW_VHDL_Backend(FV * fv)
{
    if (get_context()->onlycv)
        return;
    if (vhdl == fv->language || vhdl_brave == fv->language) {
        GLUE_MiniCV_Backend(fv);
        if (!get_context()->glue) {
            vhdl_skeletons(fv);
        }
    }
}
