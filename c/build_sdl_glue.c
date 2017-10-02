/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* build_sdl_glue.c

  this program generates the code to interface an objectgeode generated application with the assert virtual machine
  It creates vm_if.c, hpredef.h and hpostdef.h
  
  updated 20/04/2009 to disable in case "-onlycv" flag is set

 */

#define ID "$Id: build_sdl_glue.c 414 2009-12-04 16:21:52Z maxime1008 $"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "my_types.h"
#include "practical_functions.h"
 static FILE *vm_if, *hpredef, *hpostdef;

#define PARAM_TYPE(p) (p->value->type)
#define PARAM_NAME(p) (p->value->name)


/* create_gluefiles()
**
** Open/Creates glue files (vm_if, hpredef, hpostdef)
** Returns -1 if problem opening one of the files.
*/
int create_gluefiles(FV * fv)
{
    char *path = NULL;
    if (NULL != fv->system_ast->context->output)
	build_string(&path, fv->system_ast->context->output,
		      strlen(fv->system_ast->context->output));
    build_string(&path, fv->name, strlen(fv->name));
    create_file(path, "vm_if.c", &vm_if);
    create_file(path, "hpredef.h", &hpredef);
    create_file(path, "hpostdef.h", &hpostdef);
    free(path);
    if (NULL == vm_if || NULL == hpredef || NULL == hpostdef)
	return -1;

    else
	return 0;
}

void close_gluefiles()
{
    close_file(&vm_if);
    close_file(&hpredef);
    close_file(&hpostdef);
} void vm_if_preamble(FV * fv)
{
    if (NULL == vm_if)
	return;
    fprintf(vm_if,
	      "#include \"g2_appli.h\"\n#include \"g2_com.h\"\n#include \"s_%s.h\"\n",
	      fv->name);
    fprintf(vm_if, "#include \"OG_ASN1_Types.h\"\n\n\n");
}

void add_PI_to_vm_if(Interface * i)
{
    if (NULL == vm_if)
	return;

    /* Create glue for the PI */

    // a. If this PI has a parameter, declare an external global variable of type SIGNAL_signalname * for the data. Else of type GX_SIG_P
    if (NULL != i->in) {
	fprintf(vm_if, "extern SIGNAL_%s *p_%s;\n\n", (i->name),
		 (i->name));
    }

    else {
	fprintf(vm_if, "extern GX_SIG_P p_%s;\n\n", (i->name));
    }

    // b. function name and preamble (declaration of g2x_self used in OG macros)
    fprintf(vm_if, "void %s_%s(%s) {\n\n\tint g2x_self=0;\n\n",
	     (i->parent_fv->name),
	     (i->name),
	     (NULL != i->in) ? "char *param, int length" : "" );

    // c. if the PI has NO PARAMETER, only make a simple output of the signal corresponding to the PI. Use G2S_OUTPUT and not SDL_OUTPUT (which makes malloc)
    if (NULL == i->in) {
	fprintf(vm_if, "\t// Send the message to the SDL model\n");
	fprintf(vm_if, "\tG2S_OUTPUT(sig_%s,(G2P_FINDPID(%s)),(GX_SIG_P)p_%s);\n\n", (i->name), (i->parent_fv->name),	// SDL_DEST
		 (i->name) );
    }
    
	// d. if the PI has a parameter
	// generate 3 lines:
	// G2M_INIT_REC (p_signalname, SIGNAL_signalname);
	// DECODE_paramtype(...): call to Thanassis macros
	// G2S_OUTPUT(sig_signalname,(G2P_FINDPID(nodename)),(GX_SIG_P)p_signalname);
	else {
	fprintf(vm_if,
		 "\t//1. Clean the global variable before putting the decoded data\n");
	fprintf(vm_if, "\tG2M_INIT_REC (p_%s, SIGNAL_%s);\n\n",
		 (i->name), (i->name) );

	// call to Thanassis Decoding macro (in the future replace by a loop: there can be several params)
	fprintf(vm_if,
		"\t//2. Decode the ASN.1 binary stream and put it in the SDL variable\n");
	fprintf(vm_if, "\tDECODE_%s_%s(param, length, &p_%s->sg1_%s)\n\n",
		 BINARY_ENCODING(i->in->value),
		 /*(PARAM_ENCODING(i->in)),*/
		 (PARAM_TYPE(i->in)),
		 (i->name), 
		 (PARAM_TYPE(i->in)) 
	);
	fprintf(vm_if, "\t//3. Send the message to the SDL model\n");
	fprintf(vm_if, "\tG2S_OUTPUT(sig_%s,(G2P_FINDPID(%s)),(GX_SIG_P)p_%s);\n\n", (i->name), (i->parent_fv->name),	// SDL_DEST
		 (i->name) );
    }

    // e. function postamble (call to sdl_loop() and return)
    fprintf(vm_if, "\tsdl_loop_%s();\n}\n\n", (i->parent_fv->name));
}

