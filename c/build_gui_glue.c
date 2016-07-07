/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* build_gui_glue.c

  this program generates the code to interface a GUI with the assert virtual machine
  author: cyril colombo/Vega for ESA (this guy is a genuine mercenaire...)(but the best one)
  
  updated 20/04/2009 to disable in case "-onlycv" flag is set
 */

#define ID "$Id: build_gui_glue.c 437 2010-01-06 09:19:17Z maxime1008 $"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "my_types.h"
#include "practical_functions.h"

/* Constant for the sizing of the maximum number
 * of messages to be hold by the queue. The queue
 * will thus be sized as this constant multiplicated
 * by the size of the biggest possible message to
 * be put in the queue
 */
#define C_MAX_NUMBER_MESSAGES    5

/* Constant defining the maximen number of character in a line of a source file */
#define C_MAX_LINE_SIZE    256

/*Constant defining the maximum number of characters of a file name */
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




/*
 * Internal interfaces
 */

//This function allows to get a string suffix "PI" ou "RI"
//depending on the value of the enumerated type parameter
//given in input of the service
char *enum_IF_to_string(IF_type IF_KIND)
{
    static char IF_TYPE_STRING[3] = ""; //Static mandatory here...

    //Just copute a string prefix depending of the interface kind we have
    if (IF_KIND == PI) {
    strcpy(IF_TYPE_STRING, "PI");
    } else {
    strcpy(IF_TYPE_STRING, "RI");
    }

    return (IF_TYPE_STRING);
}

//Adds to an existing string a string of the form : "%s1* %s2,"
//Where :
// %s1 = parameter_type         (eg. asn1Scctype_du_param_1)
// %s2 = parameter_name         (eg. nom_du_param_1)
void build_function_parameter_list(char **generated_code,
                   char *parameter_type,
                   char *parameter_name)
{
    if (*generated_code != NULL)
    build_string(generated_code, ",", strlen(","));
    build_string(generated_code, "asn1Scc", strlen("asn1Scc"));
    build_string(generated_code, parameter_type, strlen(parameter_type));
    build_string(generated_code, "* ", strlen("* "));
    build_string(generated_code, parameter_name, strlen(parameter_name));

}



//Adds to an existing string a string of the form : "   %s1_data.%s2 = %s2;\n"
//Where :
// %s1 = IF_name        (eg. PI_RED_BUTTON)
// %s2 = parameter_name (eg. param1)
void build_assignation_code(char **generated_code, char *IF_name,
                char *parameter_name)
{
    build_string(generated_code, "   ", strlen("   "));
    build_string(generated_code, IF_name, strlen(IF_name));
    build_string(generated_code, "_", strlen("_"));
    build_string(generated_code, "data.", strlen("data."));
    build_string(generated_code, parameter_name, strlen(parameter_name));
    build_string(generated_code, " = *", strlen(" = *"));
    build_string(generated_code, parameter_name, strlen(parameter_name));
    build_string(generated_code, ";\n", strlen(";\n"));
}



//
// Creates a string for the creation of the necessary receptacles variables
// to be used by GUI developpers. These one are to be declared as globals
// in the generated code, so that they can be used whatever way, once they
// will have been filled after reception of the queue messages.
//
// Pattern created : static asn1Scc%s1 devel_%s2; (%s1 type name, %s2 variable name)
//
void create_receptacle_variables(char **pgenerated_function_parameter_list,
                 char *type_name, char *var_name)
{
    build_string(pgenerated_function_parameter_list,
         "static asn1Scc", strlen("static asn1Scc"));

    build_string(pgenerated_function_parameter_list,
         type_name, strlen(type_name));

    build_string(pgenerated_function_parameter_list, " devel_",
         strlen(" devel_"));

    build_string(pgenerated_function_parameter_list,
         var_name, strlen(var_name));

    build_string(pgenerated_function_parameter_list, ";\n", strlen(";\n"));
}



//
// Creates a string to contain a list of local variables to be used as
// receptacle of the the complete messages issued from the queue, before
// they are stripped out to fill the global variables defined just before.
//
// Pattern created : T_%s1_data   %s1_data_receptacle; (%s1 PI name)
//
void create_local_Q_recption_variables(char
                       **pgenerated_function_local_message_list,
                       char *PI_name)
{
    build_string(pgenerated_function_local_message_list,
         "T_", strlen("T_"));

    build_string(pgenerated_function_local_message_list,
         PI_name, strlen(PI_name));

    build_string(pgenerated_function_local_message_list,
         "_data ", strlen("_data "));

    build_string(pgenerated_function_local_message_list,
         PI_name, strlen(PI_name));

    build_string(pgenerated_function_local_message_list,
         "_data_receptacle;\n", strlen("_data_receptacle;\n"));
}



