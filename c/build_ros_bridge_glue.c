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

void create_incomming_data_pooler(char *NODE_NAME)
{
    if (RI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE != 0)
    {
        //Create the initialisation function in the header
        fprintf(header_id, "void %s_PI_%s();\n\n", NODE_NAME, cyclic_name);

        //Create the initialisation function code in the source file
        fprintf(code_id, "void %s_PI_%s()\n", NODE_NAME, cyclic_name);
        fprintf(code_id, "{\n");
        fprintf(code_id, "\tif(ros::ok())\n");
        fprintf(code_id, "\t{\n");
        fprintf(code_id, "\t\tros::spinOnce();\n");
        fprintf(code_id, "\t}\n\telse\n\t{\n");
        fprintf(code_id, "\t\texit(0);\n\t}\n}\n\n");
    }
}

void create_outcomming_data_sender(FV * FV_NODE)
{
    Interface_list *IF_list_iterator = NULL;    //Pointer on the list of interface for this FV
    Parameter_list *IF_In_param_list_iterator = NULL;   //Pointer on the list of parameter for this IF

    char *generated_function_parameter_list = NULL; //String to contain the list of parameters of
    //the function to be called by the VM

    char *generated_assignation_code = NULL;    //String to contain the set of assignation from
    //parameters passed by the VM to the structure to
    //be stored in shared memory

    if (PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE != 0)
    {
        IF_list_iterator = FV_NODE->interfaces;
        while (IF_list_iterator != NULL)
        {
            if (IF_list_iterator->value->direction == PI)
            {
                if (((cyclic_name != NULL)
                     && (strcmp(IF_list_iterator->value->name, cyclic_name) != 0))
                        || cyclic_name == NULL)
                {
                    // Insert a publisher.
                    fprintf(code_id, "ros::Publisher %s_pub;\n\n",
                            IF_list_iterator->value->name);
                    // Create the PI function that is called.
                    //For the current IF, Iterate over the list of input parameters
                    //in order to build the string to contain the parameter list
                    //for the function to be generated
                    IF_In_param_list_iterator =
                    IF_list_iterator->value->in;
                    while (IF_In_param_list_iterator != NULL)
                    {
                        //Build the string of parameters for the function signature by
                        //adding the current parameter
                        build_function_parameter_list
                            (&generated_function_parameter_list,
                             IF_In_param_list_iterator->value->type,
                             IF_In_param_list_iterator->value->name);

                        //Build the code that generates assignation of the parameters of the function
                        //called by the VM to the data structure to be stored in the message queue
                        /*build_assignation_code(&generated_assignation_code,
                                               IF_list_iterator->value->
                                               name,
                                               IF_In_param_list_iterator->
                                               value->name);*/

                        //Jump to next parameter for the current IF
                        IF_In_param_list_iterator =
                            IF_In_param_list_iterator->next;
                    } //end while(IF_In_param_list_iterator != NULL)

                    //Generate the code of the function to be called by the VM when the IF is stimulated

                    //Step 0 : Generate function prototype function to be called by the VM when the IF is
                    //         stimulated in header file and code
                    if (generated_function_parameter_list != NULL)
                    {
                        //Generate the header for the case where parameters  associated to the IF
                        fprintf(header_id, "void %s_PI_%s(%s);\n\n",
                                FV_NODE->name,
                                IF_list_iterator->value->name,
                                generated_function_parameter_list);
                        // TODO: Find out which message type it is and insert the ros header.

                        //Generate the code for the case where parameters are associated to the IF
                        fprintf(code_id, "void %s_PI_%s(%s)\n{\n",
                                FV_NODE->name,
                                IF_list_iterator->value->name,
                                generated_function_parameter_list);
                    }
                    else
                    {
                        //Generate the header for the case where no parameters are associated to the IF
                        fprintf(header_id, "void %s_%s(void);\n\n",
                                FV_NODE->name,
                                IF_list_iterator->value->name);
                        //Generate the code for the case where no parameters are associated to the IF
                        fprintf(code_id, "void %s_PI_%s(void)\n{\n",
                                FV_NODE->name,
                                IF_list_iterator->value->name);
                    }
                    //Step 1 : TODO: Create local message variable.

                    //Step 2 : TODO: Call the publisher.
                    //Step 3 : Finalise.
                    fprintf(code_id, "}\n\n");
                    generated_function_parameter_list = NULL;
                }
            }
            IF_list_iterator = IF_list_iterator->next;
        }
    }
}

