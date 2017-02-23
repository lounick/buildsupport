/* Buildsupport is (c) 2008-2016 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* build_vdm_skeletons.c

   Generate code skeletons for VDM functions
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <assert.h>

#include "my_types.h"
#include "practical_functions.h"

static FILE *user_code = NULL, *interface = NULL, *c_bridge;


/* Adds header to user_code.h and (if new) user_code.c */
void vdm_gw_preamble(FV * fv)
{
    assert (NULL != interface && NULL != c_bridge);

    fprintf(interface,
          "-- This file was generated automatically: DO NOT MODIFY IT ! \n\n");

    /* C bridge between TASTE and VDM-generated code */
    fprintf(c_bridge, "%s", do_not_modify_warning);

    /* Check if any interface needs ASN.1 types */
    int hasparam = 0;
    FOREACH(i, Interface, fv->interfaces, {
        CheckForAsn1Params(i, &hasparam);
    });
    if(hasparam) {
        fprintf(c_bridge, "#include \"Vdm_ASN1_Types.h\"\n");
    }

    /* Initialize the VDM class and call the startup function */
    fprintf(c_bridge, "#include \"%s.h\"\n\n"
                      "static TVP %s;\n\n"
                      "void %s_startup()\n"
                      "{\n"
                      "    //%s_const_init();\n"
                      "    //%s_static_init();\n"
                      "    %s = _Z%lu%sEV(NULL);\n"
                      "    CALL_FUNC(%s, %s, %s, CLASS_%s__Z7StartupEV);\n"
                      "}\n\n",
                      fv->name,
                      fv->name,
                      fv->name,
                      fv->name,
                      fv->name,
                      fv->name,
                      (long unsigned int) strlen(fv->name),
                      fv->name,
                      fv->name,
                      fv->name,
                      fv->name,
                      fv->name);

    fprintf(interface,
            "class %s_Interface\n"
            "operations\n"
            "    public Startup: () ==> ()\n"
            "    Startup () is subclass responsibility;\n\n",
            fv->name);


    if (NULL != user_code) {
        fprintf(user_code,
              "-- User code: This file will not be overwritten by TASTE.\n\n");

        fprintf(user_code,
            "class %s\n"
            "is subclass of %s_Interface\n"
            "operations\n"
            "    public Startup: () ==> ()\n"
            "    Startup () == skip; -- fill your code\n\n",
            fv->name,
            fv->name);
    }
}

/* Creates interface and if necessary user code template (if it did not exist) */
void Init_VDM_GW_Backend(FV *fv)
{
    char *path = NULL;
    char *filename = NULL;

    path     = make_string("%s/%s", OUTPUT_PATH, fv->name);
    filename = make_string("%s_interface.vdmpp", fv->name);
    create_file(path, filename, &interface);
    free(filename);
    filename = make_string("%s.vdmpp", fv->name);

    if (!file_exists(path, filename)) {
        create_file(path, filename, &user_code);
    }

    free(filename);
    filename = make_string("%s_bridge.c", fv->name);
    create_file(path, filename, &c_bridge);

    free(filename);

    free(path);
    vdm_gw_preamble(fv);
}


/* Add a Provided interface. Can contain in and out parameters
 * Write in user_code.h the declaration of the user functions
 * and if user_code.c is new, copy these declarations there too.
 */