//
// Create assignation code from local messages structures to global user variables
//
// Pattern created : devel_nom_du_param_2 = PI_info_red_data_receptacle.nom_du_param_2;
//
void create_assignation_code(char **pgenerated_assignation_code,
                 char *IF_name, char *param_name)
{
    build_string(pgenerated_assignation_code, "devel_", strlen("devel_"));

    build_string(pgenerated_assignation_code,
         param_name, strlen(param_name));

    build_string(pgenerated_assignation_code, " = ", strlen(" = "));

    build_string(pgenerated_assignation_code, IF_name, strlen(IF_name));

    build_string(pgenerated_assignation_code,
         "_data_receptacle.", strlen("_data_receptacle."));

    build_string(pgenerated_assignation_code,
         param_name, strlen(param_name));

    build_string(pgenerated_assignation_code, ";\n", strlen(";\n"));
}



//
// Create the switch structure that will be placed after the message identification
//
//
// Pattern created :
//    case PI_info_red : memcpy(&PI_info_red_data_receptacle, message_data_received, sizeof(T_PI_info_red_data));
//                            devel_nom_du_param_2 := PI_info_red_data_receptacle.nom_du_param_2
//                            devel_nom_du_param_1 := PI_info_red_data_receptacle.nom_du_param_1;
//                       break;
//
void create_switch_structure(char **pgenerated_switch_structure,
                 char *PI_name,
                 char *PI_related_assignation_code)
{
    //First test if there are assignation to be made
    if (PI_related_assignation_code != NULL) {
    build_string(pgenerated_switch_structure,
             "case i_", strlen("case i_"));

    build_string(pgenerated_switch_structure,
             PI_name, strlen(PI_name));

    build_string(pgenerated_switch_structure,
             " : memcpy(&", strlen(" : memcpy(&"));

    build_string(pgenerated_switch_structure,
             PI_name, strlen(PI_name));

    build_string(pgenerated_switch_structure,
             "_data_receptacle, message_data_received, sizeof(T_",
             strlen
             ("_data_receptacle, message_data_received, sizeof(T_"));

    build_string(pgenerated_switch_structure,
             PI_name, strlen(PI_name));

    build_string(pgenerated_switch_structure,
             "_data));\n", strlen("_data));\n"));

    build_string(pgenerated_switch_structure,
             PI_related_assignation_code,
             strlen(PI_related_assignation_code));

    build_string(pgenerated_switch_structure,
             "break;\n\n", strlen("break;\n\n"));
    } else {
    build_string(pgenerated_switch_structure,
             "case i_", strlen("case i_"));

    build_string(pgenerated_switch_structure,
             PI_name, strlen(PI_name));

    build_string(pgenerated_switch_structure,
             " : break;\n", strlen(" : break;\n"));
    }
}



//Start the filling of generated code files
//
//CCY 28/08/08 : (DOCUMENTATION) Document this function
//
void gui_preamble(FV * fv)
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
    fprintf(header_id, "#ifndef _%s_GUI_HEADER_H\n", fv->name);
    fprintf(header_id, "#define _%s_GUI_HEADER_H\n\n\n", fv->name);
    fprintf(header_id,
        "#include <stdlib.h>\n#include <stdio.h>\n\n#include \"C_ASN1_Types.h\"\n\n");
    fprintf(header_id, "#include \"%s_enums_def.h\"\n\n\n", fv->name);

    //Create initialization function prototype
    fprintf(header_id, "void %s_startup();\n\n", fv->name);

    // code preamble
    fprintf(code_id, "#include <unistd.h>\n");
    fprintf(code_id, "#include <mqueue.h>\n");
    fprintf(code_id, "#include \"queue_manager.h\"\n");
    fprintf(code_id, "#include \"%s_gui_header.h\"\n\n\n", fv->name);
}


