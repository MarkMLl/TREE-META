.META EXP
EXP = .ID $ ( '+' ( .ID / .NUM ) :ADD[2] ) ';' :EY[1] * ;
EY[-] => DEC[*1] *1 % ;

DEC[ADD[-,-]] => DEC [*1:*1] DEC[*1:*2]
    [.NUM]    => .EMPTY
    [.ID]     => 'INTEGER ' *1 % ;

ADD[-,-] => *1 ' PLUS ' *2 ;
.END
