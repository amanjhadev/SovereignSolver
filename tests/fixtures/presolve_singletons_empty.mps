NAME          PRE_SINGLE
ROWS
 N  COST
 E  C1
 G  C2
 L  C_EMPTY
COLUMNS
    X1        COST       1.0  C1        2.0  C2        1.0
    X2        COST       2.0  C2        1.0
    X3        COST       3.0  C2        1.0
    X4        COST       5.0
RHS
    RHS1      C1         6.0  C2        8.0  C_EMPTY  10.0
BOUNDS
 LO BND1      X1         0.0
 UP BND1      X1        10.0
 LO BND1      X2         0.0
 UP BND1      X2        10.0
 LO BND1      X3         0.0
 UP BND1      X3        10.0
 LO BND1      X4         2.0
 UP BND1      X4        10.0
ENDATA
