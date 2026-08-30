import React from 'react';
import { CheckCircle2, AlertTriangle, XCircle, Clock, Activity, Target, Hash, Play, StopCircle, HelpCircle } from 'lucide-react';
import { SolveResult, ValidationInfo } from '../types';

interface ResultPanelProps {
  result: SolveResult | null;
  validationInfo: ValidationInfo | null;
  isSolving: boolean;
}

export const ResultPanel: React.FC<ResultPanelProps> = ({ result, validationInfo, isSolving }) => {
  // Empty State: No solve or validation run yet
  if (!result && !validationInfo && !isSolving) {
    return (
      <div className="p-6 text-center text-slate-500 font-mono text-xs flex flex-col items-center justify-center h-48 border-b border-slate-800 space-y-2">
        <Activity className="w-8 h-8 text-slate-700 animate-pulse" />
        <p className="font-sans text-slate-400">Create or load an optimization model to begin.</p>
        <p className="text-[11px] text-slate-600">Click VALIDATE to check syntax, or SOLVE MODEL to execute.</p>
      </div>
    );
  }

  // Active Solving state
  if (isSolving) {
    return (
      <div className="p-6 border-b border-slate-800 bg-indigo-950/20 space-y-3">
        <div className="flex items-center gap-3">
          <div className="w-3 h-3 rounded-full bg-amber-400 animate-ping"></div>
          <div>
            <div className="text-sm font-bold text-amber-300 font-mono">◌ SOLVING MODEL...</div>
            <div className="text-xs text-slate-400">Executing C++ Dual Simplex & Parallel MILP engine</div>
          </div>
        </div>
      </div>
    );
  }

  // Pure Validation Result
  if (validationInfo && !result) {
    return (
      <div className="p-4 border-b border-slate-800 bg-slate-900/40 space-y-3">
        <div className="text-xs font-semibold text-slate-300 flex items-center justify-between">
          <div className="flex items-center gap-2">
            {validationInfo.success ? (
              <CheckCircle2 className="w-4 h-4 text-emerald-400" />
            ) : (
              <XCircle className="w-4 h-4 text-rose-400" />
            )}
            <span>Validation Result</span>
          </div>
          <span className="font-mono text-[10px] text-slate-400 uppercase">{validationInfo.model_name || 'Model'}</span>
        </div>

        {validationInfo.success ? (
          <div className="bg-emerald-950/30 border border-emerald-500/30 rounded-lg p-3 space-y-2 font-mono text-xs">
            <div className="text-emerald-300 font-bold flex items-center gap-1.5">
              <span>✓ MODEL VALID</span>
            </div>
            <div className="grid grid-cols-2 gap-2 text-[11px] text-slate-300 pt-1 border-t border-emerald-900/40">
              <div>
                <span className="text-slate-500">Variables:</span>{' '}
                <span className="text-emerald-400 font-bold">{validationInfo.variables}</span> ({validationInfo.binaries} bin, {validationInfo.continuous} cont)
              </div>
              <div>
                <span className="text-slate-500">Constraints:</span>{' '}
                <span className="text-emerald-400 font-bold">{validationInfo.constraints}</span> ({validationInfo.nnz} nonzeros)
              </div>
            </div>
          </div>
        ) : (
          <div className="bg-rose-950/40 border border-rose-500/30 text-rose-300 p-3 rounded-lg text-xs font-mono">
            <div className="font-bold mb-1 flex items-center gap-1.5 text-rose-400">
              <XCircle className="w-4 h-4" /> ✕ MODEL INVALID
            </div>
            <p className="text-[11px] text-rose-200">{validationInfo.error || 'Model parse/validation failed'}</p>
          </div>
        )}
      </div>
    );
  }

  if (!result) return null;

  // Status Badge Mapper according to exact solver semantics
  const renderStatusBadge = (statusName?: string) => {
    const s = statusName || 'NotSolved';
    switch (s) {
      case 'Optimal':
        return (
          <span className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-extrabold bg-emerald-500/15 text-emerald-400 border border-emerald-500/40 shadow-sm">
            <CheckCircle2 className="w-4 h-4" /> ✓ OPTIMAL
          </span>
        );
      case 'Feasible':
        return (
          <span className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-extrabold bg-blue-500/15 text-blue-400 border border-blue-500/40 shadow-sm">
            <CheckCircle2 className="w-4 h-4" /> ✓ FEASIBLE
          </span>
        );
      case 'TimeLimit':
        return (
          <span className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-extrabold bg-amber-500/15 text-amber-400 border border-amber-500/40 shadow-sm">
            <Clock className="w-4 h-4" /> ⚠ TIME LIMIT
          </span>
        );
      case 'NodeLimit':
        return (
          <span className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-extrabold bg-amber-500/15 text-amber-400 border border-amber-500/40 shadow-sm">
            <Clock className="w-4 h-4" /> ⚠ NODE LIMIT
          </span>
        );
      case 'Infeasible':
        return (
          <span className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-extrabold bg-rose-500/15 text-rose-400 border border-rose-500/40 shadow-sm">
            <XCircle className="w-4 h-4" /> ✕ INFEASIBLE
          </span>
        );
      case 'Unbounded':
        return (
          <span className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-extrabold bg-purple-500/15 text-purple-400 border border-purple-500/40 shadow-sm">
            <AlertTriangle className="w-4 h-4" /> ✕ UNBOUNDED
          </span>
        );
      case 'NumericalFailure':
        return (
          <span className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-extrabold bg-rose-500/15 text-rose-400 border border-rose-500/40 shadow-sm">
            <AlertTriangle className="w-4 h-4" /> ✕ NUMERICAL FAILURE
          </span>
        );
      case 'UserInterrupted':
        return (
          <span className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-extrabold bg-amber-500/15 text-amber-300 border border-amber-500/40 shadow-sm">
            <StopCircle className="w-4 h-4" /> ⏹ USER INTERRUPTED
          </span>
        );
      case 'Unresolved':
        return (
          <span className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-extrabold bg-slate-800 text-amber-400 border border-amber-500/30">
            <AlertTriangle className="w-4 h-4" /> ⚠ UNRESOLVED
          </span>
        );
      default:
        return (
          <span className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-bold bg-slate-800 text-slate-300 border border-slate-700">
            <HelpCircle className="w-4 h-4" /> ○ {s}
          </span>
        );
    }
  };

  const openNodes =
    result.nodes_created !== undefined && result.nodes_explored !== undefined
      ? Math.max(0, result.nodes_created - result.nodes_explored)
      : undefined;

  const formattedTime =
    result.solve_time_ms !== undefined
      ? result.solve_time_ms >= 1000
        ? `${(result.solve_time_ms / 1000).toFixed(2)} s`
        : `${result.solve_time_ms.toFixed(1)} ms`
      : 'N/A';

  return (
    <div className="p-4 border-b border-slate-800 space-y-4">
      {/* Header Status Row */}
      <div className="flex items-center justify-between">
        <div>
          <span className="text-[10px] text-slate-500 uppercase tracking-wider font-bold block mb-1">
            Solver Primary Status
          </span>
          <div>{renderStatusBadge(result.status_name)}</div>
        </div>
        <div className="text-right font-mono">
          <span className="text-[10px] text-slate-500 uppercase tracking-wider font-bold block mb-0.5">Solve Time</span>
          <div className="text-sm font-extrabold text-slate-100">{formattedTime}</div>
        </div>
      </div>

      {/* Primary 4 Cards Grid */}
      <div className="grid grid-cols-2 sm:grid-cols-4 gap-2.5">
        {/* Objective */}
        <div className="bg-slate-900 p-3 rounded-lg border border-slate-800 space-y-1">
          <div className="flex items-center gap-1.5 text-[11px] text-slate-400 font-semibold">
            <Target className="w-3.5 h-3.5 text-indigo-400 shrink-0" />
            <span>Objective</span>
          </div>
          <div className="text-sm font-extrabold text-slate-100 font-mono truncate" title={result.objective_value?.toString()}>
            {result.objective_value !== undefined ? result.objective_value.toFixed(6) : 'N/A'}
          </div>
        </div>

        {/* Best Bound */}
        <div className="bg-slate-900 p-3 rounded-lg border border-slate-800 space-y-1">
          <div className="flex items-center gap-1.5 text-[11px] text-slate-400 font-semibold">
            <Activity className="w-3.5 h-3.5 text-emerald-400 shrink-0" />
            <span>Best Bound</span>
          </div>
          <div className="text-sm font-extrabold text-slate-100 font-mono truncate" title={result.best_bound?.toString()}>
            {result.best_bound !== undefined ? result.best_bound.toFixed(6) : 'N/A'}
          </div>
        </div>

        {/* MIP Gap */}
        <div className="bg-slate-900 p-3 rounded-lg border border-slate-800 space-y-1">
          <div className="flex items-center gap-1.5 text-[11px] text-slate-400 font-semibold">
            <Hash className="w-3.5 h-3.5 text-amber-400 shrink-0" />
            <span>MIP Gap</span>
          </div>
          <div className="text-sm font-extrabold text-slate-100 font-mono">
            {result.mip_gap !== undefined ? `${(result.mip_gap * 100).toFixed(2)}%` : 'N/A'}
          </div>
        </div>

        {/* Solve Time */}
        <div className="bg-slate-900 p-3 rounded-lg border border-slate-800 space-y-1">
          <div className="flex items-center gap-1.5 text-[11px] text-slate-400 font-semibold">
            <Clock className="w-3.5 h-3.5 text-indigo-400 shrink-0" />
            <span>Time</span>
          </div>
          <div className="text-sm font-extrabold text-indigo-300 font-mono">{formattedTime}</div>
        </div>
      </div>

      {/* Secondary Telemetry: Nodes & LP Solves */}
      <div className="grid grid-cols-2 gap-2.5 text-xs font-mono">
        <div className="bg-slate-900/60 p-2.5 rounded-lg border border-slate-800/80 flex justify-between items-center">
          <span className="text-slate-400 text-[11px]">Nodes (Explored / Created / Open):</span>
          <span className="text-slate-100 font-bold">
            {result.nodes_explored ?? 0} / {result.nodes_created ?? 0} / {openNodes ?? 'N/A'}
          </span>
        </div>
        <div className="bg-slate-900/60 p-2.5 rounded-lg border border-slate-800/80 flex justify-between items-center">
          <span className="text-slate-400 text-[11px]">LP Relaxations:</span>
          <span className="text-slate-100 font-bold">{result.lp_solves ?? 'N/A'}</span>
        </div>
      </div>

      {/* Solver Message / Failure details */}
      {result.message && (
        <div className="text-xs font-mono bg-slate-950 p-2.5 rounded-lg border border-slate-800 text-slate-300">
          <span className="text-slate-500 font-semibold">Solver Message:</span> {result.message}
        </div>
      )}
    </div>
  );
};
