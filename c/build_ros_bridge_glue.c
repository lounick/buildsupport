/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* build_ros_bridge_glue.c

  this program generates the code to interface ROS with the assert virtual machine
  author: Nikolaos Tsiogkas (niko@intermodalics.eu)
  author: Kevin De Martelaere (kevin@intermodalics.eu)
 */

#define ID "$Id: build_ros_bridge_glue.c 437 2018-07-26 09:19:17Z lounick $"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "my_types.h"
#include "practical_functions.h"

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

/*
 * Internal interfaces
 */


//Adds to an existing string a string of the form : "%s1* %s2,"
//Where :
// %s1 = parameter_type         (eg. asn1Scctype_du_param_1)
// %s2 = parameter_name         (eg. nom_du_param_1)
/*void build_function_parameter_list(char **generated_code,
                   char *parameter_type,
                   char *parameter_name)
{
    if (*generated_code != NULL)
    {
        build_string(generated_code, ",", strlen(","));
    }
    build_string(generated_code, "const asn1Scc", strlen("const asn1Scc"));
    build_string(generated_code, parameter_type, strlen(parameter_type));
    build_string(generated_code, "* ", strlen("* "));
    build_string(generated_code, parameter_name, strlen(parameter_name));
    build_string(generated_code, "_val", strlen("_val"));
}*/

//Adds to an existing string a string of the form : "   %s1.fromASN1(%s1_val);\n"
//Where :
// %s1 = parameter_name        (eg. param1)
void build_assignation_code_from_asn(char **generated_code, char *parameter_name)
{
    build_string(generated_code, "   ", strlen("   "));
    build_string(generated_code, parameter_name, strlen(parameter_name));
    build_string(generated_code, ".fromASN1(", strlen(".fromASN1("));
    build_string(generated_code, parameter_name, strlen(parameter_name));
    build_string(generated_code, "_val);\n", strlen("_val);\n"));
}

int create_ros_enum_type_from_list(char *type_name,
                   T_RI_PI_NAME_LIST * items_list,
                   char **type_definition)
{
    T_RI_PI_NAME_LIST *iterator = NULL;

    build_string(type_definition, "typedef enum {\n",
         strlen("typedef enum {\n"));

    iterator = items_list;
    while (iterator != NULL) {
    build_string(type_definition, "  i_", strlen("  i_"));
    build_string(type_definition, iterator->name,
             strlen(iterator->name));
    build_string(type_definition, ",\n", strlen("_enum,\n"));
    iterator = iterator->next;
    }
    build_string(type_definition, "} ", strlen("} "));
    build_string(type_definition, type_name, strlen(type_name));
    build_string(type_definition, ";\n\n", strlen(";\n\n"));

    return 0;
}