void add_PI_to_hpostdef(Interface * i)
{
    if (NULL == hpostdef)
	return;

    /* Declaration for PI with parameters: global variable + undef of the SDL_OUTPUT not used anymore */
    fprintf(hpostdef, "// PROVIDED INTERFACE %s\n\n", i->name);

    // a. global variables if there is a parameter: use the type SIGNAL_signalname
    if (NULL != i->in) {
	fprintf(hpostdef, "SIGNAL_%s %s;\nSIGNAL_%s *p_%s=&%s;\n\n",
		 (i->name),
		 (i->name), (i->name), (i->name), (i->name) );
    }
    // b. global variables if there is no parameter: use the types GX_SIG_T and GX_SIG_P
    else {
	fprintf(hpostdef, "GX_SIG_T %s;\nGX_SIG_P p_%s = &%s;\n\n",
		 (i->name), (i->name), (i->name) );
    }
    fprintf(hpostdef, "#undef SDL_OUTPUT_%s\n\n", (i->name));
}

void hpredef_preamble()
{
    if (NULL == hpredef)
	return;
    fprintf(hpredef,
	      "/* External declaration of VM callback function for each REQUIRED INTERFACE */\n\n");
}

void add_RI_to_hpredef(Interface * i)
{
    Parameter_list * v_local;
    int count_in_param = 0;
    int count_out_param = 0;
    int i_in = 0, i_out = 0;
    if (NULL == hpredef)
	return;

    /* Count IN and OUT param */
    v_local = i->in;
    while (v_local != NULL) {
	v_local = v_local->next;
	count_in_param++;
    }
    v_local = i->out;
    while (v_local != NULL) {
	v_local = v_local->next;
	count_out_param++;
    }

    /* Preamble (function declaration) */
    fprintf(hpredef, "extern void vm_%s_%s(",
	    (i->parent_fv->name), (i->name));

    /* For each IN param, add "char *, int" to the declaration */
    while (i_in < count_in_param) {
	fprintf(hpredef, "%schar *, int", (i_in > 0) ? ", " : "");
	i_in++;
    }

    /* For each OUT param, add "char *, int*" to the declaration */
    while (i_out < count_out_param) {
	fprintf(hpredef, "%schar *, int *",
		 (i_out > 0 || count_in_param > 0) ? ", " : "");
	i_out++;
    }

    /* Postamble */
    fprintf(hpredef, ");\n");
}

void hpostdef_preamble()
{
    if (NULL == hpostdef)
	return;
    fprintf(hpostdef, "#include \"OG_ASN1_Types.h\"\n\n\n");
}

void hpostdef_postamble()
{
    if (NULL == hpostdef)
	return;
    fprintf(hpostdef,
	      "/* ** Redefinition of the macro G2S_RELEASE that makes a free of the current message */\n");
    fprintf(hpostdef, "#undef G2S_RELEASE\n\n#define G2S_RELEASE()\n\n");
    fprintf(hpostdef,
	      "/* ** Redefinition of the macro G2I_CRE_START (call of malloc to send the START message at system startup) */\n");
    fprintf(hpostdef,
	     "#undef G2I_CRE_START\n\n#define G2I_CRE_START(proc) \\\n{GX_SIG_T sig; \\\nsig.sg_signo=sig_START;\\\nsig.sg_receiver=new_pid;\\\nCAT(PROCESS_, proc)(&sig); }");
}

