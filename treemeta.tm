/*
 *
 * TREE-META COMPILER.
 *
 */

.META PROGRAM

PROGRAM = '.META' .ID :START[1] *
          $(RULE *)
          '.END' :ENDING[0] * ;
RULE = .ID ('=' ALTLIST :SRDEF[2]
           /'/' SIMPLEOUTRULELIST :SCRDEF[2]
           /':=' OUTEXPRESSION :SYMRDEF[2]
           /OUTRULELIST :CRDEF[2]) ';' ;

START[-] => 'CALL '*1 % ;
ENDING[] => ALLOCVARS[] ;

ALTLIST = ALTERNATIVE $('/' ALTERNATIVE :ALTER[2]) ;
ALTERNATIVE = '<-' FTEST (TEST $(TEST :DOO[2]) :SEQ[2] / .EMPTY) :BTALT[1]
            / FTEST (QTEST $(QTEST :DOO[2]) :SEQ[2] / .EMPTY) ;
FTEST = NTEST
      / STEST ;
TEST = ( NTEST
       / STEST ) :TESTT[1] ;
QTEST = NTEST :TESTT[1]
      / STEST ('?' (.NUM / .SR) '?' :ETESTT[2] / .EMPTY :TESTT[1]) ;
STEST = .SR             :TEXTTEST[1]
      / .ID             :SRCALL[1]
      / '@' .NUM        :CHRCODE[1]
      / '.EMPTY'        :EMPTY[0]
      / '(' ALTLIST ')'
      / '$' STEST       :REPET[1]
      / BASICTYPE       :BASICTYPEE[1]
      / '.' .SR         :TESTTEXTP[1] ;
BASICTYPE = .'.NUM'
          / .'.ID'
          / .'.OCT'
          / .'.HEX'
          / .'.SR'
          / .'.CHR'
          / .'.DIG'
          / .'.LET' ;
NTEST = ( ':' .ID ('[' .NUM ']' :ND[2] / .EMPTY :NDNAM[1])
        / '[' .NUM ']' :NDNBR[1]
        / '+' .SR :PSTR[1]
        / '*' ( '*' :DUMPTREE[0] / .EMPTY :GENCODE[0] ) ) :NTESTT[1] ;

SRDEF[-,-] => '%'*1':' %
              *2
              'R' % ;

ALTER[-,-] => *1
              'BT '#1 %
              *2
              '%'#1':' % ;

BTALT[-] => 'SBTP '#1 %
            *1
            '%'#1':' %
            'CBTP' % ;

SEQ[-,-] => *1
            'BF '#1 %
            *2
            '%'#1':' % ;
TESTT[-] => *1
            'ERCHK 0' % ;
ETESTT[-,.NUM] => *1
                  'ERCHK ' *2 %
      [-,.SR] => *1
                 'ERSTR ' OPSTR[*2] % ;

REPET[-] => '%'#1':' %
            *1
            'BT '#1 %
            'SET' % ;

TEXTTEST[-] => 'TST ' OPSTR[*1] % ;
TESTTEXTP[-] => 'SRP ' OPSTR[*1] % ;
SRCALL[-] => 'CALL ' *1 % ;
CHRCODE[-] => 'CHRCODE ' *1 % ;
BASICTYPEE['.NUM'] => 'NUM' %
          ['.ID'] => 'ID' %
          ['.OCT'] => 'OCT' %
          ['.HEX'] => 'HEX' %
          ['.SR'] => 'SR' %
          ['.CHR'] => 'CHR' %
          ['.DIG'] => 'DIG' %
          ['.LET'] => 'LET' % ;

NTESTT[-] => *1 ;
ND[-,-] => 'NDLBL ' *1 %
           'NDMK ' *2 % ;
NDNAM[-] => 'NDLBL ' *1 % ;
NDNBR[-] => 'NDMK ' *1 % ;
PSTR[-] => 'SRARG ' OPSTR[*1] % ;
GENCODE[] => 'OUTRE' %
             'CGER' % ;
DUMPTREE[] => 'DUMP' % ;


SIMPLEOUTRULELIST = '=>' (OUTPUTTEXT $(OUTPUTTEXT :DOO[2])
                         / '.EMPTY' :EMPTY[0] ) ;