//Creation of the queue initialization code
//
//CCY 28/08/08 : (DOCUMENTATION) Document this function
//
void create_single_queue_initialization_function(char *NODE_NAME,
                         IF_type IF_KIND,
                         int
                         IF_MAX_NUMBER_OF_PARAMS_IN_MESSAGE)
{
    char IF_TYPE_STRING[3] = "";

    //Compute the string suffix depending of the kind of IF we manage
    strcpy(IF_TYPE_STRING, enum_IF_to_string(IF_KIND));

    //Parse the created structures to find out the biggest message to be sent fo RI
    fprintf(code_id,
        "   %s_%s_max_msg_size = compute_max_queue_size_element_for_%s();\n\n",
        NODE_NAME, IF_TYPE_STRING, IF_TYPE_STRING);

    //Create queue
    fprintf (code_id, "   {\n    char *gui_queue_name = NULL;\n");
    fprintf (code_id, "    int  len = snprintf (gui_queue_name, 0, \"%%d_%s_%s_queue\", geteuid());\n", NODE_NAME, IF_TYPE_STRING);
    fprintf (code_id, "    gui_queue_name = (char *) malloc ((size_t) len + 1);\n");
    fprintf (code_id, "    if (NULL != gui_queue_name) {\n");
    fprintf (code_id, "       snprintf (gui_queue_name, len + 1, \"%%d_%s_%s_queue\", geteuid());\n\n", 
       NODE_NAME, 
       IF_TYPE_STRING);
    fprintf(code_id,
        "       create_exchange_queue(gui_queue_name, %i, %s_%s_max_msg_size, &%s_%s_queue_id);\n\n",
        C_MAX_NUMBER_MESSAGES * IF_MAX_NUMBER_OF_PARAMS_IN_MESSAGE,
        NODE_NAME, IF_TYPE_STRING, NODE_NAME, IF_TYPE_STRING);
    fprintf (code_id, "       free (gui_queue_name);\n       gui_queue_name = NULL;\n    }\n");

    if (PI == IF_KIND) {
        fprintf (code_id, "    len = snprintf (gui_queue_name, 0, \"%%d_%s_PI_Python_queue\", geteuid());\n", NODE_NAME);
        fprintf (code_id, "    gui_queue_name = (char *) malloc ((size_t) len + 1);\n");
        fprintf (code_id, "    if (NULL != gui_queue_name) {\n");
        fprintf (code_id, "       snprintf (gui_queue_name, len + 1, \"%%d_%s_PI_Python_queue\", geteuid());\n\n", NODE_NAME);

    fprintf(code_id,
        "       /* Extra queue for the TM sent to the Python mappers */\n");
    fprintf(code_id,
        "       create_exchange_queue(gui_queue_name, %i, %s_PI_max_msg_size, &%s_PI_Python_queue_id);\n\n",
        C_MAX_NUMBER_MESSAGES * IF_MAX_NUMBER_OF_PARAMS_IN_MESSAGE,
        NODE_NAME, NODE_NAME);
        fprintf (code_id, "       free (gui_queue_name);\n       gui_queue_name = NULL;\n    }\n");
    }
    fprintf (code_id, "   }\n");
}



//Creation of the queue initialization code
//
//CCY 28/08/08 : (DOCUMENTATION) Document this function
//
void create_queues_initialization_function(char *NODE_NAME)
{
    //
    //Create the initialisation functions to be called at startup
    //
    fprintf(code_id, "void %s_startup()\n", NODE_NAME);
    fprintf(code_id, "{\n");
    fprintf(code_id, "   int res_RI = 0;\n");
    fprintf(code_id, "   int res_PI = 0;\n\n");

    //Create the RI queue initialization function, if there are some RI to be stored
    if (RI_LIST != NULL) {
    create_single_queue_initialization_function(NODE_NAME, RI,
                            RI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE);
    }
    //create the PI queue initialization function, if there are some PI to be stored
    if (PI_LIST != NULL) {
    create_single_queue_initialization_function(NODE_NAME, PI,
                            PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE);
    }
    //End procedure
    fprintf(code_id, "   return;\n");
    fprintf(code_id, "}\n\n\n");
}


//
// This function generates the code that determines the size of the biggest possible element to be
// stored in the queue. Renember that there are two queues per node (FV), one the for the RI and
// PI provided, and that for a RI a message to be stored in the queue (its format being a message
// identifier followed by a structure containing a field for each parameter of the RI/PI).
// Here, we take the list of the RI or PI provided by the node, and construct the code that will
// determine the maximum size of the message the interface is able to write in the queue.
//
int create_maxelement_queue_size_compute_function_on_IF(T_RI_PI_NAME_LIST *
                            IF_LIST,
                            IF_type IF_KIND)
{
    char IF_TYPE_STRING[3] = "";
    T_RI_PI_NAME_LIST *iterator = NULL;

    //Compute the string suffix depending of the kind of IF we manage
    strcpy(IF_TYPE_STRING, enum_IF_to_string(IF_KIND));

    //Create function
    fprintf(code_id, "int compute_max_queue_size_element_for_%s(void)\n",
        IF_TYPE_STRING);
    fprintf(code_id, "{\n");
    fprintf(code_id,
        "   int MAX_SIZE = sizeof(int);   // The minimum size of a message (id + functional data) is the size of the id alone !\n\n");

    //Initiate iterator on the linked list of IF provided
    iterator = IF_LIST;
    while (NULL != iterator) {

    fprintf(code_id, "   if( sizeof(T_%s_message) > MAX_SIZE )\n",
        iterator->name);
    fprintf(code_id, "      MAX_SIZE = sizeof(T_%s_message);\n\n",
        iterator->name);

    iterator = iterator->next;
    }

    //Finalize the function code
    fprintf(code_id, "   return(MAX_SIZE);\n");
    fprintf(code_id, "}\n\n\n");

    return 0;
}



