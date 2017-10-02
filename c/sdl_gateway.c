/* Buildsupport is (c) 2008-2017 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*

ObjectGEODE 4.2 skeleton-generation
Update August 2012 : support for OpenGEODE 1.0
*/  
    
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "my_types.h"
#include "practical_functions.h"
#include "backends.h"

static FILE *f;
static char *pi_string = NULL;  // List of PI (SDL INPUT SIGNALs) used to connect ROUTEs and CHANNELs
static char *ri_string = NULL;  // List or RI (SDL OUTPUT SIGNALs) used to connect ROUTEs and CHANNELs

void Create_New_SDL_Structure(FV * fv) 
{
    FILE *process;
    char *path = NULL;

    if (NULL != fv->system_ast->context->output)
    build_string(&path, fv->system_ast->context->output,
              strlen(fv->system_ast->context->output));
    build_string(&path, fv->name, strlen(fv->name));

    /* OpenGEODE Skeletons require iv.py and DataView.py to be present in the working directory */
    Generate_Python_AST(get_system_ast(), path);
        char *dataview_uniq = getASN1DataView();
        char *dataview_path = getDataViewPath();
    if (!file_exists (dataview_path, dataview_uniq)) {
            ERROR ("[INFO] %s/%s not found. Checking for dataview-uniq.asn\n", dataview_path, dataview_uniq);
            free (dataview_uniq);
            dataview_uniq = make_string ("dataview-uniq.asn");
    }

    char *command = make_string("mono $(which asn1.exe) -customStg $(taste-config --prefix)/share/asn1scc/python.stg:%s/DataView.py -customStgAstVerion 4 %s/%s", path, dataview_path, dataview_uniq);
    if (system(command)) {
        ERROR ("[ERROR] Command \"%s\" failed in generation of SDL skeleton\n", command);
    }
    free(command);
    command = make_string("cp \"%s/%s\" %s/", dataview_path, dataview_uniq, path);
    if (system(command)) {
        ERROR ("[ERROR] Command \"%s\" failed in generation of SDL skeleton\n", command);
    }
    free(command);

    /* Initialize variables used to build the list of SDL signals */ 
    if (NULL != pi_string)
        free(pi_string);
    if (NULL != ri_string)
        free(ri_string);

    ri_string = NULL;
    pi_string = NULL;
    create_file(path, "system_structure.pr", &f);

    if (NULL != f) {
        fprintf(f, "/* CIF Keep Specific Geode ASNFilename '%s' */\n",
                    dataview_uniq);
        fprintf(f, "USE Datamodel;\nSYSTEM %s;\n\n", fv->name);
    }

    char *filename = make_string("%s.pr", fv->name);
    /* If not already existing, also create an empty system_implementation.pr
     * file - includes timer declarations, if any. 
     * Do not create in case the function is an instance of a generic function*/
    if (!file_exists (path, filename) && (NULL == fv->instance_of)) {
        create_file (path, filename, &process);
        if (fv->is_component_type == false) {
            fprintf (process, "PROCESS %s;\n", fv->name);
        } else {
            fprintf (process, "PROCESS type %s;\n", fv->name);
        }

        if (NULL != fv->timer_list) {
            fprintf (process, "/* CIF TEXT (10, 10), (200, 250) */\n");
            fprintf (process, "-- Timers defined in the interface view\n"
                              "-- Use SET_TIMER (value, timer name)\n"
                              "-- and RESET_TIMER (timer name) in a\n"
                              "-- PROCEDURE CALL symbol to use them\n\n");
            FOREACH (timer, String, fv->timer_list, {
                    fprintf (process, "TIMER %s;\n", timer);
            });
            fprintf (process, "/* CIF ENDTEXT */\n");
        }
        fprintf (process, "/* CIF START (320, 10), (70, 35) */\n"
                          "START;\n"
                          "NEXTSTATE Wait;\n"
                          "/* CIF STATE (450, 10), (70, 35) */\n"
                          "STATE Wait;\n"
                          "ENDSTATE;\n");
        if (fv->is_component_type == false) {
            fprintf (process, "ENDPROCESS;\n");
        } else {
            fprintf (process, "ENDPROCESS type;\n");
        }
        close_file(&process);
    }
    free(dataview_uniq);
    free(dataview_path);
    free(path);
    free(filename);
}

void Add_SDL_Signal(Interface * i) 
{
    Parameter_list * p = NULL;
    if (NULL == f) {
        return;
    }
    if (NULL != i->in) {
        /* Indicate parameter names as CIF comment (not supported by standard SDL) */
        fprintf(f, "\t/* CIF Keep Specific Geode PARAMNAMES");
        FOREACH(param, Parameter, i->in, {
                fprintf(f, " %s", param->name);
        });
        fprintf(f, " */\n");
    }
    fprintf(f, "\tSIGNAL %s", i->name);

    if (NULL == i->in) {
        fprintf(f, ";\n\n");
    }

    else {
        fprintf(f, " (%s", i->in->value->type);
        p = i->in;
        while (NULL != p->next) {
            p = p->next;
            fprintf(f, ", %s", p->value->type);
        }
        fprintf(f, ");\n\n");
    }
}