void create_incoming_data_sender(FV *fv)
{
    // Iterate over the interfaces.
    // - For each RI create:
    // -- Subscriber
    // -- Callback

    Interface_list *IF_list_iterator = NULL;    //Pointer on the list of interface for this FV
    Parameter_list *IF_In_param_list_iterator = NULL;   //Pointer on the list of parameter for this IF

    //String to contain the list of parameters of the function to be called by the VM
    char *generated_function_parameter_list = NULL;

    if (RI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE != 0)
    {
        IF_list_iterator = FV_NODE->interfaces;
        while (IF_list_iterator != NULL)
        {
            if (IF_list_iterator->value->direction == RI)
            {
                // TODO: Find the message type
                char *type_name = NULL;

                // Generate the callback function declaration in the header
                fprintf(header_id, "void %s_cb(%s);\n\n",
                        IF_list_iterator->value->name,
                        type_name);

                // Generate the subscriber
                fprintf(code_id, "ros::Subscriber %s_sub;\n\n",
                        IF_list_iterator->value->name);

                // Generate the callback funcrion definition
                fprintf(code_id, "void %s_cb(%s msg)\n",
                        IF_list_iterator->value->name,
                        type_name);
                fprintf(code_id, "{\n");
                // Assign the message contents to an asn1 type
                fprintf(code_id, "\tasn1Scc%s send_msg = msg;\n",
                        IF_list_iterator->value->out->type);
                // Call the remote PI
                fprintf(code_id, "\t INVOKE_RI_%s (send_msg)\n",
                        IF_list_iterator->value->name);
                fprintf(code_id, "}\n\n");
                free(type_name);
            }
            IF_list_iterator = IF_list_iterator->next;
        }
    }

}

void create_startup_function(FV *fv)
{
    fprintf(code_id, "void %s_startup(void)\n", fv->name);
    fprintf(code_id, "{\n");

    // Initialise ROS
    fprintf(code_id, "\tint argc = 0;\n");
    fprintf(code_id, "\tchar **argv;\n");
    fprintf(code_id, "\tros::init(argc, argv, \"bridge\");\n");
    fprintf(code_id, "\tros::NodeHandle n;\n");

    // Iterate over the interfaces and create publishers and subscribers
    IF_list_iterator = FV_NODE->interfaces;
    while (NULL != IF_list_iterator)
    {
        // TODO: Find the message type
        char *type_name = NULL;
        if (RI == IF_list_iterator->value->direction)
        {
            // Create subscriber
            fprintf(code_id, "\t%s_sub = n.subscribe(\"%s_topic\", 1, %s_cb);\n",
                    IF_list_iterator->value->name,
                    IF_list_iterator->value->name,
                    IF_list_iterator->value->name);
        }
        else if (PI == IF_list_iterator->value->direction)
        {
            // Create publisher
            fprintf(code_id, "\t%s_pub = n.advertise<%s>(\"%s_topic\", 1);\n",
                    IF_list_iterator->value->name,
                    type_name,
                    IF_list_iterator->value->name);
        }
        free(type_name);
        IF_list_iterator = IF_list_iterator->next;
    }
    fprintf(code_id, "}\n\n");
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
    create_enum_type_from_list(RI_list_enum_name, RI_LIST,
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
    create_enum_type_from_list(PI_list_enum_name, PI_LIST,
                   &pi_list_enum);
    fprintf(enums_header_id, "%s", pi_list_enum);
    }
    fprintf(enums_header_id, "\n\n#endif\n");

    //Close the file containing enumerated types listing RI and PI
    fclose(enums_header_id);

    // Create the cyclic interface for spinning
    create_incomming_data_pooler(fv->name); //TODO: Maybe name is not that good

    // Create the infrastructure for PI/Publishers
    create_outcomming_data_sender(fv);

    // Create the infrastructure for RI/Subscribers
    create_incoming_data_sender(fv);

    // Finalise the header file
    fprintf(header_id, "\n\n#endif\n");
    fclose(header_id);

    // Create the startup function
    create_startup_function(fv);

    // Finalise the code file
    fclose(code_id);
}

