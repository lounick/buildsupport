/* Buildsupport is (c) 2008-2016 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* build_c_skeletons.c

  this program generates empty C functions respecting the interfaces defined
  in the interface view. it provides functions to invoke RI.

  updated 10/06/2016: properly align params and use FOREACH
  updated 20/04/2009 to disable in case "-onlycv" flag is set
  updated 8/10/2009 to rename the user_code.h/c to FV_name.h/c like in the Ada backend
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <assert.h>

#include "my_types.h"
#include "practical_functions.h"

static FILE *user_code_c = NULL, *user_code_h = NULL;


/* Adds header to user_code.h and (if new) user_code.c */
void c_gw_preamble(FV * fv)
{
    int hasparam = 0;
    assert (NULL != user_code_h);

    /* Check if any interface needs ASN.1 types */
    FOREACH(i, Interface, fv->interfaces, {
            CheckForAsn1Params(i, &hasparam);}
    );

    /* a. user_code.h preamble */
    fprintf(user_code_h,
            "/* This file was generated automatically: DO NOT MODIFY IT ! */\n\n");
    fprintf(user_code_h,
            "/* Declaration of the functions that have to be provided by the user */\n\n");
    fprintf(user_code_h,
            "#ifndef __USER_CODE_H_%s__\n#define __USER_CODE_H_%s__\n\n",
            fv->name, fv->name);

    if (hasparam) {
        fprintf(user_code_h, "#include \"C_ASN1_Types.h\"\n\n");
    }

    fprintf(user_code_h,
            "#ifdef __cplusplus\n"
            "extern \"C\" {\n" 
            "#endif\n\n");


    fprintf(user_code_h, "void %s_startup();\n\n", fv->name);

    /* b. user_code.c preamble (if applicable) */
    if (NULL != user_code_c) {
        fprintf(user_code_c,
                "/* User code: This file will not be overwritten by TASTE. */\n\n");
        fprintf(user_code_c, "#include \"%s.h\"\n\n", fv->name);


        if (has_context_param(fv)) {
            char *fv_no_underscore =
                underscore_to_dash(fv->name, strlen(fv->name));
            fprintf(user_code_c,
                    "/* Function static data is declared in this file : */\n");
            fprintf(user_code_c, "#include \"Context-%s.h\"\n\n",
                    fv_no_underscore);
            free(fv_no_underscore);
        }

        fprintf(user_code_c, "void %s_startup()\n{\n", fv->name);

        fprintf(user_code_c,
                "    /* Write your initialization code here,\n"
                "       but do not make any call to a required interface."
                " */\n}\n\n");
    }
}

/* Creates user_code.h, and if necessary user_code.c (if it did not exist) */
int Init_C_GW_Backend(FV * fv, bool generateC)
{
    char *path = NULL;
    char *filename = NULL;

    if (NULL != fv->system_ast->context->output) {
        build_string(&path, fv->system_ast->context->output,
                     strlen(fv->system_ast->context->output));
    }
    build_string(&path, fv->name, strlen(fv->name));
    build_string(&filename, fv->name, strlen(fv->name));
    build_string(&filename, ".h", strlen(".h"));

    create_file(path, filename, &user_code_h);
    filename[strlen(filename) - 1] = 'c';       /* change from .h to .c */
    if (cpp == fv->language) {
        build_string(&filename, "c", 1);       /* change from .c to .cc */
    }

    if (!file_exists(path, filename) && true == generateC)
        create_file(path, filename, &user_code_c);

    free(path);
    if (NULL == user_code_h)
        return -1;

    c_gw_preamble(fv);

    return 0;
}


void close_c_gw_files()
{
    if (NULL != user_code_h) {
        fprintf(user_code_h,
            "#ifdef __cplusplus\n"
            "}\n" 
            "#endif\n\n");
        fprintf(user_code_h, "\n#endif\n");
        close_file(&user_code_h);
    }
    close_file(&user_code_c);
}

/* Add a Provided interface. Can contain in and out parameters
 * Write in user_code.h the declaration of the user functions
 * and if user_code.c is new, copy these declarations there too.
 */