void add_PI_to_ros_bridge_glue(Interface * i)
{
    //Parameter_list *tmp;
    T_RI_PI_NAME_LIST *new_elt;

    // Check if the current PI is the "cyclic" one. If so, discard this PI
    if (((cyclic_name != NULL) && (strcmp(i->name, cyclic_name) != 0))
        || cyclic_name == NULL) 
    {

        //
        // 1. Update the list of PI for this FV
        //
        new_elt = (T_RI_PI_NAME_LIST *) malloc(sizeof(T_RI_PI_NAME_LIST));
        new_elt->name =
            (char *) malloc((strlen(i->name) + 1) * sizeof(char));
        strcpy(new_elt->name, i->name);
        new_elt->next = PI_LIST;
        PI_LIST = new_elt;
        // Provided interface accepts a message
        // There should be one input param.
        
        // Indlude the header of the data type
        fprintf(code_id, "#include \"%s/%c%s.h\"\n",
                i->in->value->asn1_module, toupper(i->in->value->type[0]),
                &i->in->value->type[1]);

        // Declare a global variable to be used as a ROS message
        fprintf(code_id, "%s::%c%s %s;\n", i->in->value->asn1_module,
                toupper(i->in->value->type[0]), &i->in->value->type[1],
                i->in->value->name);

        // Generate the publisher
        fprintf(code_id, "ros::Publisher %sPub(\"%s\", &%s);\n",
                i->name, i->in->value->name, i->in->value->name);

        //Generate the header for the case where parameters
        //associated to the IF
        fprintf(header_id, "void %s_PI_%s(const asn1Scc%s *%s_val);\n\n",
                i->parent_fv->name, i->name, i->in->value->type,
                i->in->value->name);
        
        //Generate the code for the case where parameters are
        //associated to the IF
        fprintf(code_id, "void %s_PI_%s(const asn1Scc%s *%s_val)\n{\n",
                i->parent_fv->name, i->name, i->in->value->type,
                i->in->value->name);
        // Assign the value to the message
        fprintf(code_id, "    %s.fromASN1(%s_val);\n",
                i->in->value->name, i->in->value->name);
        //Call the publisher
        fprintf(code_id, "    %sPub.publish(&%s);\n}\n\n",
                i->name, i->in->value->name);
    }
}

 
/*void create_outgoing_data_sender(FV * FV_NODE)
{   // TODO (Niko): Move the generation in the add_PI_to_ros_bridge_glue
    //Pointer on the list of interface for this FV
    Interface_list *IF_list_iterator = NULL;
    //Pointer on the list of parameter for this IF
    Parameter_list *IF_In_param_list_iterator = NULL;

    //String to contain the list of parameters of the function to be called by
    //the VM
    char *generated_function_parameter_list = NULL;

    //String to contain the set of assignation from
    //parameters passed by the VM to the structure to
    //be stored in shared memory
    char *generated_assignation_code = NULL;


    //First, test in there will be some incomming messages (ie. if some PI are
    //required for this FV)
    if (PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE != 0)
    {
        // For the current node, iterate on all the IF provided by the node
        IF_list_iterator = FV_NODE->interfaces;
        while (IF_list_iterator != NULL)
        {
            //The job here has to be done only for PIs
            if (IF_list_iterator->value->direction == PI)
            {
                //First test if the acrual IF is the cyclic function, in that
                //case, nothing has to be generated
                if (((cyclic_name != NULL)
                    && (strcmp(IF_list_iterator->value->name, cyclic_name)
                    != 0)) || cyclic_name == NULL)
                {
                    //For the current IF, Iterate over the list of input
                    //parameters in order to build the string to contain the
                    //parameter list for the function to be generated
                    IF_In_param_list_iterator = IF_list_iterator->value->in;
                    while (IF_In_param_list_iterator != NULL)
                    {
                        //Build the string of parameters for the function 
                        //signature by adding the current parameter
                        build_function_parameter_list
                            (&generated_function_parameter_list,
                             IF_In_param_list_iterator->value->type,
                             IF_In_param_list_iterator->value->name);

                        //Build the code that generates assignation of the
                        //parameters of the function called by the VM to the
                        //data structure to be stored in the message queue
                        build_assignation_code_from_asn(
                            &generated_assignation_code,
                            IF_In_param_list_iterator->value->name);

                        //Jump to next parameter for the current IF
                        IF_In_param_list_iterator = IF_In_param_list_iterator->next;
                    } //end while(IF_In_param_list_iterator != NULL)

                    //
                    //Generate the code of the function to be called by the VM
                    //when the IF is stimulated
                    //

                    //Step 0 : Generate function prototype function to be called
                    //         by the VM when the IF is stimulated in header file
                    //         and code

                    //Generate the header for the case where parameters
                    //associated to the IF
                    fprintf(header_id, "void %s_PI_%s(%s);\n\n",
                        FV_NODE->name,
                        IF_list_iterator->value->name,
                        generated_function_parameter_list);
                    
                    //Generate the code for the case where parameters are
                    //associated to the IF
                    fprintf(code_id, "void %s_PI_%s(%s)\n{\n",
                        FV_NODE->name,
                        IF_list_iterator->value->name,
                        generated_function_parameter_list);

                    //Step 1 : Create code to publish the data 
                    
                    //Add assignement code to the variable structure to be
                    //stored in the queue
                    fprintf(code_id, "\n%s\n", generated_assignation_code);

                    //Call the publisher
                    fprintf(code_id, "%sPub.publish(&%s);\n",
                            IF_list_iterator->value->name,
                            IF_list_iterator->value->in->name);

                    //Step 2 : Finalize
                    fprintf(code_id, "}\n\n");

                    //Step 3 : Reinitialize things for next round
                    generated_function_parameter_list = NULL;
                    generated_assignation_code = NULL;
                } // end if ( ( (cyclic_name != NULL) && ( strcmp(IF_list_iterator...
            } // end IF_list_iterator->value->direction == PI
            //Jump to next IF for the current FV
            IF_list_iterator = IF_list_iterator->next;        
        } // end while(IF_list_iterator != NULL)
    } // end if(PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE != 0)
}*/