void add_PI_to_ros_bridge_glue(Interface *i)
{
    Parameter_list *tmp;
    T_RI_PI_NAME_LIST *new_elt;
    if (((cyclic_name != NULL) && (strcmp(i->name, cyclic_name) != 0))
            || cyclic_name == NULL)
    {
        /*
         * 1. Update the list of PI for this FV
         */
        new_elt = (T_RI_PI_NAME_LIST *) malloc(sizeof(T_RI_PI_NAME_LIST));
        new_elt->name = (char *) malloc((strlen(i->name) + 1) * sizeof(char));
        strcpy(new_elt->name, i->name);
        PI_LIST = new_elt;

        /*
         * 2. Creates the structure to contain ALL parameters related to the PI
         */

        // Start the struct definition of the functional data
        fprintf(header_id, "typedef struct\n{\n");

        // Loop over the parameters
        tmp = i->in;
        while (NULL != tmp)
        {
            // Add the current parameter as a field in the structure
            fprintf(header_id, "\tasn1Scc%s %s;\n", tmp->value->type,
                    tmp->value->name);

            // Jump to the next one...
            tmp = tmp->next;
        }

        // End the struct definition of the functional data
        fprintf(header_id, "} T_%s__data;\n\n", i->name);

        /*
         * 3. Creates the structure to contain the message related to the PI
         */
        fprintf(header_id, "typedef struct\n{\n");
        fprintf(header_id, "\tT_%s_PI_list\tmessage_identifier;\n",
                i->parent_fv->name);
        fprintf(header_id, "\tT_%s__data\tmessage;\n} T_%s_message;\n\n\n",
                i->name, i->name);
    }
}

void add_RI_to_ros_bridge_glue(Interface *i)
{
    Parameter_list *tmp;
    T_RI_PI_NAME_LIST *new_elt;

    /*
     * 1. Update the list of RI for this FV (add in the head of the linked list)
     */
    new_elt = (T_RI_PI_NAME_LIST *) malloc(sizeof(T_RI_PI_NAME_LIST));
    new_elt->name = (char *) malloc((strlen(i->name) + 1) * sizeof(char));
    strcpy(new_elt->name, i->name);
    new_elt->next = RI_LIST;
    RI_LIST = new_elt;

    /*
     * 2. Creates the structure to contain ALL parameters related to the RI
     */

    //Start the struct definition of the functional data
    fprintf(header_id, "typedef struct\n{\n");

    // Loop over input parameters
    tmp = i->in;
    while (NULL != tmp)
    {
        // Add the current parameter as a field in the structure
        fprintf(header_id, "\tasn1Scc%s %s;\n", tmp->value->type,
                tmp->value->name);

        // Jump to next one...
        tmp = tmp->next;
    }

    // End the struct definition of the functional data
    fprintf(header_id, "} T_%s__data;\n\n", i->name);

    /*
     * 3. Creates the structure to contain the message related to the RI
     */
    fprintf(header_id, "typedef struct\n{\n");
    fprintf(header_id, "\tT_%s_RI_list\tmessage_identifier;\n",
            i->parent_fv->name);
    fprintf(header_id, "\tT_%s__data\tmessage;\n} T_%s_message;\n\n\n",
            i->name, i->name);

    /*
     * 4. Create the macro for encoding and sending to VM
     */
    fprintf(header_id, "void %s_RI_%s(", i->parent_fv->name, i->name);
    bool comma = false;
    FOREACH (param, Parameter, i->in, {
        fprintf(header_id, "%sconst asn1Scc%s *", comma? ", ": "", param->type);
        comma = true;
    });
    fprintf(header_id, ");\n\n"
                       "#define INVOKE_RI_%s(params) %s_RI_%s(",
                       i->name,
                       i->parent_fv->name,
                       i->name);
    tmp = i->in;
    // Add parameters
    while (NULL != tmp)
    {
        fprintf(header_id, "%s&((T_%s__data*)params)->%s",
                (tmp != i->in) ? ", " : "", i->name, tmp->value->name);
        tmp = tmp->next;
    }
    fprintf(header_id, ");\n\n");
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
