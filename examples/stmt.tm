.META PROG
PROG      = ST * $ ( ';' ST * ) '.END' ;
ST        = IFST / ASGNST ;
ASGNST    = .ID '=' .ID :ASGN [2] ;
IFST      = '.IF' .ID '>= 0' '.THEN' ST '.ELSE' ST :IFF[3] ;

ASGN[-,-]   => 'LOAD ' *2 ' ;' ' STORE ' *1 % ;
IFF[ -,-,-] => 'LOAD ' *1 ' ;' ' BRNEG ' #1 % *2
               'BR ' #2 % #1 ':' % *3 #2 ':' % ;
.END
