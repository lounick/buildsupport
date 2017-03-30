--  *************************** buildsupport ****************************  --
--  (c) 2008-2017 European Space Agency - maxime.perrotin@esa.int
--  LGPL license, see LICENSE file
--  Define a generic Option type
generic
   type T is private;
package Option_Type is
   pragma Assertion_Policy (check);
   type Option (Present : Boolean) is tagged private;
   function Just (I : T) return Option;
   function Nothing return Option;
   function Just (O : Option) return T
      with Pre => O.Present;
   function Value (O : Option) return T renames Just;
   function Value_Or (O : Option; Default : T) return T;
   function Has_Value (O : Option) return Boolean is (O.Present);

private

   type Option (Present : Boolean) is tagged
      record
          case Present is
              when True =>
                 Value : T;
              when False =>
                 null;
          end case;
      end record;

   function Just (I : T) return Option is
       (Present => True, Value => I);

   function Nothing return Option is
       (Present => False);

   function Just (O : Option) return T is (O.Value);

   function Value_Or (O : Option; Default : T) return T is
       (if O.Present then O.Value else Default);

end Option_Type;
