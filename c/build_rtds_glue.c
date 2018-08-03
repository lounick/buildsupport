/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* 
 * build_rtds_glue.c
 * this program generates the glue to link Pragmadev'RTDS projects with ASSERT
 *
 * first version: 23/06/2009
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>

#include "my_types.h"
#include "practical_functions.h"

static FILE *glue_c = NULL, *glue_h = NULL, *glue_macros = NULL, *sync_ri =
    NULL;

#define SIMPLETYPE(p) (integer==p->basic_type || boolean==p->basic_type || enumerated==p->basic_type || real==p->basic_type || string == p->basic_type)

/* Adds header to user_code.h and (if new) user_code.c */
void rtds_glue_preamble(FV * fv)
{
    int hasparam = 0;

    /* Check if any interface needs ASN.1 types */
    FOREACH(i, Interface, fv->interfaces, {
            if (NULL != i->in || NULL != i->out) hasparam = 1;}
    );

    /* a. glue_.h preamble */
    fprintf(glue_h,
            "/* This file was generated automatically: DO NOT MODIFY IT ! */\n\n");
    fprintf(glue_h,
            "/* Declaration of the provided and required interfaces */\n\n");
    fprintf(glue_h, "#ifndef __GLUE_H_%s__\n#define __GLUE_H_%s__\n\n",
            fv->name, fv->name);
    if (hasparam) {
        fprintf(glue_h, "#include \"C_ASN1_Types.h\"\n\n");
        fprintf(glue_h, "#include \"RTDS_ASN1_Types.h\"\n\n");
    }

    /* fprintf(glue_h, "#include \"RTDS_OS.h\"\n"); */
    fprintf(glue_h, "#include \"RTDS_gen.h\"\n");
    fprintf(glue_h, "#include \"RTDS_Proc.h\"\n");
    fprintf(glue_h, "#include \"RTDS_%s_project_messages.h\"\n",
            fv->name);
    fprintf(glue_h, "#include \"%s_p_decl.h\"\n\n", fv->name);

    /* b. glue_.c preamble */

    fprintf(glue_c,
            "/* This file was generated automatically: DO NOT MODIFY IT ! */\n\n");
    fprintf(glue_c, "#include \"glue_%s.h\"\n\n", fv->name);

    fprintf(glue_c, "static RTDS_Proc %s_instanceDescriptor;\n", fv->name);     /* Declare the SDL instance descriptor */
    fprintf(glue_c, "static RTDS_GlobalProcessInfo %s_instanceContext;\n", fv->name);   /* Declare the SDL instance context */
    fprintf(glue_c, "static RTDS_MessageHeader currentMessage;\n\n");   /* Declare the RTDS message header */

    fprintf(glue_c,
            "RTDS_Proc* %s_instanceDescriptor_ptr = NULL;\n",
            fv->name);

    fprintf(glue_c,
            "char* %s_instanceDescriptor_locals_ptr = NULL;\n",
            fv->name);

    fprintf(glue_c, "void %s_startup()\n{\n", fv->name);
    fprintf(glue_h, "void %s_startup();\n\n", fv->name);
    fprintf(glue_c,
            "\tRTDS_Proc_%s_p_createInstance(&%s_instanceDescriptor, NULL, &%s_instanceContext, NULL);\n",
            fv->name, fv->name, fv->name);

    fprintf(glue_c,
            "\t%s_instanceDescriptor_ptr = &%s_instanceDescriptor;\n",
            fv->name, fv->name);

    fprintf(glue_c,
            "\t%s_instanceDescriptor_locals_ptr = %s_instanceDescriptor.myLocals.%s_p.RTDS_myLocals;\n",
            fv->name, fv->name, fv->name);

    fprintf(glue_c,
            "\tRTDS_%s_p_executeTransition(&%s_instanceDescriptor, NULL);\n}\n\n",
            fv->name, fv->name);

    /* c. glue macros preamble */
    fprintf(glue_macros,
            "/* This file was generated automatically: DO NOT MODIFY IT ! */\n");
    fprintf(glue_macros,
            "/* Definition of the RI macros called by RTDS generated code */\n\n");
    fprintf(glue_macros, "#include \"glue_%s.h\"\n\n", fv->name);

    /* d. sync_RI preamble */
    fprintf(sync_ri,
            "/* This file was generated automatically: DO NOT MODIFY IT ! */\n\n");
    fprintf(sync_ri,
            "/* Definition of the synchronous required interfaces */\n\n");
    fprintf(sync_ri, "#include \"common.h\"\n\n");

}

