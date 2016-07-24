group taste;

interface_view(arrsFunctNames, arrsConnections) ::= <<
# TASTE Interface View
# Save this output to file.dot and run "neato file.dot -Tpng -o out.png"

digraph interfaceview {
    overlap=scale;
    splines=ortho;
    sep=0.2;
<arrsFunctNames: {each|    <each> [shape=box, style=filled, fontsize=36, margin=0.8 label="<each>"];}; separator="\n">

<arrsConnections; separator="\n">
}
>>

connection(sFrom, sTo, arrsMessages) ::= <<
    <sFrom> -> <sTo> [fontsize=20, color=blue, label="[<arrsMessages; separator=",\n">]"];
>>
