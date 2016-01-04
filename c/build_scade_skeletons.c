/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*

SCADE 6.x skeletons generator

MP 14-05-2010
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "my_types.h"
#include "practical_functions.h"

static FILE *f;


void Generate_Scade6_Skeleton(FV * fv)
{
    char *path = NULL;
    char *filename = NULL;
    Interface *i = NULL;

    if (NULL != fv->system_ast->context->output) {
	build_string(&path, fv->system_ast->context->output,
		     strlen(fv->system_ast->context->output));
    }

    build_string(&path, fv->name, strlen(fv->name));

    build_string(&filename, fv->name, strlen(fv->name));
    build_string(&filename, ".xscade", strlen(".xscade"));
    create_file(path, filename, &f);

    free(filename);
    filename = NULL;

    if (NULL == f)
	return;

    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(f,
	    "<File xmlns=\"http://www.esterel-technologies.com/ns/scade/3\" xmlns:ed=\"http://www.esterel-technologies.com/ns/scade/pragmas/editor/2\" xmlns:kcg=\"http://www.esterel-technologies.com/ns/scade/pragmas/codegen/1\">\n");
    fprintf(f, "\t<declarations>\n");
    fprintf(f, "\t\t<Operator kind=\"node\" name=\"%s\">\n", fv->name);

    if (NULL == fv->interfaces) {
	printf("Error - SCADE subsystem with no interface (%s)\n",
	       fv->name);
	return;
    }
    i = fv->interfaces->value;

    if (NULL != i->in) {
	fprintf(f, "\t\t\t<inputs>\n");
	FOREACH(p, Parameter, i->in, {
		fprintf(f, "\t\t\t\t<Variable name=\"%s\">\n", p->name);
		fprintf(f,
			"\t\t\t\t\t<type>\n\t\t\t\t\t\t<NamedType>\n\t\t\t\t\t\t\t<type>\n\t\t\t\t\t\t\t\t<TypeRef name=\"%s\"/>\n",
			p->type);
		fprintf(f,
			"\t\t\t\t\t\t\t</type>\n\t\t\t\t\t\t</NamedType>\n\t\t\t\t\t</type>\n\t\t\t\t</Variable>\n");}
	)
	    fprintf(f, "\t\t\t</inputs>\n");
    }
    if (NULL != i->out) {
	fprintf(f, "\t\t\t<outputs>\n");
	FOREACH(p, Parameter, i->out, {
		fprintf(f, "\t\t\t\t<Variable name=\"%s\">\n", p->name);
		fprintf(f,
			"\t\t\t\t\t<type>\n\t\t\t\t\t\t<NamedType>\n\t\t\t\t\t\t\t<type>\n\t\t\t\t\t\t\t\t<TypeRef name=\"%s\"/>\n",
			p->type);
		fprintf(f,
			"\t\t\t\t\t\t\t</type>\n\t\t\t\t\t\t</NamedType>\n\t\t\t\t\t</type>\n\t\t\t\t</Variable>\n");}
	)
	    fprintf(f, "\t\t\t</outputs>\n");
    }
    fprintf(f, "\t\t</Operator>\n");
    fprintf(f, "\t</declarations>\n");
    fprintf(f, "</File>\n");

    return;
}


// External interface (the one and unique)
void GW_SCADE_Backend(FV * fv)
{
    if (fv->system_ast->context->onlycv)
	return;

    if (scade == fv->language) {
	Generate_Scade6_Skeleton(fv);
    }

}
