#!env sed -f
# syntactically translates darwin-ppc assembly to GNU/linux assembly
# GNU sed required
s|\<r\([0-9]\+\)\>|\1|g
s|\<ha16\>\(([^(]*)\)|\1@ha|g
s|\<lo16\>\(([^(]*)\)|\1@l|g
# s|\<L[A-Za-z0-9.$_]\+\>|.&|g
