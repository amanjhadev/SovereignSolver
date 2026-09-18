export type SolverStatusName =
  | 'Optimal'
  | 'Feasible'
  | 'TimeLimit'
  | 'NodeLimit'
  | 'Unresolved'
  | 'Infeasible'
  | 'Unbounded'
  | 'NumericalFailure'
  | 'UserInterrupted'
  | 'NotSolved';

export interface SolverTelemetry {
  variables: number;
  binaries: number;
  continuous: number;
  constraints: number;
  nnz: number;
  sense: string;
  root_lp_status?: number;
  root_lp_objective?: number;
  root_lp_iterations?: number;
  fractional_binaries?: number;
  cuts_generated?: number;
  cuts_accepted?: number;
  heuristic_incumbents?: number;
}

export interface SolverLogItem {
  timestamp: string;
  phase: string;
  message: string;
  type?: 'info' | 'warn' | 'error';
}

export interface MipCheckpoint {
  time_sec: number;
  node: number;
  incumbent?: number;
  best_bound?: number;
}

export interface BbTreeNode {
  id: number;
  parentId?: number;
  depth: number;
  bound: number;
  status: 'processing' | 'open' | 'fathomed' | 'infeasible' | 'bound_pruned' | 'integer_feasible';
  incumbentContribution?: number;
}

export interface SolveResult {
  success: boolean;
  solveId?: string;
  model_name?: string;
  status_code?: number;
  status_name?: SolverStatusName | string;
  message?: string;
  objective_value?: number;
  best_bound?: number;
  mip_gap?: number;
  nodes_explored?: number;
  nodes_created?: number;
  lp_solves?: number;
  solve_time_ms?: number;
  solution?: Record<string, number>;
  telemetry?: SolverTelemetry;
  logs?: SolverLogItem[];
  progressHistory?: MipCheckpoint[];
  treeNodes?: BbTreeNode[];
  error?: string;
}

export interface ValidationInfo {
  success: boolean;
  model_name?: string;
  variables?: number;
  binaries?: number;
  continuous?: number;
  constraints?: number;
  nnz?: number;
  sense?: string;
  error?: string;
}

export interface BackendHealth {
  connected: boolean;
  version: string;
  solverReady: boolean;
  checking: boolean;
}

export type PresetInputType = 'structured' | 'mps';

export interface PresetModel {
  id: string;
  name: string;
  type: PresetInputType;
  description: string;
  content: string;
  varsSummary?: string;
  consSummary?: string;
  isBuiltIn?: boolean;
}
