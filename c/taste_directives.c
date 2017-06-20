/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*
 *  Generate ASN.1 modules containing the declaration of 
 *  TASTE directives, based on the "functional state" variables 
 *  declared in the interface view, then call asn1.exe to
 *  compile it, create a C source file, compile it, and run it
 *  to get an XML version of the directives.
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

void Process_Directives(FV *fv)
{
        ASN1_Module_list *module_list   = NULL;
        ASN1_Type_list   *type_list     = NULL;
        char    *command                = NULL;
        FILE    *asn                    = NULL;
        FILE    *main_c                 = NULL;
        char    *fv_no_underscore       = NULL;
        char    *cp_no_underscore       = NULL;
        bool    directive_present       = false;

        ASN1_Filename *filename = NULL;

        if (NULL == fv->context_parameters) return;

        /*
         * Build lists (sets) of ASN.1 modules, and types
        */
        FOREACH (cp, Context_Parameter, fv->context_parameters, {
                if (!strcmp (cp->type.name, "Taste-directive")) {
                    directive_present = true;
                }
                ADD_TO_SET (ASN1_Module, module_list, cp->type.module);
                ADD_TO_SET (ASN1_Type, type_list, &(cp->type));
        });

        if (false == directive_present) return;

        /*
         * Make sure the FV name has no underscore (to be ASN.1-friendly)
         */
        fv_no_underscore = underscore_to_dash (fv->name, strlen(fv->name));

        /*
         * Create the ASN.1 file to store the context parameters 
         */
        filename = make_string ("Directive-%s.asn", fv_no_underscore);
        asn = fopen (filename, "wt");
        assert (NULL != asn);

        fprintf(asn, "Directive-%s DEFINITIONS ::=\n", fv_no_underscore);
        fprintf(asn, "BEGIN\n");

        /*
         * Import the ASN.1 modules containing the directive type definition
         */
        fprintf (asn, "IMPORTS Taste-directive FROM TASTE-Directives;\n");

        /*
         * Create a type that wraps all directives (to form a namespace)
         */
        fprintf(asn, "\nDirective-%s ::= SEQUENCE {", fv_no_underscore);
        bool comma = false;
        FOREACH (cp, Context_Parameter, fv->context_parameters, {
                if (!strcmp (cp->type.name, "Taste-directive")) {
                    cp_no_underscore = underscore_to_dash
                                                 (cp->name, strlen(cp->name));
                    fprintf(asn, "%s\t%s Taste-directive", 
                        true == comma ? ",\n": "\n",
                        cp_no_underscore);
                    free (cp_no_underscore);
                    comma = true;
                }
        })
        fprintf(asn,"\n}\n");

        /*
         * And finally, declare a variable of the wrapped directive type.
         */
        comma = false;
        fprintf (asn, "\n%s-directive Directive-%s ::= {",
                      fv_no_underscore,
                      fv_no_underscore);
        FOREACH (cp, Context_Parameter, fv->context_parameters, {
            if (!strcmp (cp->type.name, "Taste-directive")) {
                cp_no_underscore = underscore_to_dash
                                                 (cp->name, strlen(cp->name));
                fprintf(asn, "%s\t%s %s", 
                    true == comma ? ",\n" : "\n",
                    cp_no_underscore, 
                    cp->value);
                free (cp_no_underscore);
                comma = true;
            }
        })
        fprintf (asn, "\n}\n");

        fprintf (asn, "END\n");
        fclose (asn);

        /*
         * Next part: call asn1.exe with the newly-created ASN.1 file 
        */

        char *directives_file = make_string ("$(taste-config --directives)");

        char *directive_path = make_string
                               ("%s/%s/directives", OUTPUT_PATH, fv->name);

        mkdir (directive_path, 0700);
        free (directive_path);

        command = make_string ("mono $(which asn1.exe) -c -XER -o %s/%s/directives %s %s",
                OUTPUT_PATH,
                fv->name,
                filename,
                directives_file);

        ERROR ("[INFO] Executing %s\n", command);

        if (system (command)) {
            ERROR ("The command failed. Try it yourself"
                   " (correct paths, access to files, etc.)\n");
            exit (-1);
        }
        else if (!get_context()->test) {
            unlink (filename);  
        }

        /* Create a C application that will encode the directives using XER */
        char *cpath = make_string ("%s/%s/directives", OUTPUT_PATH, fv->name);
        create_file(cpath, "main.c" , &main_c);
        assert (NULL != main_c);
        free (cpath);

        fprintf (main_c,
                "#include <stdlib.h>\n"
                "#include <string.h>\n"
                "#include <stdio.h>\n"
                "#include <assert.h>\n"
                "#include \"Directive-%s.h\"\n\n"
                "#define XML_BUF_SIZE 65535\n\n"
                "main()\n"
                "{\n"
                "    byte buffer[XML_BUF_SIZE]={0};\n"
                "    ByteStream b;\n"
                "    int errcode = 0;\n\tFILE *out = NULL;\n\n"
                "    ByteStream_Init (&b, buffer, XML_BUF_SIZE);\n"
                "    Xer_EncodeXmlHeader (&b, NULL);\n\n"
                "    assert (TRUE == Directive_%s_XER_Encode"
                "(&%s_directive, &b, &errcode, TRUE));\n\n"
                "    out = fopen (\"directives.xml\", \"w\");\n"
                "    assert (NULL != out);\n\n"
                "    fwrite"
                " (buffer, 1, (size_t)ByteStream_GetLength(&b), out);\n"
                "    fclose (out);\n"
                "}\n",
                fv_no_underscore,
                fv->name,
                fv->name);

        close_file (&main_c);

        /* Compile and execute the main.c file */
        free (command);
        command = NULL;

        command = make_string ("curr=$(pwd) && cd %s/%s/directives \
                 && gcc *.c -o generate_xml -g && ./generate_xml && cd $(pwd)",
                 OUTPUT_PATH, fv->name);
        assert (0 == system (command));

        free (filename);
        free (command);
        free (fv_no_underscore);
}