//Creation of the code of a function in charge of computing the biggest element
//that will be stored in the queue
//
//CCY 28/08/08 : (DOCUMENTATION) Document this function
//
int create_max_element_queues_size_compute_function(char *NODE_NAME)
{
   (void) NODE_NAME;
    //First, do it for the RI
    create_maxelement_queue_size_compute_function_on_IF(RI_LIST, RI);

    //Second, do it for the PI
    create_maxelement_queue_size_compute_function_on_IF(PI_LIST, PI);

    return 0;
}

// Creates the queue creation code
//
//CCY 28/08/08 : (DOCUMENTATION) Document this function
//
void create_gui_fv_queues(char *NODE_NAME)
{
    //Create the functions allowing to determine the biggest message size for the
    //two queues (PI and RI)
    create_max_element_queues_size_compute_function(NODE_NAME);

    //Create the queues initialization, function
    create_queues_initialization_function(NODE_NAME);
}



//This function generates for a given Node (FV) the code that allows to retrived informations from
// the outside application trough a didicated message queue.
//
//   NODE_NAME  : [in] Pointer on the FV name to be handled
//
void create_incomming_data_pooler(char *NODE_NAME)
{
    //Code to be created by reading the list of RI for the FV
    T_RI_PI_NAME_LIST *iterator = RI_LIST;

    //First, test in there will be some incomming messages (ie some RI are required)
    if (RI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE != 0) {
    //Create the initialisation function in the header
    fprintf(header_id, "void %s_PI_%s();\n\n", NODE_NAME, cyclic_name);

    //Create the initialisation function code in the source file
    fprintf(code_id, "void %s_PI_%s()\n", NODE_NAME, cyclic_name);
    fprintf(code_id, "{\n");

    //Creates necessary local varaiables
    fprintf(code_id, "   int msgsz = 0;\n");
    fprintf(code_id, "   unsigned int sender = 0;\n");
    fprintf(code_id, "   struct mq_attr msgq_attr;\n");
    fprintf(code_id, "   char* msgcontent = NULL;\n");
    fprintf(code_id, "   int msg_cnt = 0;\n");
    fprintf(code_id, "   T_%s_RI_list message_recieved_type;\n\n\n",
        NODE_NAME);

    //
    //Creates the code to pool the content of the queue.
    //
    //First allocate memory for the message buffer
    fprintf(code_id,
        "   if ( (msgcontent = (char*)malloc( sizeof(char) * %s_RI_max_msg_size ) ) == NULL)\n",
        NODE_NAME);
    fprintf(code_id, "   {\n");
    fprintf(code_id,
        "      perror(\"Error when allocating memory in gui_cyc_ function\");\n");
    fprintf(code_id, "      return;\n");
    fprintf(code_id, "   }\n\n");


    //Start by getting queue attributes, including number of messages
    fprintf(code_id, "   mq_getattr(%s_RI_queue_id, &msgq_attr);\n\n",
        NODE_NAME);

    //Start the loop on the messages

    fprintf(code_id,
        "   while (!retrieve_message_from_queue(%s_RI_queue_id, %s_RI_max_msg_size, msgcontent, (int *)&message_recieved_type))\n",
        NODE_NAME, NODE_NAME);

    fprintf(code_id, "   {\n");

    //Creates the switch structure
    fprintf(code_id, "      switch(message_recieved_type)\n");
    fprintf(code_id, "      {\n");

    while (iterator != NULL) {
        fprintf(code_id,
            "       case i_%s : INVOKE_RI_%s (msgcontent);\n",
            iterator->name, iterator->name);
        fprintf(code_id, "                 break; \n");
        iterator = iterator->next;
    }
    fprintf(code_id, "       default : break;\n");
    fprintf(code_id, "      }\n");  //End switch

    fprintf(code_id, "   }\n\n");   // End for loop

    fprintf(code_id, "   free(msgcontent);\n\n");   // Free alloated memory

    fprintf(code_id, "   return;\n");
    fprintf(code_id, "}\n");    //End procedure
    }
}