/*
 * Create a string with the list of PI for the current functional view
 * This is later used to build the SDL connections (CHANNELs and ROUTEs)
*/
void add_PI_to_string(char *if_name) 
{
    pi_string = build_comma_string(&pi_string, if_name, strlen(if_name));
} 

/* 
 * Create a string with the list of RI for the current functional view
 * This is later used to build the SDL connections (CHANNELs and ROUTEs)
*/ 
void add_RI_to_string(char *if_name) 
{
    ri_string = build_comma_string(&ri_string, if_name, strlen(if_name));
} 

void Add_Sync_Declaration(char *name, Parameter_list * in,
                   Parameter_list * out) 
{
    if (NULL == f)
    return;
    fprintf(f, "\tPROCEDURE %s COMMENT '#c_predef';\n", name);
    if (NULL != in || NULL != out) {
    fprintf(f, "\t\tFPAR\n");
    if (NULL != in) {
        fprintf(f, "\t\t\tIN %s %s", in->value->name,
             in->value->type);
        while (NULL != in->next) {
        in = in->next;
        fprintf(f, ",\n\t\t\tIN %s %s", in->value->name,
             in->value->type);
        }
        if (NULL != out)
        fprintf(f, ",\n");
    }
    if (NULL != out) {
        fprintf(f, "\t\t\tIN/OUT %s %s", out->value->name,
              out->value->type);
        while (NULL != out->next) {
        out = out->next;
        fprintf(f, ",\n\t\t\tIN/OUT %s %s", out->value->name,
             out->value->type);
        }
    }
    fprintf(f, ";\n");
    }
    fprintf(f, " \tEXTERNAL;\n\n");
}

void Build_SDL_Connections(FV * fv) 
{
    if (NULL == f)
    return;
    fprintf(f, "\tCHANNEL c\n");
    if (NULL != pi_string)
    fprintf(f, "\t\tFROM ENV TO %s WITH %s;\n", fv->name, pi_string);
    if (NULL != ri_string)
    fprintf(f, "\t\tFROM %s TO ENV WITH %s;\n", fv->name, ri_string);
    fprintf(f, "\tENDCHANNEL;\n\n");
    fprintf(f, "\tBLOCK %s;\n\n\t\tSIGNALROUTE r\n", fv->name);
    if (NULL != pi_string)
    fprintf(f, "\t\t\tFROM ENV TO %s WITH %s;\n", fv->name,
         pi_string);
    if (NULL != ri_string)
    fprintf(f, "\t\t\tFROM %s TO ENV WITH %s;\n", fv->name,
         ri_string);
    fprintf(f, "\n\t\tCONNECT c and r;\n\n");

    if (fv->is_component_type == false) {
        if (fv->instance_of == NULL) {
            fprintf(f, "\t\tPROCESS %s REFERENCED;\n\n", fv->name);
        } else {
            fprintf(f, "\t\tPROCESS %s:%s;\n\n", fv->name, fv->instance_of);
        }
    } else {
        fprintf(f, "\t\tPROCESS type %s;\n\n", fv->name);
    }

    fprintf(f, "\tENDBLOCK;\n\nENDSYSTEM;");
}

void Close_SDL_Structure() 
{
    close_file(&f);
} 

/* Function to process one interface of the FV */ 
void GW_SDL_Interface(Interface * i) 
{
    switch (i->synchronism) {
        case asynch:
            Add_SDL_Signal(i);
            switch (i->direction) {
                case PI:
                    add_PI_to_string(i->name);
                    break;
                case RI:
                    add_RI_to_string(i->name);
                    break;
                default:
                    break;
            }
            break;
        case synch:
            Add_Sync_Declaration(i->name, i->in, i->out);
            break;
        default:
        break;
    }
}


/* External interface */
void GW_SDL_Backend(FV * fv) 
{
    if (fv->system_ast->context->onlycv) return;
    if ((sdl == fv->language || opengeode == fv->language)) {
       printf("FV %s is in SDL\n", fv->name);
       if (NULL == fv->instance_of) {
           printf("Its instance_of is NULL\n");
       }
       else {
           printf("It IS AN INSTANCE OF %s\n", fv->instance_of);
       }
    }
    if ((sdl == fv->language || opengeode == fv->language) && (NULL == fv->instance_of)) {
        Create_New_SDL_Structure(fv);
        FOREACH(i, Interface, fv->interfaces, {
            GW_SDL_Interface(i);
        })
        Build_SDL_Connections(fv);
        Close_SDL_Structure();
    }
}