/* Creates glue_<function>.h/c, common.h, and <function>_syncRI.c  */
int Init_RTDS_GLUE_Backend(FV * fv)
{
    char *path = NULL;
    char *filename = NULL;
    path = make_string("%s/%s", OUTPUT_PATH, fv->name);

    filename = make_string("glue_%s.c", fv->name);
    create_file(path, filename, &glue_c);

    /* change from .c to .h */
    filename[strlen(filename) - 1] = 'h';
    create_file(path, filename, &glue_h);
    free(filename);

    /*  Create the file used to store the definition of macros
     *  "called" by RTDS to send message to the environment of
     *  the SDL subsystem. It is called "common.h", and is
     *  #included by RTDS-generated code 
     */
    filename = make_string("common.h");
    create_file(path, filename, &glue_macros);
    free(filename);

    /* Create the file used to store the functions called by
     * RTDS to call synchronous RI (e.g. call to SCADE, Simulink).
     */
    filename = make_string("%s_syncRI.c", fv->name);
    create_file(path, filename, &sync_ri);

    free(path);
    free(filename);

    assert(NULL != glue_h && NULL != glue_c && NULL != glue_macros
           && NULL != sync_ri);

    rtds_glue_preamble(fv);

    return 0;
}

/* 
 * ForEachWithParam function : Call the function to convert a variable declared with an  ASN.1 type 
 * to a variable declared with an RTDS equivalent type
 * (the function making the conversion is provided by the B mapper from Semantix)
*/
void Convert_ASN1_to_RTDS(Parameter * p, FILE ** file)
{
    fprintf(*file,
            "\tConvert_%s_from_ASN1SCC_to_RTDS (&rtds_%s_%s, asn1_%s_%s);\n",
            p->type, (param_in == p->param_direction) ? "IN" : "OUT",
            p->name, (param_in == p->param_direction) ? "IN" : "OUT",
            p->name);
}

/* 
 * ForEachWithParam function : Call the function to convert a variable declared with a ASN.1 type 
 * to a variable declared with an ASN.1 equivalent type
 * (the function making the conversion is provided by the B mapper from Semantix)
 * Macro version. Line finished with '\' and Asn.1 name passed by pointer
*/
void Convert_ASN1_to_RTDS_Macro(Parameter * p, FILE ** file)
{
    fprintf(*file,
            "\tConvert_%s_from_ASN1SCC_to_RTDS (rtds_%s_%s_%s, &asn1_%s_%s);\\\n",
            p->type, (param_in == p->param_direction) ? "IN" : "OUT",
            p->interface->name, p->name,
            (param_in == p->param_direction) ? "IN" : "OUT", p->name);
}

/* 
 * ForEachWithParam function : Call the function to convert a variable declared with a RTDS type 
 * to a variable declared with an ASN.1 equivalent type
 * (the function making the conversion is provided by the B mapper from Semantix)
*/
void Convert_RTDS_to_ASN1(Parameter * p, FILE ** file, char *eol_separator)
{
    fprintf(*file,
            "\tConvert_%s_from_RTDS_to_ASN1SCC (&asn1_%s_%s_%s, %srtds_%s_%s);",
            p->type, (param_in == p->param_direction) ? "IN" : "OUT",
            p->interface->name, p->name, (SIMPLETYPE(p)
                                          || synch ==
                                          p->interface->
                                          synchronism) ? "" : "&",
            (param_in == p->param_direction) ? "IN" : "OUT", p->name);
    fprintf(*file, "%s", eol_separator);
}