//This function generates for a given Node (FV) the code that allows to send informations to the outside
//application trough a didicated message queue.
//
//To generate the code, this function iterates over all the IF provided by the FV under consideration. If
//the interface  happens to be a PI, then the list of parameters related to this parameter is retrieved
//(it is ASSUMED that such paramters will ALL be inputs parameters, ie. that the model is consistent).
//
//Starting from this list of parameters, one function (for each PI) with the following prototype is built :
//
//  void name_of_the_PI(asn1Scctype_du_param_2 nom_du_param_2,asn1Scctype_du_param_1 nom_du_param_1...);
//
//The code of the body of this function is also generated to perform the following :
//
// 1 : Affectation of all input parameters to the field of a message structure of type
//     T_name_of_the_PI_data (as defined in the name_of_the_FV_gui_header.h generated file)
//     to be sent to the queue created to recieve informations related to the PI.
// 2 : Generate the code to indeed store the message by calling the write_message_to_queue
//     function.
//
//   FV_NODE    : [in] Pointer on the FV struiucture to be handled
//
void create_outcomming_data_sender(FV * FV_NODE)
{
    Interface_list *IF_list_iterator = NULL;    //Pointer on the list of interface for this FV
    Parameter_list *IF_In_param_list_iterator = NULL;   //Pointer on the list of parameter for this IF

    char *generated_function_parameter_list = NULL; //String to contain the list of parameters of
    //the function to be called by the VM

    char *generated_assignation_code = NULL;    //String to contain the set of assignation from
    //parameters passed by the VM to the structure to
    //be stored in shared memory


    //First, test in there will be some incomming messages (ie. if some PI are required for this FV)
    if (PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE != 0) {
    // For the current node, iterate on all the IF provided by the node
    IF_list_iterator = FV_NODE->interfaces;
    while (IF_list_iterator != NULL) {
        //The job here has to be done only for PIs
        if (IF_list_iterator->value->direction == PI) {
        //First test if the acrual IF is the cyclic function, in that
        //case, nothing has to be generated
        if (((cyclic_name != NULL)
             && (strcmp(IF_list_iterator->value->name, cyclic_name)
             != 0)) || cyclic_name == NULL) {

            //For the current IF, Iterate over the list of input parameters
            //in order to build the string to contain the parameter list
            //for the function to be generated
            IF_In_param_list_iterator =
            IF_list_iterator->value->in;
            while (IF_In_param_list_iterator != NULL) {
            //Build the string of parameters for the function signature by
            //adding the current parameter
            build_function_parameter_list
                (&generated_function_parameter_list,
                 IF_In_param_list_iterator->value->type,
                 IF_In_param_list_iterator->value->name);

            //Build the code that generates assignation of the parameters of the function
            //called by the VM to the data structure to be stored in the message queue
            build_assignation_code(&generated_assignation_code,
                           IF_list_iterator->value->
                           name,
                           IF_In_param_list_iterator->
                           value->name);

            //Jump to next parameter for the current IF
            IF_In_param_list_iterator =
                IF_In_param_list_iterator->next;
            }       //end while(IF_In_param_list_iterator != NULL)

            //
            //Generate the code of the function to be called by the VM when the IF is stimulated
            //

            //strip_last_token(&generated_function_parameter_list, ',');
            //Step 0 : Generate function prototype function to be called by the VM when the IF is
            //         stimulated in header file and code
            if (generated_function_parameter_list != NULL) {
            //Generate the header for the case where parameters  associated to the IF
            fprintf(header_id, "void %s_PI_%s(%s);\n\n",
                FV_NODE->name,
                IF_list_iterator->value->name,
                generated_function_parameter_list);
            //Generate the code for the case where parameters are associated to the IF

            fprintf(code_id, "void %s_PI_%s(%s)\n{\n",
                FV_NODE->name,
                IF_list_iterator->value->name,
                generated_function_parameter_list);
            } else {
            //Generate the header for the case where no parameters are associated to the IF
            fprintf(header_id, "void %s_%s(void);\n\n",
                FV_NODE->name,
                IF_list_iterator->value->name);
            //Generate the code for the case where no parameters are associated to the IF
            fprintf(code_id, "void %s_PI_%s(void)\n{\n",
                FV_NODE->name,
                IF_list_iterator->value->name);
            }

            //Step 1 : Generate a string for the assignation of each individual data
            //         to the structure to be stored in the queue
            if (generated_function_parameter_list != NULL) {
            fprintf(code_id,
                "   //Create a variable of type T_%s_data to be contained in the queue\n",
                IF_list_iterator->value->name);
            fprintf(code_id, "   T_%s_data %s_data;\n",
                IF_list_iterator->value->name,
                IF_list_iterator->value->name);
            }
            //Step 2 : Finally create code to dump data in the queue
            if (generated_assignation_code != NULL) {

            //Add assignement code to the variable structure to be stored in the queue
            fprintf(code_id, "\n%s\n",
                generated_assignation_code);

            //Call the queue manager to store the data
            if (strcmp (FV_NODE->name, "taste_probe_console")) {
                fprintf(code_id,
                "   write_message_to_queue(%s_PI_queue_id, sizeof(T_%s_data), (void*)&%s_data, i_%s);\n\n",
                FV_NODE->name,
                IF_list_iterator->value->name,
                IF_list_iterator->value->name,
                IF_list_iterator->value->name);
            }

            // Same for the Python TM queue
            fprintf(code_id,
                "   write_message_to_queue(%s_PI_Python_queue_id, sizeof(T_%s_data), (void*)&%s_data, i_%s);\n\n",
                FV_NODE->name,
                IF_list_iterator->value->name,
                IF_list_iterator->value->name,
                IF_list_iterator->value->name);
            } else {
            //No assignement code, as no parameters are associated to the current IF, thus
            //call directly the queue manager to store the data
                        if (strcmp (FV_NODE->name, "taste_probe_console")) {
                fprintf(code_id,
                "   write_message_to_queue(%s_PI_queue_id, sizeof(T_%s_data), NULL, i_%s);\n\n",
                FV_NODE->name,
                IF_list_iterator->value->name,
                IF_list_iterator->value->name);
            }
            // Same for the Python TM queue
            fprintf(code_id,
                "   write_message_to_queue(%s_PI_Python_queue_id, sizeof(T_%s_data), NULL, i_%s);\n\n",
                FV_NODE->name,
                IF_list_iterator->value->name,
                IF_list_iterator->value->name);
            }

            //Step 3 : Finalize
            fprintf(code_id, "}\n\n");

            //Step 4 : Reinitialize things for next round
            generated_function_parameter_list = NULL;
            generated_assignation_code = NULL;
        }
        }           // end if ( ( (cyclic_name != NULL) && ( strcmp(IF_list_iterator...

        //Jump to next IF for the current FV
        IF_list_iterator = IF_list_iterator->next;  // end IF_list_iterator->value->direction == PI
    }           // end while(IF_list_iterator != NULL)
    }               // end if(PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE != 0)
}


