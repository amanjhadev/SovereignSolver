NAME          MIXEDBOUND
ROWS
 N  COST
 L  C1
 G  C2
 E  C3
 L  C4
COLUMNS
    X1        COST       2.0  C1        1.0  C2        2.0
    X1        C4        1.0
    X2        COST      -1.0  C1        1.0  C3        1.0
    X2        C4        1.0
    X3        COST       3.0  C2        1.0  C4        1.0
    X4        COST      -4.0  C3        2.0  C4        1.0
RHS
    RHS1      C1        12.0  C2        6.0  C3       10.0
    RHS1      C4        20.0
BOUNDS
 LO BND1      X1         1.0
 UP BND1      X1         5.0
 LO BND1      X2         2.0
 UP BND1      X2         8.0
 LO BND1      X3         0.0
 UP BND1      X3        10.0
 LO BND1      X4         0.0
 UP BND1      X4         6.0
ENDATA
