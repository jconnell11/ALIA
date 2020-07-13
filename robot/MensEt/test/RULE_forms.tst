if something is a girl then it is a person   // fwd  = $cond $res
something is a tiger if it is orange         // rev  = $res $cond
a tiger is orange                            // ifwd = $cond-i $res-i with %obj-i
orange is a color                            // ifwd = $cond-i $res-i with HQ
orange striped things are tigers             // sfwd = $cond-s $res-s
move means drive                             // macro with ACT

if something is orange and it is striped then it is a tiger    // conjunctive cond
if something is a tiger it is orange and striped               // conjunctive res
if something is striped but it is red then it is not a tiger   // negative res
if something is not striped but it is red then it is a dog     // negative cond
if something is red, unless it is striped, it is a dog         // explicit unless
unless it is not red if something is striped it is a dog       // negative unless