/*
 * ForEachWithParam function : Write to file the list of parameters, 
 * separated with commas (form "rtds_IN_PARAM1, ..., rtds_OUT_PARAMn")
*/
void List_RTDS_Params(Parameter * p, FILE ** file)
{
    unsigned int comma = 0;

    /* Determine if a comma is needed prior to the item to be put in the list.  */
    comma =
        ((p->param_direction == param_in && p != p->interface->in->value)
         || (p->param_direction == param_out
             && (NULL != p->interface->in
                 || p != p->interface->out->value)));

    fprintf(*file, "%srtds_%s_%s",
            comma ? ", " : "",
            (param_in == p->param_direction) ? "IN" : "OUT", p->name);
}

/*
 * List RTDS types and params (RTDStype [*]IN_param1, ..., *OUT_param_i) 
 * Add pointer to IN params for complex types only (struct...)
*/
void List_RTDS_Types_And_Params(Parameter * p, FILE ** file)
{
    if ((param_in == p->param_direction && p != p->interface->in->value) ||
        (param_out == p->param_direction
         && (NULL != p->interface->in || p != p->interface->out->value))) {
        fprintf(*file, ", ");
    }
    fprintf(*file, "%s %srtds_%s_%s",
            p->type,
            (param_out == p->param_direction || !SIMPLETYPE(p)) ? "*" : "",
            (param_in == p->param_direction) ? "IN" : "OUT", p->name);
}

/* 
 * ForEachWithParam function : Write to file the list of ASN.1 parameters, 
 * separated with commas (form "asn1_IN_PARAM1, ..., rtds_OUT_PARAMn")
*/
void List_ASN1_Params_With_Pointers(Parameter * p, FILE ** file)
{
    unsigned int comma = 0;

    /* Determine if a comma is needed prior to the item to be put in the list.  */
    comma =
        ((p->param_direction == param_in && p != p->interface->in->value)
         || (p->param_direction == param_out
             && (NULL != p->interface->in
                 || p != p->interface->out->value)));

    fprintf(*file, "%s&asn1_%s_%s_%s",
            comma ? ", " : "",
            (param_in == p->param_direction) ? "IN" : "OUT",
            p->interface->name, p->name);
}

/* ForEachWithParam function : Make a declaration of static variable for a given parameter */
void Declare_static_ASN1_param(Parameter * p, FILE ** file)
{
    fprintf(*file, "static asn1Scc%s asn1_%s_%s_%s;\n",
            p->type,
            (param_in == p->param_direction) ? "IN" : "OUT",
            p->interface->name, p->name);
}