void add_Immediate_RI_to_hpostdef(Interface * i)
{
    Parameter_list * v_local;
    int count_in_param = 0;
    int count_out_param = 0;
    int j;
    if (NULL == hpostdef)
	return;
    fprintf(hpostdef, "// IMMEDIATE REQUIRED INTERFACE %s\n\n", i->name);
    
	/* 1) for all parameters (in/out) declare encoding/decoding buffers: DEFINE_type(IN/OUT_RIname_varname) */
	// First IN param
	v_local = i->in;
    while (NULL != v_local) {
	fprintf(hpostdef, "DEFINE_%s(IN_%s_%s)\n\n",
		  (PARAM_TYPE(v_local)),
		  (i->name), (PARAM_NAME(v_local)) );

	/* In the meantime, also count the number of params */
	count_in_param++;
	v_local = v_local->next;
    }

    // Then OUT param
    v_local = i->out;
    while (NULL != v_local) {
	fprintf(hpostdef, "DEFINE_%s(OUT_%s_%s)\n\n",
		  (PARAM_TYPE(v_local)),
		  (i->name), (PARAM_NAME(v_local)) );

	/* In the meantime, also count the number of params */
	count_out_param++;
	v_local = v_local->next;
    }

    /* 2) Macro declaration:  "#define RI_NAME(...)" */
    fprintf(hpostdef, "#define %s(", (i->name));
    for (j = 0; j < count_in_param; j++) {
	if (j > 0)
	    fprintf(hpostdef, ", ");
	fprintf(hpostdef, "param_in_%d", j + 1);
    }
    if (count_in_param > 0 && count_out_param > 0)
	fprintf(hpostdef, ", ");
    for (j = 0; j < count_out_param; j++) {
	if (j > 0)
	    fprintf(hpostdef, ", ");
	fprintf(hpostdef, "param_out_%d", j + 1);
    }
    fprintf(hpostdef, ")\\\n{ ");

    /* 3) Inside the macro: add a call to ENCODE_SYNC macros for each IN param */
    /* Note: the SYNC postfix to ENCODE had to be added because OG treats differently the parameters of SIGNAL and CALL actions */
    v_local = i->in;
    j = 1;
    while (NULL != v_local) {
	fprintf(hpostdef, "ENCODE_SYNC_%s_%s(IN_%s_%s,param_in_%d)\\\n",
		   BINARY_ENCODING(v_local->value), /*(PARAM_ENCODING(v_local)), */
		   (PARAM_TYPE(v_local)),
		   (i->name), (PARAM_NAME(v_local)), j );
	j++;
	v_local = v_local->next;
    }
    
	/* 4a) Inside the macro: add the call to the VM interface (preamble) */
	fprintf(hpostdef, "vm_%s_%s(", (i->parent_fv->name), (i->name) );

    /* 4b) Add each IN param (IN_RIname_varname, IN_RIname_varname_len) */
    v_local = i->in;
    j = 0;
    while (NULL != v_local) {
	fprintf(hpostdef, "%sIN_%s_%s,IN_%s_%s_len",
		  (j > 0) ? ", " : "",
		  (i->name),
		  (PARAM_NAME(v_local)),
		  (i->name), (PARAM_NAME(v_local)) );
	j++;
	v_local = v_local->next;
    }
    if (count_in_param > 0 && count_out_param > 0)
	fprintf(hpostdef, ", ");

    /* 4c) Add each OUT param (OUT_RIname_varname, &OUT_RIname_varname_len) */
    v_local = i->out;
    j = 0;
    while (NULL != v_local) {
	fprintf(hpostdef, "%sOUT_%s_%s,&OUT_%s_%s_len",
		  (j > 0) ? ", " : "",
		  (i->name),
		  (PARAM_NAME(v_local)),
		  (i->name), (PARAM_NAME(v_local)) );
	j++;
	v_local = v_local->next;
    }
    fprintf(hpostdef, ");\\\n");

    /* 5) Inside the macro: add DECODE for each OUT param */
    if (count_out_param > 0) {
	v_local = i->out;
	j = 1;
	while (NULL != v_local) {
	    fprintf(hpostdef,
		      "DECODE_%s_%s(OUT_%s_%s,OUT_%s_%s_len,param_out_%d)\\\n",
		      BINARY_ENCODING(v_local->value), /*(PARAM_ENCODING(v_local)), */
		      (PARAM_TYPE(v_local)),
		      (i->name), (PARAM_NAME(v_local)), (i->name),
		      (PARAM_NAME(v_local)), j );
	    j++;
	    v_local = v_local->next;
	}
    }
    fprintf(hpostdef, "}\n\n");
}