void create_outcomming_data_sender_reader(FV * FV_NODE)
{
    Interface_list *IF_list_iterator = NULL;    //Pointer on the list of interface for this FV

    Parameter_list *IF_In_param_list_iterator = NULL;   //Pointer on the list of parameter for this IF

    char *generated_function_parameter_list = NULL; //String to contain the complete list of parameters to be recieved
    //for the FV under consideration (ie. gathers all parameters of all PIs)

    char *generated_function_local_message_list = NULL; //String to contain the complete list of messages to be recieved
    //for the FV under consideration (ie. gathers all parameters of all PIs)

    char *generated_assignation_code = NULL;    //String to contain the set of assignation from
    //parameters passed by the VM to the structure to
    //be stored in shared memory

    char *generated_switch_structure = NULL;    //String to contain the switch structure



    //First, test in there will be some incomming messages (ie. if some PI are required for this FV)
    if (PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE != 0) {
    // For the current node, iterate on all the IF provided by the node
    IF_list_iterator = FV_NODE->interfaces;
    while (IF_list_iterator != NULL) {
        //The job here has to be done only for PIs
        if (IF_list_iterator->value->direction == PI) {
        //First test if the acrual IF is not the cyclic function, in that
        //case, nothing has to be generated
        if (((cyclic_name != NULL)
             && (strcmp(IF_list_iterator->value->name, cyclic_name)
             != 0)) || cyclic_name == NULL) {
            // Creates a string to contain a list of local variables to be used as
            // receptacle of the the complete messages issued from the queue, before
            // they are stripped out to fill the global variables defined just before.
            create_local_Q_recption_variables
            (&generated_function_local_message_list,
             IF_list_iterator->value->name);

            //For the current IF, Iterate over the list of input parameters
            //in order to build the string to contain the parameter list
            //for the function to be generated
            IF_In_param_list_iterator =
            IF_list_iterator->value->in;
            while (IF_In_param_list_iterator != NULL) {
            // Creates a string for the creation of the necessary receptacles variables
            // to be used by GUI developpers. These one are to be declared as globals
            // in the generated code, so that they can be used whatever way, once they
            // will have been filled after reception of the queue messages.
            create_receptacle_variables
                (&generated_function_parameter_list,
                 IF_In_param_list_iterator->value->type,
                 IF_In_param_list_iterator->value->name);

            // Create assignation code from local messages structures to global user variables
            create_assignation_code
                (&generated_assignation_code,
                 IF_list_iterator->value->name,
                 IF_In_param_list_iterator->value->name);

            //Jump to next parameter for the current IF
            IF_In_param_list_iterator =
                IF_In_param_list_iterator->next;
            }

            // Create the switch structure that will be placed after the message identification
            create_switch_structure(&generated_switch_structure,
                        IF_list_iterator->value->name,
                        generated_assignation_code);
        }
        }
        //Clear assignation code for this IF
        //strcpy(generated_assignation_code, "");
        generated_assignation_code = NULL;

        //Jump to next IF for the current FV
        IF_list_iterator = IF_list_iterator->next;
    }
    }
}