/* 
 * Add a Provided interface. Can contain IN parameters and no OUT parameter (asynchronous SDL system)
 * Write in glue_<function>.h and in glue_<function>.c  the declaration of the functions
*/
void add_PI_to_RTDS_glue(Interface * i)
{
    Parameter_list *tmp;

    fprintf(glue_h, "void %s_PI_%s(", i->parent_fv->name, i->name);
    fprintf(glue_c, "void %s_PI_%s(", i->parent_fv->name, i->name);

    tmp = i->in;

    while (NULL != tmp) {
        fprintf(glue_h, "%sconst asn1Scc%s *",
                (tmp != i->in) ? ", " : "", tmp->value->type);

        fprintf(glue_c, "%sconst asn1Scc%s *asn1_IN_%s",
                (tmp != i->in) ? ", " : "",
                tmp->value->type, tmp->value->name);
        tmp = tmp->next;
    }

    fprintf(glue_h, ");\n\n");
    fprintf(glue_c, ")\n{\n");

    fprintf(glue_c,
            "\t/* 1) For each parameter, declare a RTDS variable */\n");
    FOREACH(p, Parameter, i->in, {
            fprintf(glue_c, "\t%s rtds_%s_%s;\n",
                    p->type,
                    (param_in == p->param_direction) ? "IN" : "OUT",
                    p->name);}
    )

        fprintf(glue_c, "\t/* 2) RTDS Macro to declare a message */\n");
    fprintf(glue_c, "\n\tRTDS_MSG_DATA_DECL\n");

    fprintf(glue_c,
            "\n\t/* 3) Convert each input parameter from ASN.1 to RTDS */\n");
    /*ForEachWithParam (i->in, Convert_ASN1_to_RTDS, &glue_c); */
    FOREACH(p, Parameter, i->in, {
            Convert_ASN1_to_RTDS(p, &glue_c);
            }
    );

    fprintf(glue_c, "\n\t/* 4) Set the message header */\n");
    fprintf(glue_c, "\tRTDS_%s_SET_MESSAGE(&currentMessage%s", i->name,
            (NULL != i->in) ? ", " : "");
    /*ForEachWithParam (i->in, List_RTDS_Params, &glue_c); */
    FOREACH(p, Parameter, i->in, {
            List_RTDS_Params(p, &glue_c);
            }
    );
    fprintf(glue_c, ");\n");

    fprintf(glue_c, "\n\t/* 5) Execute the SDL transition */\n");
    fprintf(glue_c,
            "\tRTDS_%s_p_executeTransition (&%s_instanceDescriptor, &currentMessage);\n",
            i->parent_fv->name, i->parent_fv->name);

    fprintf(glue_c, "}\n\n");
}

/* 
 * Declaration of the RI in glue_<function>.h 
 * (can contain several IN/OUT params -in case of local procedure call)
*/
void add_RI_to_RTDS_glue(Interface * i)
{
    Parameter_list *tmp;

    fprintf(glue_h, "extern void %s_RI_%s(", i->parent_fv->name, i->name);

    tmp = i->in;

    while (NULL != tmp) {
        fprintf(glue_h, "%sconst asn1Scc%s *",
                (tmp != i->in) ? ", " : "", tmp->value->type);
        tmp = tmp->next;
    }

    tmp = i->out;

    while (NULL != tmp) {
        fprintf(glue_h, "%sasn1Scc%s *",
                (tmp != i->out
                 || (tmp == i->out
                     && NULL != i->in)) ? ", " : "", tmp->value->type);
        tmp = tmp->next;
    }

    fprintf(glue_h, ");\n\n");
}

/* Add a synchronous RI function (syncRI_<InterfaceName> (in, out)) */
void add_sync_RI(Interface * i)
{
    fprintf(sync_ri, "\n/* SYNCHRONOUS REQUIRED INTERFACE %s */\n",
            i->name);

    /* 
     * function prototype : syncRI_<InterfaceName> (rtdstype [*]rtds_IN_param1,.., rtdstype *rtds_OUT_param1,...) 
     * for input parameters: there is a pointer for "complex" types only
     */
    fprintf(sync_ri, "void syncRI_%s (", i->name);
    FOREACH(p, Parameter, i->in, {
            List_RTDS_Types_And_Params(p, &sync_ri);
            });
    FOREACH(p, Parameter, i->out, {
            List_RTDS_Types_And_Params(p, &sync_ri);
            });
    fprintf(sync_ri, ")\n{\n");

    /* declare static data to put the Asn.1 encoded data */
    FOREACH(p, Parameter, i->in, {
            Declare_static_ASN1_param(p, &sync_ri);
            });
    FOREACH(p, Parameter, i->out, {
            Declare_static_ASN1_param(p, &sync_ri);
            });

    /* convert rtds input to asn.1 static data */
    FOREACH(p, Parameter, i->in, {
            Convert_RTDS_to_ASN1(p, &sync_ri, "\n");
            });

    fprintf(sync_ri, "\n");

    /* call RI function generated in invoke_ri.c by the C backend */
    fprintf(sync_ri, "\t%s_RI_%s(", i->parent_fv->name, i->name);
    FOREACH(p, Parameter, i->in, {
            List_ASN1_Params_With_Pointers(p, &sync_ri);
            });
    FOREACH(p, Parameter, i->out, {
            List_ASN1_Params_With_Pointers(p, &sync_ri);
            });
    fprintf(sync_ri, ");\n");

    /* convert result from asn1 to rtds and end function */
    FOREACH(p, Parameter, i->out, {
            fprintf(sync_ri,
                    "\tConvert_%s_from_ASN1SCC_to_RTDS (rtds_OUT_%s, &asn1_OUT_%s_%s);\n",
                    p->type, p->name, p->interface->name, p->name);});

    fprintf(sync_ri, "}\n");
}

