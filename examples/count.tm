.META PRIDS
PRIDS = .ID $ ( ',' .ID :DD[2] ) '.END' :IDENTS[1] * ;
IDENTS[-] => COUNT[*1] 'THERE ARE '< OUT[A] > ' IDENTIFIERS' % *1 % ;
COUNT[DD[-,-]] => COUNT[*1:*1] < A<-A+1 >
     [.ID]     => < A<-1 > ;
DD[-,-] => *1 % *2 ;
.END