SCRDEF[-,-] => '%'*1':' %
               *2
               'RC' % ;


OUTRULELIST = OUTRULE $(OUTRULE :ORLIST[2]) ;
OUTRULE = '[' (NODETEST / .EMPTY :NOBR[0]) ']' :NODETESTT[1] '=>' OUTEXPRESSION :OUTRULEE[2] ;
NODETEST = ITEM $(',' ITEM :ITEMSEQ[2]) ;
ITEM = ( .'-'
       / .ID '[' (NODETEST / .EMPTY :NOBR[0]) ']' :NODETESTT[1] :INODE[2]
       / .SR
       / NODENAME
       / LABEL
       / BASICTYPE ) :ITEMM[1] ;
NODENAME = '*' .NUM $(':' '*' .NUM :BRSEQ[2]) :NODENAMEE[1] ;
LABEL = ( .'#1'
        / .'#2'
        / .'#3'
        / .'#4' ) :LABELL[1] ;
OUTEXPRESSION = OUTALTERNATIVE $('/' OUTALTERNATIVE :OUTALTER[2]) ;
OUTALTERNATIVE = OUTITEM (EOUTITEM $(EOUTITEM :DOO[2]) :OUTSEQ[2] / .EMPTY) ;
EOUTITEM = OUTITEM :EOUTITEMM[1] ;
OUTITEM = OUTPUTTEXT
        / NODENAME :CRCALL[1]
        / .ID '[' (ARGLIST / .EMPTY :NOARG[0]) ']' :DIRCRCALL[2]
        / ARITHMETIC
        / '(' OUTEXPRESSION ')'
        / '.EMPTY' :EMPTY[0]
        / LABEL ;
OUTPUTTEXT = '%' :OUTNL[0]
           / .SR :OUTTEXT[1]
           / '!' .SR :OUTASM[1]
           / '@' .NUM :OUTCHAR[1] ;
ARGLIST = ARGMNT $(',' ARGMNT :ARGLISTT[2]) ;
ARGMNT = ( NODENAME
         / LABEL
         / .SR ) :ARGMNTT[1] ;

CRDEF[-,-] => '%'*1':' %
              *2
              '%_CREXIT'<OUT[CREXITCNT] ; CREXITCNT<-CREXITCNT+1>':' %
              'RC' % ;
ORLIST[-,-] => *1
               'BT '#1 %
               'RSTBR' %
               *2
               '%'#1':' % ;

