.META PROG

/* THIS RULE DEFINES SYNTAX OF COMPLETE PROGRAM */
PROG  =  'BEGIN' :BEG[0] *
         DECLN *
         STMT * $ ( ';' STMT ?1? * )
         'END' :ENDS[0] * ;
DECLN =  'NEW' DEC $ ( ',' DEC :DOO[2] ) ?2? ';' ?5? :DECS[1] ;
DEC   =  .ID :DECID[1] ;

/* DEFINE STATEMENT TYPES */
STMT = BLOCK / IFST / .ID ':=' AEXP :STORE[2] ;
BLOCK = 'BEGIN' STMT $ ( ';' STMT :DOO[2] ) 'END' ;
IFST = 'IF' LEXP 'THEN' STMT ( 'ELSE' STMT :IFF[3] / .EMPTY :IFF[2] ) ;
LEXP = AEXP ( '=' AEXP :EQQ / '#' AEXP :NEQ ) [2] ;
AEXP = FACTOR $ ( '+' FACTOR :ADD[2] / '-' FACTOR :SUB[2] );
FACTOR = '-' PRIME :MINUSS[1] / PRIME ;
PRIME =  .ID / .NUM / '(' AEXP ')' ?3? ;

/* OUTPUT RULES */

BEG[] => % < A<-0 > ;
ENDS[] => % 'END' % ;
DECS[-] => 'GOTO #1' % *1 #1 ':' % ;
DOO[-,-] => *1 *2 ;
DECID[-] => *1 ':DATA(0)' % ;
STORE[-,-] => GET[*2] 'STORE ' *1 % ;
GET[.ID] => 'LOAD ' *1 %
   [.NUM] => 'LOADI ' *1 %
   [MINUSS[.NUM]] => 'LOADN ' *1:*1 %
   [-] => *1 ;
ADD[-,-] =>  SIMP[*2] GET[*1] 'ADD' VAL[*2] % /
             SIMP[*1] GET[*2] 'ADD' VAL[*1] % /
             GET[*1] 'STORE T+' < OUT[A] ; A<-A+1 > %
             GET[*2] 'ADD T+' < A<-A-1 ; OUT[A] > % ;

SUB[-,-]  => SIMP[*2] GET[*1] 'SUB' VAL[*2] % /
             SIMP[*1] GET[*2] 'NEGATE' % 'ADD' VAL[*1] % /
             GET[*2] 'STORE T+' < OUT[A] ; A<-A+1 > %
             GET[*1] 'SUB T+' < A<-A-1 ; OUT[A] > % ;

SIMP[.ID]           => .EMPTY
    [.NUM]          => .EMPTY
    [MINUSS[.NUM]]  => .EMPTY ;

VAL[.ID]             => ' ' *1
   [.NUM]            => 'I ' *1
   [MINUSS[.NUM]]    => 'N ' *1:*1 ;

IFF[-,-]             => BF[*1,#1] *2 #1 ':' %
   [-,-,-]           => BF[*1,#1] *2 'GOTO ' #2 % #1 ':' %
                        *3 #2 ':' % ;

BF[EQQ[-,-],#1]      => SIMP[*1:*1] GET[*1:*2] 'COMPEQ' VAL[*1:*1] %
                        'BRANCHF ' #1 % /
                        SIMP[*1:*2] GET[*1:*1] 'COMPEQ' VAL[*1:*2] %
                        'BRANCHF ' #1 % /
                        SUB[*1:*1 ,*1:*2] 'COMPEQ 0' %
                        'BRANCHF ' #1 %
  [NEQ[-,-],#1]      => SUB[*1:*1,*1:*2] 'COMPNEI 0' %
                        'BRANCHF ' #1 % ;
EQQ[-,-]             => .EMPTY ;
NEQ[-,-]             => .EMPTY ;
MINUSS[-]            => GET[*1] 'NEGATE' % ;
.END