void add_Deffered_RI_to_hpostdef(Interface * i)
{
    if (NULL == hpostdef)
	return;
    fprintf(hpostdef, "/* DEFFERED (ASYNCHRONOUS) REQUIRED INTERFACE %s */\n\n", i->name);

    // 1). call to Thanassis macro to declare encoding buffers:
    if (NULL != i->in) {
	fprintf(hpostdef, "DEFINE_%s(%s_sg1)\n\n",
		 (PARAM_TYPE(i->in)), (i->name) );
    }
    // 2) redefine SDL_OUTPUT
    fprintf(hpostdef, "#undef SDL_OUTPUT_%s\n\n", (i->name));
    if (NULL != i->in) {
	fprintf(hpostdef, "#define SDL_OUTPUT_%s(pid, param1)\\\n{ ",
		 (i->name));
	fprintf(hpostdef, "ENCODE_ASYNC_%s_%s(%s_sg1,param1)\\\n",
		  BINARY_ENCODING(i->in->value), /*(PARAM_ENCODING(i->in)),*/
		  (PARAM_TYPE(i->in)), (i->name));
	fprintf(hpostdef, "vm_async_%s_%s(%s_sg1,%s_sg1_len);}\n\n",
		  (i->parent_fv->name), (i->name), (i->name), (i->name));
    }

    else {
	fprintf(hpostdef,
		 "#define SDL_OUTPUT_%s(pid)\\\n{vm_async_%s_%s();}\n\n",
		 (i->name), (i->parent_fv->name), (i->name) );
    }
}

int Create_OG_files(FV * fv)
{
    if (create_gluefiles(fv) != -1) {
	vm_if_preamble(fv);
	hpredef_preamble();
	hpostdef_preamble();
    } else {
	printf("File creation error (Build_SDL_Glue backend)\n\n");
	return -1;
    }
    return 0;
}

void End_SDL_Glue_Backend()
{
    hpostdef_postamble();
    close_gluefiles();
} 

/* Function to process one interface of the FV */
void GLUE_OG_Interface(Interface * i)
{

    switch (i->direction) {
    case PI:
	add_PI_to_vm_if(i);
	add_PI_to_hpostdef(i);
	break;
    case RI:
	switch (i->synchronism) {
	case asynch:
	    add_Deffered_RI_to_hpostdef(i);
	    break;
	case synch:
	    add_Immediate_RI_to_hpostdef(i);
	    break;
	default : break;
	}
	break;
    default:
	break;
    }
}


// External interface (the one and unique)
void GLUE_OG_Backend(FV * fv)
{
    if (fv->system_ast->context->onlycv)
	return;
    if ((sdl == fv->language) && (false == fv->is_component_type)) {
	Create_OG_files(fv);
	FOREACH(i, Interface, fv->interfaces, {
	    GLUE_OG_Interface(i);
	})
	End_SDL_Glue_Backend();
    }
}