void add_RI_to_ros_bridge_glue(Interface * i)
{
    //Parameter_list *tmp;
    T_RI_PI_NAME_LIST *new_elt;

    //
    // 1. Update the list of RI for this FV (add in head of linked list)
    //
    new_elt = (T_RI_PI_NAME_LIST *) malloc(sizeof(T_RI_PI_NAME_LIST));
    new_elt->name = (char *) malloc((strlen(i->name) + 1) * sizeof(char));
    strcpy(new_elt->name, i->name);
    new_elt->next = RI_LIST;
    RI_LIST = new_elt;

    // Indlude the header of the data type
    fprintf(code_id, "#include \"%s/%c%s.h\"\n",
            i->in->value->asn1_module, toupper(i->in->value->type[0]),
            &i->in->value->type[1]);

    // Generate RI function prototype
    fprintf(header_id, "extern void %s_RI_%s(const asn1Scc%s *);\n\n",
            i->parent_fv->name, i->name, i->in->value->type);

    // Generate callback definition
    fprintf(code_id, "void %sCb(const %s::%c%s &msg)\n", 
            i->name, i->in->value->asn1_module,
            toupper(i->in->value->type[0]), &i->in->value->type[1]);
    fprintf(code_id, "{\n");
    fprintf(code_id, "    asn1Scc%s *val;\n", i->in->value->type);
    fprintf(code_id, "    val = (asn1Scc%s *)malloc(sizeof(asn1Scc%s));\n",
		    i->in->value->type, i->in->value->type);
    fprintf(code_id, "    if(val == NULL)\n");
    fprintf(code_id, "    {\n");
    fprintf(code_id, "        perror(\"Error while allocating memory in %sCb\");\n",
            i->name);
    fprintf(code_id, "        return;\n");
    fprintf(code_id, "    }\n");
    fprintf(code_id, "    msg.toASN1(val);\n");
    fprintf(code_id, "    %s_RI_%s(val);\n", i->parent_fv->name, i->name);
    fprintf(code_id, "}\n\n");

    // Generate subscriber
    fprintf(code_id, "ros::Subscriber <%s::%c%s> %sSub(\"%s\", &%sCb);\n",
            i->in->value->asn1_module, toupper(i->in->value->type[0]),
            &i->in->value->type[1], i->name, i->in->value->name, i->name);
}

void ros_bridge_preamble(FV * fv)
{
    if ((NULL == header_id) || (NULL == enums_header_id)
    || (NULL == code_id))
    return;

    //Initialize the list of Requird Interface
    RI_LIST = NULL;
    //Initialize the list of Provided Interface
    PI_LIST = NULL;

    // headers preamble
    fprintf(enums_header_id,
        "/* This file was generated automatically: DO NOT MODIFY IT ! */\n\n");
    fprintf(enums_header_id, "#ifndef _%s_ENUM_DEF_H\n", fv->name);
    fprintf(enums_header_id, "#define _%s_ENUM_DEF_H\n\n\n", fv->name);

    fprintf(header_id,
        "/* This file was generated automatically: DO NOT MODIFY IT ! */\n\n");
    fprintf(header_id, "#ifndef _%s_ROS_BRIDGE_HEADER_H\n", fv->name);
    fprintf(header_id, "#define _%s_ROS_BRIDGE_HEADER_H\n\n\n", fv->name);
    fprintf(header_id,
        "#include <stdlib.h>\n#include <stdio.h>\n\n#include \"C_ASN1_Types.h\"\n\n");
    fprintf(header_id, "#include \"%s_enums_def.h\"\n\n\n", fv->name);

    fprintf(header_id, "#ifdef __cplusplus\n");
    fprintf(header_id, "extern \"C\" {\n");
    fprintf(header_id, "#endif\n\n");

    //Create initialization function prototype
    fprintf(header_id, "void %s_startup();\n\n", fv->name);

    // code preamble
    fprintf(code_id, "#include <unistd.h>\n");
    fprintf(code_id, "#include \"%s_ros_bridge_header.h\"\n\n\n", fv->name);
    
    // Include the ROS header
    fprintf(code_id, "#include \"ros.h\"\n");

    // Declare a global Nodehandle
    fprintf(code_id, "ros::NodeHandle nh;\n");


}