OUTRULEE[-,-] => <LABITEM<-0>
                 *1
                 (<LABITEM#0> 'CLABS' % / .EMPTY)
                 'BF '#1 %
                 *2
                 'BR _CREXIT'<OUT[CREXITCNT]> %
                 '%'#1':' % ;

NODETESTT[NOBR[]] => 'CNTCK 0' %
         [-] => COUNTITEM[*1] 'CNTCK '<OUT[NI]> %
                'BF '#1 %
                *1
                '%'#1':' % ;

COUNTITEM[ITEMSEQ[-,-]] => COUNTITEM[*1:*1] < NI<-NI+1 >
         [-] => < NI<-1 > ;

ITEMSEQ[-,-] => *1
                'BF '#1 %
                'NXTBR' %
                *2
                '%'#1':' % ;

ITEMM[.SR] => 'CKTXT ' OPSTR[*1] %
     ['-'] => 'SET' %
     [INODE[-,-]] => *1

     [NODENAMEE[-]] => 'SAVEP' %
                       'MOVTOBASE' %
                       *1
                       'CKBR' %
                       'RSTRP' %
     [LABELL[-]] => ILABEL[*1:*1] <LABITEM<-1>
     ['.NUM'] => 'CKNUM' %
     ['.ID'] => 'CKID' %
     ['.OCT'] => 'CKOCT' %
     ['.HEX'] => 'CKHEX' %
     ['.SR'] => 'CKSR' %
     ['.CHR'] => 'CKCHR' %
     ['.DIG'] => 'CKDIG' %
     ['.LET'] => 'CKLET' % ;

ILABEL['#1'] => 'CKLAB 1' %
      ['#2'] => 'CKLAB 2' %
      ['#3'] => 'CKLAB 3' %
      ['#4'] => 'CKLAB 4' % ;

INODE[-,-] => 'CKNDNAM '*1 %
              'BF '#1 %
              'SAVEP' %
              'CHASE' %
              *2
              'RSTRP' %
              '%'#1':' % ;

NODENAMEE[BRSEQ[-,-]] => *1
         [.NUM] => 'MOVTO '*1 % ;
BRSEQ[.NUM,.NUM] => 'MOVTO '*1 %
                    'CHASE' %
                    'MOVTO '*2 %
     [-,.NUM] => *1
                 'CHASE' %
                 'MOVTO '*2 % ;


OUTALTER[-,-] => *1
                 'BT '#1 %
                 *2
                 '%'#1':' % ;

OUTSEQ[-,-] => *1
               'BF '#1 %
               *2
               '%'#1':' % ;
EOUTITEMM[-] => *1
                'OER' % ;

CRCALL[NODENAMEE[-]] => *1
                        'OUTCL' % ;

DIRCRCALL[.ID,NOARG[]] => 'NDLBL '*1 %
                          'NDMK 0' %
                          'OUTRE' %
         [.ID,-] => <NA<-0>
                    *2
                    'NDLBL '*1 %
                    'NDMK '<OUT[NA]> %
                    'OUTRE' % ;

ARGLISTT[-,-] => *1 *2 ;
ARGMNTT[NODENAMEE[-]] => 'SAVEP' %
                         *1
                         'BRARG' %
                         'RSTRP' % <NA<-NA+1>
       [LABELL[-]] => PASSLAB[*1:*1] <NA<-NA+1>
       [.SR] => 'SRARG ' OPSTR[*1] % <NA<-NA+1> ;
PASSLAB['#1'] => 'LBARG 1' %
       ['#2'] => 'LBARG 2' %
       ['#3'] => 'LBARG 3' %
       ['#4'] => 'LBARG 4' % ;

OUTNL[] => 'ONL' % 'SET' % ;
OUTTEXT[-] => 'OUTSR ' OPSTR[*1] % 'SET' % ;
OUTASM[-] => .EMPTY ;
OUTCHAR[-] => 'OUTCH '*1 % 'SET' % ;

LABELL['#1'] => 'GNLB 1' % 'SET' %
      ['#2'] => 'GNLB 2' % 'SET' %
      ['#3'] => 'GNLB 3' % 'SET' %
      ['#4'] => 'GNLB 4' % 'SET' % ;

ARITHMETIC = '<' ARITHLIST '>' ;
ARITHLIST = ARITH $(';' ARITH :DOO[2]) ;
ARITH = <- ASSIGNMENT
      / <- FUNCTION
      / RELATION ;
ASSIGNMENT = .ID '<-' EXPRESSION :STORE[2] ;

FUNCTION = .ID '[' (NODENAME / EXPRESSION) ']' :FUNC[2] ;

RELATION = .ID (.'='
               /.'#'
               /'>=' ('/' '+' +'>=+' / .EMPTY +'>=')
               /'>' ('/' '+' +'>+' / .EMPTY +'>')
               /'<=' ('/' '+' +'<=+' / .EMPTY +'<=')
               /'<' ('/' '+' +'<+' / .EMPTY +'<')) EXPRESSION :REL[3] ;
EXPRESSION = EOREXPR $('|' EOREXPR :OR[2]) :EXPRESSIONN[1] ;
EOREXPR = ANDEXPR $('^' ANDEXPR :EOR[2]) ;
ANDEXPR = SHIFTEXPR $('&' SHIFTEXPR :AND[2]) ;
SHIFTEXPR = ADDEXPR $('<<' ADDEXPR :LSH[2]
                     /'>>>' ADDEXPR :SRSH[2]
                     /'>>' ADDEXPR :URSH[2]) ;
ADDEXPR = MULTEXPR $('+' MULTEXPR :ADD[2]
                    /'-' MULTEXPR :SUB[2]) ;
MULTEXPR = UNARYEXPR $('*' UNARYEXPR :MUL[2]
                      /'/' UNARYEXPR :UDIV[2]
                      /'//' UNARYEXPR :SDIV[2]
                      /'%' UNARYEXPR :UMOD[2]
                      /'%%' UNARYEXPR :SMOD[2]) ;
UNARYEXPR = '-' PRIMARYEXPR :NEG[1]
          / '~' PRIMARYEXPR :CMPL[1]
          / '!' PRIMARYEXPR :LNEG[1]
          / ('+' / .EMPTY) PRIMARYEXPR ;
PRIMARYEXPR = <- FUNCTION
            / .ID :VAR[1]
            / .NUM :ICONST[1]
            / '(' EXPRESSION ')' ;

STORE[-,-] => *2 'STORE _'*1 % <ENTER[*1]> 'SET' % ;

REL[.ID,'=',-] => *3 'CMPEQ _'*1 %
   [.ID,'#',-] => *3 'CMPNEQ _'*1 %
   [.ID,'>=',-] => *3 'CMPUGET _'*1 %
   [.ID,'>=+',-] => *3 'CMPSGET _'*1 %
   [.ID,'>',-] => *3 'CMPUGT _'*1 %
   [.ID,'>+',-] => *3 'CMPSGT _'*1 %
   [.ID,'<=',-] => *3 'CMPULET _'*1 %
   [.ID,'<=+',-] => *3 'CMSLET _'*1 %
   [.ID,'<',-] => *3 'CMPULT _'*1 %
   [.ID,'<+',-] => *3 'CMPSLT _'*1 % ;

EXPRESSIONN[-] => *1 ;
OR[-,-] => *1 *2 'OR' % ;
EOR[-,-] => *1 *2 'EOR' % ;
AND[-,-] => *1 *2 'AND' % ;
LSH[-,-] => *1 *2 'LSH' % ;
SRSH[-,-] => *1 *2 'SRSH' % ;
URSH[-,-] => *1 *2 'URSH' % ;
ADD[-,-] => *1 *2 'ADD' % ;
SUB[-,-] => *1 *2 'SUB' % ;
MUL[-,-] => *1 *2 'MUL' % ;
UDIV[-,-] => *1 *2 'UDIV' % ;
SDIV[-,-] => *1 *2 'SDIV' % ;
UMOD[-,-] => *1 *2 'UMOD' % ;
SMOD[-,-] => *1 *2 'SMOD' % ;

NEG[-] => *1 'NEG' % ;
CMPL[-] => *1 'CMPL' % ;
LNEG[-] => *1 'LNEG' % ;

FUNC['LEN',NODENAMEE[-]] => FNNN['0',*2]
    ['CODE',NODENAMEE[-]] => FNNN['1',*2]
    ['CONV',NODENAMEE[-]] => FNNN['2',*2]
    ['XCONV',NODENAMEE[-]] => FNNN['3',*2]
    ['OUTL',NODENAMEE[-]] => FNNN['4',*2]
    ['OUTC',NODENAMEE[-]] => FNNN['5',*2]
    ['ENTER',NODENAMEE[-]] => FNNN['6',*2]
    ['LOOK',NODENAMEE[-]] => FNNN['7',*2]
    ['CLEAR',EXPRESSIONN[-]] => FNEX['8',*2]
    ['PUSH',EXPRESSIONN[-]] => FNEX['9',*2]
    ['POP',EXPRESSIONN[-]] => FNEX['10',*2]
    ['OUT',EXPRESSIONN[-]] => FNEX['11',*2]
    ['OUTS',EXPRESSIONN[-]] => FNEX['12',*2] ;

FNNN[-,-] => 'SAVEP' %
             *2
             'FUNC '*1 %
             'RSTRP' %
             (LOOKFUNC[*1] .EMPTY / 'SET' %) ;
LOOKFUNC['7'] => .EMPTY ;
FNEX[-,-] => *2
             'FUNC '*1 %
             'SET' % ;

VAR[-] => 'LOAD _'*1 % <ENTER[*1]> ;
ICONST[-] => 'LOADI '*1 % ;

ALLOCVARS := '%_'*1':'%
             'CELL 0' % ;


SYMRDEF[-,-] => '%'*1':' %
                'SRINIT '#2 %
                '%'#1':' %
                *2
                /*'MOVTOBASE' %*/
                'SRCHECK '#1 %
                '%'#2':' %
                'RC' % ;


NOBR[] => .EMPTY ;
NOARG[] => .EMPTY ;
OPSTR[-] => @39 *1 @39 ;
EMPTY[] => 'SET' % ;
DOO[-,-] => *1 *2 ;

.END
