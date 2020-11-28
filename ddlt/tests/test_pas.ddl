// Line comment

(* Block comment *)

{ Block comment
spawning multiple
lines }

{$line test_cpp.ddl 9}

(*$ifndef DEBUG*)
  (*$define assert(x) do { x; } while (0)*){$define NDEBUG}
(*$endif*)

[{
  Free
  form
  block

  [{ Nested freeform block }]
}]

id
Id
iD
ID
_id
id_
_

0
$abcd
$ABCD
&123
%01
123
1.1
1e1
1.1e1
''
'a'
'ab'
#48
#48''
''#48
''#48''
'a'#48
#48'a'
'a'#48'b'
#48#49
#48#49''
''#48#49
''#48#49''
'a'#48#49
#48#49'a'
'a'#48#49'b'
