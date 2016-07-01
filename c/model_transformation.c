/* Buildsupport is (c) 2008-2016 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* model_transformation.c

   Group all the sporadic messages between two functions into a single
   "Telecommand" with a CHOICE parameter.
   Create the CHOICE datatype in ASN.1 and generate the code that
   mux/demux the message.
   This is done to limit the number of generated threads in a system.
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


/* External interface */
void ModelTransformation_Backend (System *s)
{
    (void) s;
    FOREACH (fv, FV, s->functions, {
        Interface_list *pis = NULL;
        FOREACH (pi, Interface, fv->interfaces, {
            if(sporadic == pi->rcm && PI == pi->direction) {
                ADD_TO_SET(Interface, pis, pi)
            }
        });
    });
}