void add_RI_to_Macro_Definitions(Interface * i)
{

    fprintf(glue_macros, "/* REQUIRED INTERFACE %s */\n", i->name);

    /* 1) declare static data to put the Asn.1 encoded data */
    FOREACH(p, Parameter, i->in, {
            Declare_static_ASN1_param(p, &glue_macros);
            });
    FOREACH(p, Parameter, i->out, {
            Declare_static_ASN1_param(p, &glue_macros);
            });

    /* 2) undef the possibly existing macro */
    fprintf(glue_macros, "\n#undef RTDS_MSG_SEND_%s\n", i->name);

    /* 
     * 3) define the macro:
     *      #define RTDS_MSG_SEND_<msg name>(PARAM1, ..., PARAMn)\
     *      { Convert_<type>_from_RTDS_to_ASN1SCC (PARAM1, &PARAM1_in_asn1); \  (for each IN param)
     *      message(PARAM1_in_asn1, ...);\
     *      Convert_<type>_from_ASN1_to_RTDS (&out_param_in_asn1, outPARAMi) \ (if applicable, for each OUT param)
     *      }
     */
    fprintf(glue_macros, "#define RTDS_MSG_SEND_%s%s", i->name,
            (NULL != i->in || NULL != i->out) ? "(" : "");
    FOREACH(p, Parameter, i->in, {
        List_RTDS_Params(p, &glue_macros);
    })
    FOREACH(p, Parameter, i->out, {
        List_RTDS_Params(p, &glue_macros);
    })

    fprintf(glue_macros, "%s\\\n{ ",
            (NULL != i->in || NULL != i->out) ? ")" : "");
    FOREACH(p, Parameter, i->in, {
            Convert_RTDS_to_ASN1(p, &glue_macros, "\\\n");
            });
    fprintf(glue_macros, "\t%s_RI_%s(", i->parent_fv->name, i->name);
    FOREACH(p, Parameter, i->in, {
        List_ASN1_Params_With_Pointers(p, &glue_macros);
    })
    FOREACH(p, Parameter, i->out, {
        List_ASN1_Params_With_Pointers(p, &glue_macros);
    })

    fprintf(glue_macros, ");\\\n");
    FOREACH(p, Parameter, i->out, {
        Convert_ASN1_to_RTDS_Macro(p, &glue_macros);
    })
    fprintf(glue_macros, "}\n\n");
}

/* Function to process one interface of the FV */
void GLUE_RTDS_Interface(Interface * i)
{
    switch (i->direction) {
    case PI:
        add_PI_to_RTDS_glue(i);
        break;

    case RI:
        add_RI_to_RTDS_glue(i);
        if (asynch == i->synchronism)
            add_RI_to_Macro_Definitions(i);
        if (synch == i->synchronism)
            add_sync_RI(i);
        break;
    default:
        break;
    }
}

/* External interface (the one and unique) */
void GLUE_RTDS_Backend(FV * fv)
{

    if (fv->system_ast->context->onlycv)
        return;
    if (rtds == fv->language) {

        Init_RTDS_GLUE_Backend(fv);
        FOREACH(i, Interface, fv->interfaces, {
            GLUE_RTDS_Interface(i);
        })

        fprintf(glue_h, "\n#endif\n");
        close_file(&glue_h);
        close_file(&glue_c);
        close_file(&glue_macros);
        close_file(&sync_ri);
    }
}
