#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "my_types.h"
#include "practical_functions.h"

/* Constant defining the maximen number of character in a line of a source file */
#define C_MAX_LINE_SIZE    256

/* Constant defining the maximum number of characters of a file name */
#define C_MAX_FILE_NAME_SIZE    80

/*
 * Internal variable definition
 */

/* Linked lists on the RI and PI for the current FV*/ 
static T_RI_PI_NAME_LIST *RI_LIST = NULL;
static T_RI_PI_NAME_LIST *PI_LIST = NULL;

/* Variable defining the number of RI for a given node */
static int RI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE = 0;

/* Variable defining the number of RI for a given node */
static int PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE = 0;


/* File structure pointers */
static FILE *header_id = NULL;
static FILE *enums_header_id = NULL;
static FILE *code_id = NULL;

/* 
 * Name of the cyclic function that polls the shared memory
 * that will be build by this backend 
 */
static char *cyclic_name = NULL;

void ros_bridge_preamble(FV *fv)
{
    if ((NULL == header_id) || (NULL == code_id) || (NULL == enums_header_id))
    {
        return;
    }

    RI_LIST = NULL;
    PI_LIST = NULL;

    // headers preamble
    fprintf(enums_header_id,
        "/* This file was generated automatically: DO NOT MODIFY IT ! */\n\n");
    fprintf(enums_header_id, "#ifndef _%s_ENUM_DEF_H\n", fv->name);
    fprintf(enums_header_id, "#define _%s_ENUM_DEF_H\n\n\n", fv->name);

     fprintf(header_id,
        "/* This file was generated automatically: DO NOT MODIFY IT ! */\n\n");
    fprintf(header_id, "#ifndef _%s_GUI_HEADER_H\n", fv->name);
    fprintf(header_id, "#define _%s_GUI_HEADER_H\n\n\n", fv->name);
    fprintf(header_id,
        "#include <stdlib.h>\n#include <stdio.h>\n\n#include \"C_ASN1_Types.h\"\n\n");
    fprintf(header_id, "#include \"%s_enums_def.h\"\n\n", fv->name);
    fprintf(header_id, "#include <ros/ros.h>\n\n\n");

    //Create initialization function prototype
    fprintf(header_id, "void %s_startup();\n\n", fv->name);

    // code preamble
    fprintf(code_id, "#include <unistd.h>\n");
    fprintf(code_id, "#include \"%s_ros_bridge_header.h\"\n\n\n", fv->name);
}

int Init_ROS_Bridge_Backend(FV *fv)
{
    char *header_name = NULL;
    char *code_name = NULL;
    char *enum_header_name = NULL;
    char *path = NULL;
    
    if (NULL != fv->system_ast->context->output)
    {
        build_string(&path, fv->system_ast->context->output,
                strlen(fv->system_ast->context->output));
    }

    build_string(&path, fv->name, strlen(fv->name));

    header_id = NULL;
    build_string(&header_name, fv->name, strlen(fv->name));
    build_string(&header_name, "_ros_bridge_header.h",
            strlen("_ros_bridge_header.h"));
    create_file(path, header_name, &header_id);

    code_id = NULL;
    build_string(&code_name, fv->name, strlen(fv->name));
    build_string(&code_name, "_ros_bridge_code.c",
            strlen("_ros_bridge_code.c"));
    create_file(path, code_name, &code_id);

    enums_header_id = NULL;
    build_string(&enum_header_name, fv->name, strlen(fv->name));
    build_string(&enum_header_name, "_enums_def.h",
            strlen("_enums_def.h"));
    create_file(path, enum_header_name, &enums_header_id);

    ros_bridge_preamble(fv);

    free(path);
    free(enum_header_name);
    free(code_name);
    free(header_name);

    return 0;
}

void End_ROS_Bridge_Glue_Backend(FV *fv)
{
}

void add_PI_to_ros_bridge_glue(Interface *i)
{
    if (((cyclic_name != NULL) && (strcmp(i->name, cyclic_name) != 0))
            || cyclic_name == NULL)
    {

    }
}

void add_RI_to_ros_bridge_glue(Interface *i)
{
}

/*
 * Function to process one interface of the FV
 */
void GLUE_ROS_Bridge_Interface(Interface * i)
{
    switch (i->direction) {
        case PI: 
            add_PI_to_ros_bridge_glue(i);
            break;
        case RI:
            add_RI_to_ros_bridge_glue(i);
            break;
        default:
            break;
    }
}

/*
 * External interface (the one and unique)
 */
void GLUE_ROS_Bridge_Backend(FV * fv)
{
    if (fv->system_ast->context->onlycv)
        return;

    if (ros_bridge == fv->language) {
        create_enum_file(fv);

        build_string(&cyclic_name, "ros_bridge_polling_", strlen("ros_bridge_polling_"));
        build_string(&cyclic_name, fv->name, strlen(fv->name));

        /* count the number of PI and RIs for this FV */
        PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE = 0;
        RI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE = 0;
        FOREACH(i, Interface, fv->interfaces, {
            if (PI == i->direction) PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE ++;
            else RI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE ++;
        })

        Init_ROS_Bridge_Backend(fv);
        FOREACH(i, Interface, fv->interfaces, {
            GLUE_ROS_Bridge_Interface(i);
        })

        End_ROS_Bridge_Glue_Backend(fv);

        if(NULL != cyclic_name) {
            free(cyclic_name);
            cyclic_name = NULL;
        }
    }
}