void add_PI_to_C_gw(Interface * i)
{
    if (NULL == user_code_h)
        return;

    char *signature = make_string("void %s_PI_%s(",
                                  i->parent_fv->name,
                                  i->name);

    size_t sig_len = strlen(signature);
    char *sep = make_string(",\n%*s", sig_len, "");
    bool comma = false;

    fprintf(user_code_h, "%s", signature);

    if (NULL != user_code_c) {
        fprintf(user_code_c, "%s", signature);
    }

    FOREACH (p, Parameter, i->in, {
        char *sort = make_string("%sconst asn1Scc%s *",
                                 comma? sep: "",
                                 p->type);
        fprintf(user_code_h, "%s", sort);
        if(NULL != user_code_c) {
            fprintf(user_code_c, "%sIN_%s",
                                 sort,
                                 p->name);
        }
        free(sort);
        comma = true;
    });

    FOREACH (p, Parameter, i->out, {
        char *sort = make_string("%sasn1Scc%s *",
                                 comma? sep: "",
                                 p->type);
        fprintf(user_code_h, "%s", sort);
        if(NULL != user_code_c) {
            fprintf(user_code_c, "%sOUT_%s",
                                 sort,
                                 p->name);
        }
        free(sort);
        comma = true;
    });

    fprintf(user_code_h, ");\n\n");
    if (NULL != user_code_c)
        fprintf(user_code_c, ")\n{\n    /* Write your code here! */\n}\n\n");

    free(signature);
    free(sep);
}

/* Declaration of the RI in user_code.h */
void add_RI_to_C_gw(Interface * i)
{
    if (NULL == user_code_h)
        return;

    char *signature = make_string("extern void %s_RI_%s(",
                                  i->parent_fv->name,
                                  i->name);

    size_t sig_len = strlen(signature);
    char *sep = make_string(",\n%*s", sig_len, "");
    bool comma = false;

    fprintf(user_code_h, "%s", signature);

    FOREACH (p, Parameter, i->in, {
        fprintf(user_code_h, "%sconst asn1Scc%s *",
                             comma? sep: "",
                             p->type);
        comma = true;
    });

    FOREACH (p, Parameter, i->out, {
        fprintf(user_code_h,"%sconst asn1Scc%s *",
                            comma? sep: "",
                            p->type);
        comma = true;
    });

    fprintf(user_code_h, ");\n\n");
    free(signature);
    free(sep);
}

/* Add timer declarations to the C code skeletons */
void C_Add_timers(FV *fv)
{
    if (NULL != fv->timer_list) {
        fprintf (user_code_h, "/* Timers management */\n\n");
    }
    FOREACH(timer, String, fv->timer_list, {
        fprintf(user_code_h,
            "/* This function is called when the timer expires */\n"
            "void %s_PI_%s();\n\n"
            "/* Call this function to set (enable) the timer\n"
            " * Value is in milliseconds, and must be a multiple of 100 */\n"
            "void %s_RI_SET_%s(asn1SccT_UInt32 *val);\n\n"
            "/* Call this function to reset (disable) the timer */\n"
            "void %s_RI_RESET_%s();\n\n",
            fv->name,
            timer,
            fv->name,
            timer,
            fv->name,
            timer);
        if (NULL != user_code_c) {
            fprintf(user_code_c,
              "/* This function is called when the timer \"%s\" expires */\n"
              "void %s_PI_%s()\n"
              "{\n"
              "    /* Write your code here! */\n"
              "}\n\n",
              timer,
              fv->name,
              timer);
        }
    });
}


void End_C_GW_Backend()
{
    close_c_gw_files();
}

/* Function to process one interface of the FV */
void GW_C_Interface(Interface * i)
{
    switch (i->direction) {
    case PI:
        add_PI_to_C_gw(i);
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
                    !strcmp(interface->name, i->name) &&
                    synch == interface->synchronism) {
                    return;
                }

            });
        }
        add_RI_to_C_gw(i);
        break;
    default:
        break;
    }
}

/* External interface (the one and unique) */
void GW_C_Backend(FV * fv)
{
    if (fv->system_ast->context->onlycv)
        return;
    if ((ada == fv->language || qgenada == fv->language)
            && fv->system_ast->context->glue
            && fv->system_ast->context->polyorb_hi_c) {
        /* Only generate the header file if user code is in Ada,
         * for inclusion in POHIC headers
         * This is done to avoid a warning due to missing function declarations
         */
        Init_C_GW_Backend(fv, false);
        End_C_GW_Backend();
    }
    else if (c == fv->language || cpp == fv->language) {
        Init_C_GW_Backend(fv, true);
        FOREACH(i, Interface, fv->interfaces, {
            GW_C_Interface(i);
        });
        // If any timers, add declarations in code skeleton
        C_Add_timers(fv);

        End_C_GW_Backend();
    }
}