//
// Creates a string containing the defintion of an enumerated type starting from a linked list containing
// the name of the elements composing the enumerated type to be built
//
//   type_name          : [in]  Name of the enumerated type to be built
//   items_list         : [in]  Linked list containing the name of interfaces
//   type_definition    : [out] Pointer on the string containing the built enumerated type
//
int create_enum_type_from_list(char *type_name,
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




//
//Creates files to contain the generated code for a given FV.
//
//This creates basically three files for a FV :
//
// - A file to contain two enumerated types that list
//   the set of IF for the FV under consideration. Note
//   that two list will be created (one for PIs and one
//   for RIs). File referenced by enum_header_name
//
// - A file to contain the header of functions generated
//   File referenced by header_name
//
// - A file to contain the implementation fo the fonctions
//   File referenced by code_name
//
//   fv  : [in] Pointer on the FV to handle
//
int Init_GUI_Backend(FV * fv)
{
    char *header_name = NULL;
    char *code_name = NULL;
    char *enum_header_name = NULL;

    char *path = NULL;
    if (NULL != fv->system_ast->context->output)
    build_string(&path, fv->system_ast->context->output,
             strlen(fv->system_ast->context->output));
    build_string(&path, fv->name, strlen(fv->name));

    header_id = NULL;

    build_string(&header_name, fv->name, strlen(fv->name));
    build_string(&header_name, "_gui_header.h", strlen("_gui_header.h"));
    create_file(path, header_name, &header_id);

    code_id = NULL;
    build_string(&code_name, fv->name, strlen(fv->name));
    build_string(&code_name, "_gui_code.c", strlen("_gui_code.c"));
    create_file(path, code_name, &code_id);

    enums_header_id = NULL;
    build_string(&enum_header_name, fv->name, strlen(fv->name));
    build_string(&enum_header_name, "_enums_def.h",
         strlen("_enums_def.h"));
    //enum_header_name = (char*)malloc((strlen("_enums_def.h") + strlen(NODE_NAME) + 1) * sizeof(char));
    //sprintf(enum_header_name, "%s_enums_def.h\0", NODE_NAME);
    create_file(path, enum_header_name, &enums_header_id);

    //Call the preamble function that will start to fill the created
    //files with global informations (types def, variable definitions...)
    gui_preamble(fv);

    //Free memory
    free(path);

    //Return to caller with no error
    return 0;
}


// Creates the code related to the management of a RI
//
//CCY 28/08/08 : (DOCUMENTATION) Document this function
//
void add_RI_to_gui_glue(Interface * i)
{
    Parameter_list *tmp;
    T_RI_PI_NAME_LIST *new_elt;

    //
    // 1. Update the list of RI for this FV (add in head of linked list)
    //
    new_elt = (T_RI_PI_NAME_LIST *) malloc(sizeof(T_RI_PI_NAME_LIST));
    new_elt->name = (char *) malloc((strlen(i->name) + 1) * sizeof(char));
    strcpy(new_elt->name, i->name);
    new_elt->next = RI_LIST;
    RI_LIST = new_elt;

    //
    // 2. Creates the structure to contain ALL parameters related to the RI
    //

    //Start the struct definition of the functional data
    fprintf(header_id, "typedef struct\n{\n");

    // Loop over input parameters
    tmp = i->in;
    while (NULL != tmp) {
    //Add the current parameter as a field in the structure
    fprintf(header_id, "\tasn1Scc%s %s;\n", tmp->value->type,
        tmp->value->name);

    //Jump to next one...
    tmp = tmp->next;
    }

    //End the struct definition of the functional data
    fprintf(header_id, "} T_%s_data;\n\n", i->name);

    //
    // 3. Creates the structure to contain the message related to the RI
    //
    fprintf(header_id, "typedef struct\n{\n");
    fprintf(header_id, "\tT_%s_RI_list\tmessage_identifier;\n",
        i->parent_fv->name);
    fprintf(header_id, "\tT_%s_data\tmessage;\n} T_%s_message;\n\n\n",
        i->name, i->name);


    //
    // 4. Create the macro for encoding and sending to VM
    //
    // Modified by MP 8-8-8 to reuse invoke_ri.c
    // Modified by MP 7-7-16 to declare the prototype
    fprintf(header_id, "void %s_RI_%s(", i->parent_fv->name, i->name);
    bool comma = false;
    FOREACH (param, Parameter, i->in, {
        fprintf(header_id, "%sasn1Scc%s *", comma? ", ": "", param->type);
        comma = true;
    });
    fprintf(header_id, ");\n\n"
                       "#define INVOKE_RI_%s(params) %s_RI_%s(",
                       i->name,
                       i->parent_fv->name,
                       i->name);
    tmp = i->in;
    // Add parameters
    while (NULL != tmp) {
    fprintf(header_id, "%s&((T_%s_data*)params)->%s",
        (tmp != i->in) ? ", " : "", i->name, tmp->value->name);
    tmp = tmp->next;
    }
    fprintf(header_id, ");\n\n");
}



// Creates the code related to the management of a PI
//
//CCY 28/08/08 : (DOCUMENTATION) Document this function
//
void add_PI_to_gui_glue(Interface * i)
{
    Parameter_list *tmp;
    T_RI_PI_NAME_LIST *new_elt;

    // Check if the current PI is the "cyclic" one. If so, discard this PI
    if (((cyclic_name != NULL) && (strcmp(i->name, cyclic_name) != 0))
    || cyclic_name == NULL) {

    //
    // 1. Update the list of PI for this FV
    //
    new_elt = (T_RI_PI_NAME_LIST *) malloc(sizeof(T_RI_PI_NAME_LIST));
    new_elt->name =
        (char *) malloc((strlen(i->name) + 1) * sizeof(char));
    strcpy(new_elt->name, i->name);
    new_elt->next = PI_LIST;
    PI_LIST = new_elt;

    //
    // 2. Creates the structure to contain ALL parameters related to the PI
    //

    //Start the struct definition of the functional data
    fprintf(header_id, "typedef struct\n{\n");

    // Loop over input parameters
    tmp = i->in;
    while (NULL != tmp) {
        //Add the current parameter as a field in the structure
        fprintf(header_id, "\tasn1Scc%s %s;\n", tmp->value->type,
            tmp->value->name);

        //Jump to next one...
        tmp = tmp->next;
    }

    //End the struct definition of the functional data
    fprintf(header_id, "} T_%s_data;\n\n", i->name);

    //
    // 3. Creates the structure to contain the message related to the PI
    //
    fprintf(header_id, "typedef struct\n{\n");
    fprintf(header_id, "\tT_%s_PI_list\tmessage_identifier;\n",
        i->parent_fv->name);
    fprintf(header_id, "\tT_%s_data\tmessage;\n} T_%s_message;\n\n\n",
        i->name, i->name);
    }
}



//Finalize the FV creation after all RI/PI have been added
//
//CCY 28/08/08 : (DOCUMENTATION) Document this function
//
void End_GUI_Glue_Backend(FV * fv)
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


    //Global variables for RI queue management
    fprintf(code_id, "\n//Handle to the queue of incomming messages\n");
    fprintf(code_id, "static mqd_t    %s_RI_queue_id;\n\n\n", fv->name);

    //Define global variables for RI queue size management
    fprintf(code_id,
        "\n//Variable to contain the size of the biggest type of messqges to be handled for RI\n");
    fprintf(code_id, "int    %s_RI_max_msg_size = 0;\n\n\n", fv->name);

    //Global variables for PI queue management
    fprintf(code_id, "\n//Handle to the queue of incomming messages\n");
    fprintf(code_id, "static mqd_t    %s_PI_queue_id;\n\n\n", fv->name);

    /* MP 23/11/2009 : Adding a separate TM (PI) queue for the Python Mappers */
    fprintf(code_id, "static mqd_t    %s_PI_Python_queue_id;\n\n\n",
        fv->name);

    //Define global variables for PI queue size management
    fprintf(code_id,
        "\n//Variable to contain the size of the biggest type of messqges to be handled for PI\n");
    fprintf(code_id, "int    %s_PI_max_msg_size = 0;\n\n\n", fv->name);

    //Create the code for calling of the RI depending on the recieved messages
    create_incomming_data_pooler(fv->name);

    //Create the code to be called by the GUI designer to get infos from VM side
    create_outcomming_data_sender(fv);

    //Create the code to be called by the Virtual Machine when the PI are called
    create_outcomming_data_sender_reader(fv);

    //Finalize header code file
    fprintf(header_id, "\n\n#endif\n");
    fclose(header_id);

    //Create the queues management code for exchanging data with the outside
    create_gui_fv_queues(fv->name);

    //finalize code file
    fclose(code_id);
}



