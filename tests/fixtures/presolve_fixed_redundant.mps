NAME          PRE_FIXRED
ROWS
 N  COST
 L  C1
 L  C2
 G  C3
COLUMNS
    X1        COST       2.0  C1        1.0
    X2        COST       3.0  C1        1.0  C2        1.0
    X2        C3        1.0
    X3        COST       1.0  C1        1.0  C2        1.0
RHS
    RHS1      C1        25.0  C2       30.0  C3        4.0
BOUNDS
 LO BND1      X1         5.0
 UP BND1      X1         5.0
 LO BND1      X2         0.0
 UP BND1      X2        10.0
 LO BND1      X3         0.0
 UP BND1      X3        10.0
ENDATA
