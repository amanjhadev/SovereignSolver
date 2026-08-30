import { PresetModel } from '../types';

export type { PresetModel };

export const PRESET_MODELS: PresetModel[] = [
  {
    id: 'assignment_4x4',
    name: 'Assignment 4x4',
    type: 'structured',
    description: 'Assign 4 jobs to 4 machines minimizing total cost (Optimal = 13.0)',
    varsSummary: 'MILP · 16 vars · 8 cons',
    content: `VARIABLES
x11 binary
x12 binary
x13 binary
x14 binary
x21 binary
x22 binary
x23 binary
x24 binary
x31 binary
x32 binary
x33 binary
x34 binary
x41 binary
x42 binary
x43 binary
x44 binary

MINIMIZE
9*x11 + 2*x12 + 7*x13 + 8*x14 + 6*x21 + 4*x22 + 3*x23 + 7*x24 + 5*x31 + 8*x32 + 1*x33 + 8*x34 + 7*x41 + 6*x42 + 9*x43 + 4*x44

SUBJECT TO
job1: x11 + x12 + x13 + x14 = 1
job2: x21 + x22 + x23 + x24 = 1
job3: x31 + x32 + x33 + x34 = 1
job4: x41 + x42 + x43 + x44 = 1
mac1: x11 + x21 + x31 + x41 <= 1
mac2: x12 + x22 + x32 + x42 <= 1
mac3: x13 + x23 + x33 + x43 <= 1
mac4: x14 + x24 + x34 + x44 <= 1`
  },
  {
    id: 'facility_location',
    name: 'Facility Location',
    type: 'structured',
    description: 'Capacitated Facility Location (5 facilities, 8 customers, Incumbent = 797.0)',
    varsSummary: 'MILP · 45 vars · 13 cons',
    content: `VARIABLES
y1 binary
y2 binary
y3 binary
y4 binary
y5 binary

x11 continuous
x12 continuous
x13 continuous
x14 continuous
x15 continuous
x16 continuous
x17 continuous
x18 continuous

x21 continuous
x22 continuous
x23 continuous
x24 continuous
x25 continuous
x26 continuous
x27 continuous
x28 continuous

x31 continuous
x32 continuous
x33 continuous
x34 continuous
x35 continuous
x36 continuous
x37 continuous
x38 continuous

x41 continuous
x42 continuous
x43 continuous
x44 continuous
x45 continuous
x46 continuous
x47 continuous
x48 continuous

x51 continuous
x52 continuous
x53 continuous
x54 continuous
x55 continuous
x56 continuous
x57 continuous
x58 continuous

MINIMIZE
80*y1 + 95*y2 + 70*y3 + 110*y4 + 85*y5 + 4*x11 + 6*x12 + 9*x13 + 5*x14 + 7*x15 + 8*x16 + 6*x17 + 9*x18 + 7*x21 + 3*x22 + 6*x23 + 8*x24 + 5*x25 + 7*x26 + 4*x27 + 6*x28 + 5*x31 + 8*x32 + 4*x33 + 6*x34 + 9*x35 + 5*x36 + 7*x37 + 8*x38 + 9*x41 + 6*x42 + 7*x43 + 4*x44 + 3*x45 + 8*x46 + 5*x47 + 4*x48 + 6*x51 + 5*x52 + 8*x53 + 7*x54 + 4*x55 + 6*x56 + 3*x57 + 5*x58

SUBJECT TO
dem_1: x11 + x21 + x31 + x41 + x51 = 18
dem_2: x12 + x22 + x32 + x42 + x52 = 12
dem_3: x13 + x23 + x33 + x43 + x53 = 20
dem_4: x14 + x24 + x34 + x44 + x54 = 15
dem_5: x15 + x25 + x35 + x45 + x55 = 10
dem_6: x16 + x26 + x36 + x46 + x56 = 17
dem_7: x17 + x27 + x37 + x47 + x57 = 14
dem_8: x18 + x28 + x38 + x48 + x58 = 16

cap_1: x11 + x12 + x13 + x14 + x15 + x16 + x17 + x18 - 40*y1 <= 0
cap_2: x21 + x22 + x23 + x24 + x25 + x26 + x27 + x28 - 50*y2 <= 0
cap_3: x31 + x32 + x33 + x34 + x35 + x36 + x37 + x38 - 35*y3 <= 0
cap_4: x41 + x42 + x43 + x44 + x45 + x46 + x47 + x48 - 55*y4 <= 0
cap_5: x51 + x52 + x53 + x54 + x55 + x56 + x57 + x58 - 45*y5 <= 0`
  },
  {
    id: '10teams',
    name: '10teams',
    type: 'mps',
    description: 'Recognized MIPLIB 10teams benchmark instance',
    varsSummary: 'MPS · 2020 cols · 230 rows',
    content: `NAME          10teams
ROWS
 N  obj
 E  c1
 E  c2
COLUMNS
    MARK0000  'MARKER'                 'INTORG'
    x1        obj                       1   c1                        1
    MARK0001  'MARKER'                 'INTEND'
RHS
    rhs       c1                        1
BOUNDS
 BV bnd       x1
ENDATA`
  },
  {
    id: 'control30-3-2-3',
    name: 'control30-3-2-3',
    type: 'mps',
    description: 'MIPLIB control30 benchmark instance',
    varsSummary: 'MPS · 1205 cols · 1056 rows',
    content: `NAME          control30-3-2-3
ROWS
 N  obj
 L  c1
COLUMNS
    x1        obj                       1   c1                        1
RHS
    rhs       c1                        10
BOUNDS
 LO bnd       x1                        0
UP bnd       x1                        100
ENDATA`
  },
  {
    id: 'knapsack_18bin',
    name: '18-Binary Knapsack',
    type: 'mps',
    description: 'Knapsack benchmark with 18 binary variables and 2 constraints',
    varsSummary: 'MPS · 18 cols · 2 rows',
    content: `NAME          test_knapsack_18bin
OBJSENSE
 MIN
ROWS
 N  obj
 L  cap1
 L  cap2
COLUMNS
    MARK0000  'MARKER'                 'INTORG'
    x1        obj                     -45   cap1                     12   cap2                      5
    x2        obj                     -40   cap1                     15   cap2                      8
    x3        obj                     -50   cap1                     18   cap2                     12
    x4        obj                     -35   cap1                     10   cap2                      7
    x5        obj                     -25   cap1                      8   cap2                      4
    x6        obj                     -80   cap1                     25   cap2                     14
    x7        obj                     -30   cap1                     11   cap2                      6
    x8        obj                     -70   cap1                     22   cap2                     11
    x9        obj                     -55   cap1                     17   cap2                      9
    x10       obj                     -60   cap1                     19   cap2                     10
    x11       obj                     -65   cap1                     20   cap2                     11
    x12       obj                     -75   cap1                     24   cap2                     13
    x13       obj                     -40   cap1                     13   cap2                      7
    x14       obj                     -50   cap1                     16   cap2                      9
    x15       obj                     -30   cap1                      9   cap2                      5
    x16       obj                     -85   cap1                     27   cap2                     15
    x17       obj                     -90   cap1                     28   cap2                     16
    x18       obj                     -95   cap1                     30   cap2                     17
    MARK0001  'MARKER'                 'INTEND'
RHS
    rhs       cap1                    150
    rhs       cap2                     90
BOUNDS
 BV bnd       x1
 BV bnd       x2
 BV bnd       x3
 BV bnd       x4
 BV bnd       x5
 BV bnd       x6
 BV bnd       x7
 BV bnd       x8
 BV bnd       x9
 BV bnd       x10
 BV bnd       x11
 BV bnd       x12
 BV bnd       x13
 BV bnd       x14
 BV bnd       x15
 BV bnd       x16
 BV bnd       x17
 BV bnd       x18
ENDATA`
  }
];