/*
 * Function to process one interface of the FV
 */
void GLUE_GUI_Interface(Interface * i)
{
    switch (i->direction) {
    case PI:
    add_PI_to_gui_glue(i);
    break;
    case RI:
    add_RI_to_gui_glue(i);
    break;
    default:
    break;
    }
}



/*
 * External interface (the one and unique)
 */
void GLUE_GUI_Backend(FV * fv)
{


    if (fv->system_ast->context->onlycv)
    return;

    if (gui == fv->language) {
        create_enum_file(fv);

        build_string(&cyclic_name, "gui_polling_", strlen("gui_polling_"));
        build_string(&cyclic_name, fv->name, strlen(fv->name));

        /* count the number of PI and RIs for this FV */
        PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE = 0;
        RI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE = 0;
        FOREACH(i, Interface, fv->interfaces, {
            if (PI == i->direction) PI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE ++;
            else RI_CURRENT_MAX_NUMBER_OF_PARAMS_IN_MESSAGE ++;
        })

        Init_GUI_Backend(fv);
        FOREACH(i, Interface, fv->interfaces, {
            GLUE_GUI_Interface(i);
        })

        End_GUI_Glue_Backend(fv);
        if (NULL != cyclic_name) {
            free(cyclic_name);
            cyclic_name = NULL;
        }
    }
}