void create_cyclic_interface(char *node_name)
{
    fprintf(header_id, "void %s_PI_%s();\n\n", node_name, cyclic_name);
    fprintf(code_id, "void %s_PI_%s()\n{\n", node_name, cyclic_name);
    fprintf(code_id, "    nh.spinOnce();\n}\n\n");
}

void create_startup_funcion(FV *fv)
{
    fprintf(code_id, "void %s_startup()\n{\n", fv->name);
    // TODO (Niko): This generates a static IP. Should be somehow configurable.
    fprintf(code_id, "    const char *ros_master = \"127.0.0.1\";\n");
    fprintf(code_id, "    nh.initNode(ros_master);\n");
    FOREACH (i, Interface, fv->interfaces, 
    {
        if (strcmp(i->name, cyclic_name)) 
        {
            if (PI == i->direction) 
            {
                // Create publisher
                fprintf(code_id, "    nh.advertise(%sPub);\n", i->name);
            } 
            else 
            {
                fprintf(code_id, "    nh.subscribe(%sSub);\n", i->name);
            }
        }
    })
    fprintf(code_id, "}\n\n");
}
                

void Init_ROS_Bridge_Backend(FV *fv)
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
    build_string(&header_name, "_ros_bridge_header.h", strlen("_ros_bridge_header.h"));
    create_file(path, header_name, &header_id);

    code_id = NULL;
    build_string(&code_name, fv->name, strlen(fv->name));
    build_string(&code_name, "_ros_bridge_code.cpp", strlen("_ros_bridge_code.cpp"));
    create_file(path, code_name, &code_id);

    enums_header_id = NULL;
    build_string(&enum_header_name, fv->name, strlen(fv->name));
    build_string(&enum_header_name, "_enums_def.h",
         strlen("_enums_def.h"));
    create_file(path, enum_header_name, &enums_header_id);

    //Call the preamble function that will start to fill the created
    //files with global informations (types def, variable definitions...)
    ros_bridge_preamble(fv);

    free(path);
}

void GLUE_ROS_Bridge_Interface(Interface *i)
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

void End_ROS_Bridge_Glue_Backend(FV *fv)
{
    //Creates an enumerated to contain the list of all possible RI for this FV
    //and put it in a dedicated header file
    char *ri_list_enum = NULL;
    char *RI_list_enum_name = (char *) malloc((strlen("T__RI_list") +
                           strlen(fv->name) +
                           1) * sizeof(char));

    sprintf(RI_list_enum_name, "T_%s_RI_list", fv->name);
    if (RI_LIST != NULL) {
    create_ros_enum_type_from_list(RI_list_enum_name, RI_LIST,
                   &ri_list_enum);
    fprintf(enums_header_id, "%s", ri_list_enum);
    }
    //Creates an enumerated to contain the list of all possible PI for this FV
    //and put it in a dedicated header file
    char *pi_list_enum = NULL;
    char *PI_list_enum_name = (char *) malloc((strlen("T__PI_list") +
                           strlen(fv->name) +
                           1) * sizeof(char));

    sprintf(PI_list_enum_name, "T_%s_PI_list", fv->name);
    if (PI_LIST != NULL) {
    create_ros_enum_type_from_list(PI_list_enum_name, PI_LIST,
                   &pi_list_enum);
    fprintf(enums_header_id, "%s", pi_list_enum);
    }
    fprintf(enums_header_id, "\n\n#endif\n");

    //Close the file containing enumerated types listing RI and PI
    fclose(enums_header_id);
    
    create_cyclic_interface(fv->name);

    fprintf(header_id, "#ifdef __cplusplus\n");
    fprintf(header_id, "}\n");
    fprintf(header_id, "#endif");

    //Finalize header code file
    fprintf(header_id, "\n\n#endif\n");
    fclose(header_id);

    create_startup_funcion(fv);
    //finalize code file
    fclose(code_id);
}

void GLUE_ROS_Bridge_Backend(FV * fv)
{

    printf("ROS Bridge is generated!\n");
    if (fv->system_ast->context->onlycv)
    {
	printf("Building only CV!\n");
        return;
    }

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
        if (NULL != cyclic_name) {
            free(cyclic_name);
            cyclic_name = NULL;
        }
    } else {
	printf("Language is: %d\n",fv->language);
    }
}
