group taste;

interface_view(arrsFunctNames, arrsConnections) ::= <<
# TASTE Interface View
# Save this output to file.dot and run "neato file.dot -Tpng -o out.png"

digraph interfaceview {
<arrsFunctNames: {each|    <each> [shape=box, label="<each>"];}; separator="\n">

<arrsConnections; separator="\n">
}
>>

connection(sFrom, sTo, arrsMessages) ::= <<
    <sFrom> -> <sTo> [label="[<arrsMessages; separator=",\n">]"];
>>
