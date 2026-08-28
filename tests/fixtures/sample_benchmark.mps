NAME          BENCHMARK1
ROWS
 N  COST
 L  C1
 L  C2
 G  C3
COLUMNS
    X1        COST      10.0  C1        1.0
    X1        C2        2.0   C3        1.0
    X2        COST      15.0  C1        2.0
    X2        C2        1.0   C3        3.0
    X3        COST      20.0  C1        1.0
    X3        C2        3.0   C3        1.0
RHS
    RHS1      C1       100.0  C2       150.0
    RHS1      C3        30.0
BOUNDS
 LO BND1      X1        0.0
 LO BND1      X2        0.0
 LO BND1      X3        0.0
 UP BND1      X1        40.0
 UP BND1      X2        40.0
 UP BND1      X3        40.0
ENDATA
