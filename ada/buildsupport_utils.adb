--  *************************** buildsupport ****************************  --
--  (c) 2008-2017 European Space Agency - maxime.perrotin@esa.int
--  LGPL license, see LICENSE file

with Ada.Text_IO;
with GNAT.OS_Lib;

with Buildsupport_Version;
--  with Ocarina.Namet;
with Ocarina.Configuration;
with Ocarina.AADL_Values;
with Ocarina.Instances.Queries;
--  with Ocarina.ME_AADL.AADL_Tree.Nodes;
--  with Ocarina.ME_AADL.AADL_Instances.Nodes;
with Ocarina.ME_AADL.AADL_Instances.Nutils;
with Ada.Characters.Latin_1;

package body Buildsupport_Utils is

   use Ada.Text_IO;
   use GNAT.OS_Lib;

--   use Ocarina.Namet;
   use Ocarina.Instances.Queries;
--   use Ocarina.ME_AADL.AADL_Instances.Nodes;
   use Ocarina.ME_AADL.AADL_Instances.Nutils;
   use Ada.Characters.Latin_1;
--  package ATN renames Ocarina.ME_AADL.AADL_Tree.Nodes;
--  package AIN renames Ocarina.ME_AADL.AADL_Instances.Nodes;
--   use type ATN.Node_Kind;

   ------------
   -- Banner --
   ------------

   procedure Banner is
      The_Banner : constant String :=
        "TASTE Buildsupport (Version "
        & Buildsupport_Version.Buildsupport_Release & ") "
        & ASCII.LF & ASCII.CR
        & "Contact: Maxime.Perrotin@esa.int or Thanassis.Tsiodras@esa.int"
        & ASCII.LF & ASCII.CR
        & "Based on Ocarina: " & Ocarina.Configuration.Ocarina_Version;
   begin
      Put_Line (The_Banner);
   end Banner;

   -----------
   -- Usage --
   -----------

   procedure Usage is
   begin
      Put_Line
        ("Usage: buildsupport <options> otherfiles");
      Put_Line
        ("Where <options> are:");
      New_Line;
      Put ("-l, --glue" & HT & HT & HT & HT);
      Put_Line ("Generate glue code");
      Put ("-w, --gw" & HT & HT & HT & HT);
      Put_Line ("Generate code skeletons");
      Put ("-j, --keep-case" & HT & HT & HT & HT);
      Put_Line ("Respect the case for interface names");
      Put ("-o, --output <outputDir>" & HT & HT);
      Put_Line ("Root directory for the output files");
      Put ("-i, --interfaceview <i_view.aadl>" & HT);
      Put_Line ("The interface view in AADL");
      Put ("-c, --deploymentview <d_view.aadl>" & HT);
      Put_Line ("The deployment view in AADL");
      Put ("-d, --dataview <dataview.aadl>" & HT & HT);
      Put_Line ("The data view in AADL");
      Put ("-t, --test" & HT & HT & HT & HT);
      Put_Line ("Dump model information");
      Put ("-g, --debug" & HT & HT & HT & HT);
      Put_Line ("Generate runtime debug output");
      Put ("-s, --stack <stack-value>" & HT & HT);
      Put_Line ("Set the size of the stack per thread in kbytes (default 50)");
      Put ("-x, --timer <timer-resolution in ms>" & HT);
      Put_Line ("Set the timer resolution (default 100 ms)");
      Put ("-v, --version" & HT & HT & HT & HT);
      Put_Line ("Display buildsupport version number");
      Put ("-p, --polyorb-hi-c" & HT & HT & HT);
      Put_Line ("Interface glue code with PolyORB-HI-C");
      Put ("otherfiles" & HT & HT & HT & HT);
      Put_Line ("Any other aadl file you want to parse");
      New_Line;
      New_Line;
      Put_Line ("For example, this command will generate your application"
       & " skeletons:");
      New_Line;
      Put_Line ("buildsupport -i InterfaceView.aadl -d DataView.aadl"
       & " -o code --gw --keep-case");
      New_Line;

   end Usage;

   -------------------
   -- Exit_On_Error --
   -------------------

   procedure Exit_On_Error (Error : Boolean; Reason : String) is
   begin
      if Error then
         Put_Line (Reason);
         --  Imported_Routines.C_End;
         OS_Exit (1);
      end if;
   end Exit_On_Error;

   ----------------------------
   -- Get_RCM_Operation_Kind --
   ----------------------------

   function Get_RCM_Operation_Kind
     (E : Node_Id) return Supported_RCM_Operation_Kind
   is
      RCM_Operation_Kind_N : Name_Id;
      RCM_Operation_Kind : constant Name_Id :=
          Get_String_Name ("taste::rcmoperationkind");
      Unprotected_Name   : constant Name_Id := Get_String_Name ("unprotected");
      Protected_Name     : constant Name_Id := Get_String_Name ("protected");
      Cyclic_Name        : constant Name_Id := Get_String_Name ("cyclic");
      Sporadic_Name      : constant Name_Id := Get_String_Name ("sporadic");
   begin
      if Is_Defined_Enumeration_Property (E, RCM_Operation_Kind) then
         RCM_Operation_Kind_N
           := Get_Enumeration_Property (E, RCM_Operation_Kind);

         if RCM_Operation_Kind_N = Unprotected_Name then
            return Unprotected_Operation;

         elsif RCM_Operation_Kind_N =  Protected_Name then
            return Protected_Operation;

         elsif RCM_Operation_Kind_N =  Cyclic_Name then
            return Cyclic_Operation;

         elsif RCM_Operation_Kind_N =  Sporadic_Name then
            return Sporadic_Operation;
         end if;
      end if;
      Exit_On_Error (True, "Could not determine interface kind");
      return Sporadic_Operation;
   end Get_RCM_Operation_Kind;

   -----------------------
   -- Get_RCM_Operation --
   -----------------------

   function Get_RCM_Operation (E : Node_Id) return Node_Id is
      RCM_Operation : constant Name_Id :=
          Get_String_Name ("taste::rcmoperation");
   begin
      if Is_Subprogram_Access (E) then
         return Corresponding_Instance (E);
      else
         if Is_Defined_Property (E, RCM_Operation) then
            return Get_Classifier_Property (E, RCM_Operation);
         else
            return No_Node;
         end if;
      end if;
   end Get_RCM_Operation;

   -----------------------
   -- Get_APLC_Binding --
   -----------------------

   function Get_APLC_Binding (E : Node_Id) return List_Id is
      APLC_Binding : constant Name_Id :=
          Get_String_Name ("taste::aplc_binding");
   begin
      if Is_Defined_Property (E, APLC_Binding) then
         return Get_List_Property (E, APLC_Binding);
      else
         return No_List;
      end if;
   end Get_APLC_Binding;

   --------------------
   -- Get_RCM_Period --
   --------------------

   function Get_RCM_Period (D : Node_Id) return Unsigned_Long_Long is
      RCM_Period : constant Name_Id := Get_String_Name ("taste::rcmperiod");
   begin
      if Is_Defined_Integer_Property (D, RCM_Period) then
         return Get_Integer_Property (D, RCM_Period);
      else
         return 0;
      end if;
   end Get_RCM_Period;

   --------------------------
   -- Get_Ada_Package_Name --
   --------------------------

   function Get_Ada_Package_Name (D : Node_Id) return Name_Id is
      Ada_Package_Name : constant Name_id :=
         Get_String_Name ("taste::ada_package_name");
   begin
      return Get_String_Property (D, Ada_Package_Name);
   end Get_Ada_Package_Name;

   -------------------------------
   -- Get_Ellidiss_Tool_Version --
   -------------------------------

   function Get_Ellidiss_Tool_Version (D : Node_Id) return Name_Id is
      Ellidiss_Tool_Version : constant Name_id :=
         Get_String_Name ("taste::version");
   begin
      return Get_String_Property (D, Ellidiss_Tool_Version);
   end Get_Ellidiss_Tool_Version;

   ------------------------
   -- Get_Interface_Name --
   ------------------------

   function Get_Interface_Name (D : Node_Id) return Name_Id is
      Interface_Name : constant Name_id :=
         Get_String_Name ("taste::interfacename");
   begin
      return Get_String_Property (D, Interface_Name);
   end Get_Interface_Name;

   ---------------------------
   -- Get ASN.1 Module name --
   ---------------------------

   function Get_ASN1_Module_Name (D : Node_Id) return String is
      id : Name_Id := No_Name;
      ASN1_Module : constant Name_id :=
         Get_String_Name ("deployment::asn1_module_name");
   begin
      if Is_Defined_String_Property (D, ASN1_Module) then
         id := Get_String_Property (D, ASN1_Module);
         return Get_Name_String (id);
      else
         return Get_Name_String (Get_String_Name ("nomodule"));
      end if;
   end Get_ASN1_Module_Name;

   --------------------------------------------
   -- Get all properties as a Map Key/String --
   -- Input parameter is an AADL instance    --
   --------------------------------------------
   function Get_Properties_Map (D : Node_Id) return Property_Maps.Map is
      properties : constant List_Id  := AIN.Properties (D);
      result     : Property_Maps.Map := Empty_Map;
      property   : Node_Id           := AIN.First_Node (properties);
      prop_value : Node_Id;
      single_val : Node_Id;
   begin
      while Present (property) loop
         prop_value := AIN.Property_Association_Value (property);
         if Present (ATN.Single_Value (prop_value)) then
            --  Only support single-value properties for now
            single_val := ATN.Single_Value (prop_value);
            result.Insert (Key => AIN_Case (property),
                        New_Item =>
              (case ATN.Kind (single_val) is
                 when ATN.K_Signed_AADLNumber =>
                   Ocarina.AADL_Values.Image
                      (ATN.Value (ATN.Number_Value (single_val))) &
                      (if Present (ATN.Unit_Identifier (single_val)) then " " &
                      Get_Name_String
                          (ATN.Display_Name (ATN.Unit_Identifier (single_val)))
                      else ""),
                 when ATN.K_Literal =>
                    Ocarina.AADL_Values.Image (ATN.Value (single_val),
                                               Quoted => False),
                 when ATN.K_Reference_Term =>
                    Get_Name_String
                       (ATN.Display_Name (ATN.First_Node --  XXX must iterate
                          (ATN.List_Items (ATN.Reference_Term (single_val))))),
                 when ATN.K_Enumeration_Term =>
                    Get_Name_String
                       (ATN.Display_Name (ATN.Identifier (single_val))),
                 when ATN.K_Number_Range_Term =>
                    "RANGE NOT SUPPORTED!",
                 when others => "ERROR! Unsupported kind: "
                                & ATN.Kind (single_val)'Img));
         end if;
         property := AIN.Next_Node (property);
      end loop;
      return result;
   end Get_Properties_Map;

   -----------------------
   -- Get_ASN1_Encoding --
   -----------------------

   function Get_ASN1_Encoding (E : Node_Id) return Supported_ASN1_Encoding is
      ASN1_Encoding_N : Name_Id;
      ASN1_Encoding : constant Name_Id := Get_String_Name ("taste::encoding");
      Native_Name   : constant Name_Id := Get_String_Name ("native");
      UPER_Name     : constant Name_Id := Get_String_Name ("uper");
      ACN_Name      : constant Name_Id := Get_String_Name ("acn");
   begin
      if Is_Defined_Enumeration_Property (E, ASN1_Encoding) then
         ASN1_Encoding_N := Get_Enumeration_Property (E, ASN1_Encoding);

         if ASN1_Encoding_N = Native_Name then
            return Native;

         elsif ASN1_Encoding_N = UPER_Name then
            return UPER;

         elsif ASN1_Encoding_N = ACN_Name then
            return ACN;
         end if;
      end if;
      Exit_On_Error (True, "ASN1 Encoding not set");
      return Default;
   end Get_ASN1_Encoding;

   -------------------------
   -- Get_ASN1_Basic_Type --
   -------------------------

   function Get_ASN1_Basic_Type (E : Node_Id) return Supported_ASN1_Basic_Type
   is
      ASN1_Basic_Type  : constant Name_Id :=
                               Get_String_Name ("taste::asn1_basic_type");
      Sequence_Name    : constant Name_Id := Get_String_Name ("asequence");
      SequenceOf_Name  : constant Name_Id := Get_String_Name ("asequenceof");
      Enumerated_Name  : constant Name_Id := Get_String_Name ("aenumerated");
      Set_Name         : constant Name_Id := Get_String_Name ("aset");
      SetOf_Name       : constant Name_Id := Get_String_Name ("asetof");
      Integer_Name     : constant Name_Id := Get_String_Name ("ainteger");
      Boolean_Name     : constant Name_Id := Get_String_Name ("aboolean");
      Real_Name        : constant Name_Id := Get_String_Name ("areal");
      OctetString_Name : constant Name_Id := Get_String_Name ("aoctetstring");
      Choice_Name      : constant Name_Id := Get_String_Name ("achoice");
      String_Name      : constant Name_Id := Get_String_Name ("astring");
      ASN1_Basic_Type_N : Name_Id;
   begin
      if Is_Defined_Enumeration_Property (E, ASN1_Basic_Type) then
         ASN1_Basic_Type_N := Get_Enumeration_Property (E, ASN1_Basic_Type);

         if ASN1_Basic_Type_N = Sequence_Name then
            return ASN1_Sequence;

         elsif ASN1_Basic_Type_N = SequenceOf_Name then
            return ASN1_SequenceOf;

         elsif ASN1_Basic_Type_N = Enumerated_Name then
            return ASN1_Enumerated;

         elsif ASN1_Basic_Type_N = Set_Name then
            return ASN1_Set;

         elsif ASN1_Basic_Type_N = SetOf_Name then
            return ASN1_SetOf;

         elsif ASN1_Basic_Type_N = Integer_Name then
            return ASN1_Integer;

         elsif ASN1_Basic_Type_N = Boolean_Name then
            return ASN1_Boolean;

         elsif ASN1_Basic_Type_N = Real_Name then
            return ASN1_Real;

         elsif ASN1_Basic_Type_N = OctetString_Name then
            return ASN1_OctetString;

         elsif ASN1_Basic_Type_N = Choice_Name then
            return ASN1_Choice;

         elsif ASN1_Basic_Type_N = String_Name then
            return ASN1_String;

         else
            raise Program_Error with "Undefined choice "
              & Get_Name_String (ASN1_Basic_Type_N);
         end if;
      end if;
      Exit_On_Error (True, "Error: ASN.1 Basic type undefined!");
      return ASN1_Unknown;
   end Get_ASN1_Basic_Type;

   function AADL_to_Ada_IV (System : Node_Id) return Complete_Interface_View is
      pragma Unreferenced (System);
      Funcs  : Functions.Vector;
      Routes : Channels.Vector;
   begin
      return IV_AST : constant Complete_Interface_View :=
          (Flat_Functions => Funcs,
           Connections    => Routes);
   end AADL_to_Ada_IV;

end Buildsupport_Utils;