void add_pi_to_vdm_gw(Interface * i, int idx)
{
    if (NULL == interface)
        return;

    char *signature = make_string("    public PI_%s: ",
                                  i->name);

    fprintf(interface, "%s", signature);

    if (NULL != user_code) {
        fprintf(user_code, "%s", signature);
    }

    char *sig_c = make_string("void %s_PI_%s(",
                              i->parent_fv->name,
                              i->name);
    size_t sig_c_len = strlen(sig_c);
    char *sep_c = make_string(",\n%*s", sig_c_len, "");
    fprintf(c_bridge, "%s", sig_c);

    char *sep = " * ";
    bool comma = false;
    char *params = " (";
    char *sep2 = ", ";

    FOREACH (p, Parameter, i->in, {
        char *sort = make_string("%s%s`%s",
                                 comma? sep: "",
                                 p->asn1_module,
                                 p->type);
        params = make_string("%s%s%s",
                             params,
                             comma? sep2: "",
                             p->name);
        fprintf(interface, "%s", sort);
        if(NULL != user_code) {
            fprintf(user_code, "%s",sort);
        }
        free(sort);

        fprintf(c_bridge,
                "%sconst asn1Scc%s *IN_%s",
                comma? sep_c: "",
                p->type,
                p->name);

        comma = true;
    });

    params = make_string("%s)", params);

    if (NULL != i->out) {
        char *out = make_string(" ==> %s`%s",
                                i->out->value->asn1_module,
                                i->out->value->type);
        fprintf(interface, "%s", out);
        if(NULL != user_code) {
            fprintf(user_code, "%s", out);
        }
        free(out);

        fprintf(c_bridge,
                "%sasn1Scc%s *OUT_%s",
                comma? sep_c: "",
                i->out->value->type,
                i->out->value->name);
    }
    else {
        fprintf(interface, " ==> ()");
        if(NULL != user_code) {
            fprintf(user_code, " ==> ()");
        }
    }

    fprintf(interface,
            "\n"
            "    %s%s == is subclass responsibility;\n\n",
            i->name,
            NULL != i->in? " (-)" : "()");

    if (NULL != user_code)

        fprintf(user_code,
                "\n"
                "    %s%s == -- Write your code here\n\n",
                i->name,
                NULL != i->in? params: "");

    /* Fill in the C bridge */
    fprintf(c_bridge,
            ")\n"
            "{\n");

    FOREACH(param, Parameter, i->in, {
        fprintf(c_bridge,
                "    TVP ptr_%s = NULL;\n"
                "    Convert_%s_from_ASN1SCC_to_VDM(&ptr_%s, IN_%s);\n",
                param->name,
                param->type,
                param->name,
                param->name);
    });

    if (i->out) {
        fprintf(c_bridge,
                "\n    TVP vdm_OUT_%s;\n"
                "    vdm_OUT_%s = ",
                i->out->value->name,
                i->out->value->name);
    }
    else {
        fprintf(c_bridge,
                "\n    ");
    }

    fprintf(c_bridge,
            "CALL_FUNC(%s, %s, %s, %d",
            i->parent_fv->name,
            i->parent_fv->name,
            i->parent_fv->name,
            idx);

    FOREACH(param, Parameter, i->in, {
        fprintf(c_bridge,
                ", ptr_%s",
                param->name);
    });
    fprintf(c_bridge, ");\n");

    if (i->out) {
        fprintf(c_bridge,
                "\n    Convert_%s_from_VDM_to_ASN1SCC(OUT_%s, &vdm_OUT_%s);\n",
                i->out->value->type,
                i->out->value->name,
                i->out->value->name);
    }

    fprintf(c_bridge,
            "}\n\n");

}

/* Declaration of the RI */
void add_ri_to_vdm_gw(Interface * i)
{
    if (NULL == interface || NULL == i)
        return;
    // TODO
}


void End_VDM_GW_Backend(FV *fv)
{
    fprintf(interface, "end %s_Interface\n", fv->name);
    close_file(&interface);
    if (NULL != user_code) {
        fprintf(user_code, "end %s\n", fv->name);
        close_file(&user_code);
    }
    close_file(&c_bridge);
}

/* Function to process one interface of the FV */
void GW_VDM_Interface(Interface * i, int idx)
{
    switch (i->direction) {
        case PI:
            add_pi_to_vdm_gw(i, idx);
            break;

        case RI:
            /*
             * There can be duplicate RI name but one sync, the other async
             * In that case, discard the async one (generated by VT to allow
             * a mix of sync and async PI in one block
             */
            if (asynch == i->synchronism) {
                FOREACH(ri, Interface, i->parent_fv->interfaces, {
                    if (RI == ri->direction &&
                        !strcmp(ri->name, i->name) &&
                        synch == ri->synchronism) {
                        return;
                    }

                });
            }
            // RI not supported yet
            //add_ri_to_vdm_gw(i);
            break;
        default:
            break;
    }
}

/* External interface (the one and unique) */
void GW_VDM_Backend(FV * fv)
{
    int count = 1;
    if (fv->system_ast->context->onlycv)
        return;
    if (vdm == fv->language) {
        Init_VDM_GW_Backend(fv);
        FOREACH(i, Interface, fv->interfaces, {
            GW_VDM_Interface(i, count);
            if (PI == i->direction) {
                count ++;
            }
        });
        End_VDM_GW_Backend(fv);
    }
}
