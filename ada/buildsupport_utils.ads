--  *************************** buildsupport ****************************  --
--  (c) 2017 European Space Agency - maxime.perrotin@esa.int
--  LGPL license, see LICENSE file

--  Set of helper functions for buildsupport
with Ocarina,
     --  Ocarina.Types,
     Types,
     Namet,
     Ocarina.Backends.Properties,
     Ada.Containers.Indefinite_Ordered_Maps,
     Ada.Containers.Indefinite_Vectors,
     Ocarina.ME_AADL.AADL_Tree.Nodes,
     Ocarina.ME_AADL.AADL_Instances.Nodes,
     Ada.Strings.Unbounded,
     Option_Type;

use Ocarina,
    Types,
    Namet,
    Ocarina.Backends.Properties,
    Ocarina.ME_AADL.AADL_Tree.Nodes,
    Ocarina.ME_AADL.AADL_Instances.Nodes,
    Ada.Containers,
    Ada.Strings.Unbounded;

package Buildsupport_Utils is

   package ATN renames Ocarina.ME_AADL.AADL_Tree.Nodes;
   package AIN renames Ocarina.ME_AADL.AADL_Instances.Nodes;
   function US (Source : String) return Unbounded_String renames
       To_Unbounded_String;

   procedure Banner;

   procedure Usage;

   procedure Exit_On_Error (Error : Boolean; Reason : String);

   type Synchronism is (Sync, Async);

   type Supported_RCM_Operation_Kind is (Unprotected_Operation,
                                         Protected_Operation,
                                         Cyclic_Operation,
                                         Sporadic_Operation);

   function Get_RCM_Operation_Kind (E : Node_Id)
     return Supported_RCM_Operation_Kind;

   function Get_RCM_Operation (E : Node_Id) return Node_Id;

   function Get_RCM_Period (D : Node_Id) return Unsigned_Long_Long;

   function Get_APLC_Binding (E : Node_Id) return List_Id;

   type Supported_ASN1_Encoding is (Default, Native, UPER, ACN);

   function Get_ASN1_Encoding (E : Node_Id) return Supported_ASN1_Encoding;

   type Supported_ASN1_Basic_Type is (ASN1_Sequence,
                                      ASN1_SequenceOf,
                                      ASN1_Enumerated,
                                      ASN1_Set,
                                      ASN1_SetOf,
                                      ASN1_Integer,
                                      ASN1_Boolean,
                                      ASN1_Real,
                                      ASN1_OctetString,
                                      ASN1_Choice,
                                      ASN1_String,
                                      ASN1_Unknown);

   package Property_Maps is new Indefinite_Ordered_Maps (String, String);
   use Property_Maps;
   package String_Vectors is new Indefinite_Vectors (Natural, String);

   function Get_ASN1_Basic_Type (E : Node_Id) return Supported_ASN1_Basic_Type;

   function Get_Ada_Package_Name (D : Node_Id) return Name_Id;

   function Get_Ellidiss_Tool_Version (D : Node_Id) return Name_Id;

   function Get_Interface_Name (D : Node_Id) return Name_Id;

   function Get_ASN1_Module_Name (D : Node_Id) return String;

   function Get_Properties_Map (D : Node_Id) return Property_Maps.Map;

   --  Shortcut to read an identifier from the parser, in lowercase
   function ATN_Lower (N : Node_Id) return String is
       (Get_Name_String (ATN.Name (ATN.Identifier (N))));

   --  Shortcut to read an identifier from the parser, with original case
   function ATN_Case (N : Node_Id) return String is
       (Get_Name_String (ATN.Display_Name (ATN.Identifier (N))));

   --  Shortcut to read an identifier from the parser, in lowercase
   function AIN_Lower (N : Node_Id) return String is
       (Get_Name_String (AIN.Name (AIN.Identifier (N))));

   --  Shortcut to read an identifier from the parser, with original case
   function AIN_Case (N : Node_Id) return String is
       (Get_Name_String (AIN.Display_Name (AIN.Identifier (N))));

   --  Types needed to build the AST of the TASTE Interface View in Ada
   type Parameter_Direction is (param_in, param_out);

   package Option_UString is new Option_Type (Unbounded_String);
   use Option_UString;

   type ASN1_Parameter is
       record
           Name            : Unbounded_String;
           Sort            : Unbounded_String;
           ASN1_Module     : Unbounded_String;
           ASN1_Basic_Type : Supported_ASN1_Basic_Type;
           ASN1_File_Name  : Unbounded_String;
           Encoding        : Supported_ASN1_Encoding;
           Direction       : Parameter_Direction;
       end record;

   package Parameters is new Indefinite_Vectors (Natural, ASN1_Parameter);

   type Taste_Interface is
       record
           Name            : Unbounded_String;
           In_Parameters   : Parameters.Vector;
           Out_Parameters  : Parameters.Vector;
           RCM             : Supported_RCM_Operation_Kind;
           Period_Or_MIAT  : Natural;
           WCET            : Natural;
           WCET_Unit       : Unbounded_String;
           Queue_Size      : Natural;
           User_Properties : Property_Maps.Map;
       end record;

   package Interfaces is new Indefinite_Vectors (Natural, Taste_Interface);

   type Context_Parameter is
       record
           Name           : Unbounded_String;
           Sort           : Unbounded_String;
           Default_Value  : Unbounded_String;
           ASN1_Module    : Unbounded_String;
           ASN1_File_Name : Option;
       end record;

   package Ctxt_Params is new Indefinite_Vectors (Natural, Context_Parameter);

   type Taste_Terminal_Function is
       record
           Name            : Unbounded_String;
           Language        : Supported_Source_Language;
           Zip_File        : Option := Nothing;
           Context_Params  : Ctxt_Params.Vector;
           User_Properties : Property_Maps.Map;
           Timers          : String_Vectors.Vector;
           Provided        : Interfaces.Vector;
           Required        : Interfaces.Vector;
       end record;

   package Functions is new Indefinite_Vectors (Natural,
                                                Taste_Terminal_Function);

   type Connection is
       record
           Source_Function : Taste_Terminal_Function;
           Dest_Function   : Taste_Terminal_Function;
           Source_RI       : Taste_Interface;
           Dest_PI         : Taste_Interface;
       end record;

   package Channels is new Indefinite_Vectors (Natural, Connection);

   type Complete_Interface_View is
       record
           Flat_Functions : Functions.Vector;
           Connections    : Channels.Vector;
       end record;

   --  Function to build up the Ada AST by transforming the one from Ocarina
   function AADL_to_Ada_IV (System : Node_Id) return Complete_Interface_View;

end Buildsupport_Utils;
