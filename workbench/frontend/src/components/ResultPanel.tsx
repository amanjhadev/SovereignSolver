import React from 'react';
import { SolveResult, ValidationInfo } from '../types';

interface ResultPanelProps {
  result: SolveResult | null;
  validationInfo: ValidationInfo | null;
  isSolving: boolean;
}

export const ResultPanel: React.FC<ResultPanelProps> = ({ result, validationInfo, isSolving }) => {
  // Pure Validation Result
  if (validationInfo && !result && !isSolving) {
    return (
      <div className="p-3.5 border-b border-slate-800 bg-slate-900/40 space-y-2.5 text-xs">
        <div className="flex items-center justify-between">
          <span className="font-semibold text-slate-200">Validation Result</span>
          <span className="font-mono text-[11px] text-slate-400">{validationInfo.model_name || 'Model'}</span>
        </div>

        {validationInfo.success ? (
          <div className="bg-slate-950 border border-slate-800 rounded p-3 space-y-2 font-mono text-xs">
            <div className="text-emerald-400 font-medium flex items-center gap-2">
              <span className="w-1.5 h-1.5 rounded-full bg-emerald-400"></span>
              <span>Model definition valid</span>
            </div>
            <div className="grid grid-cols-2 gap-2 text-[11px] text-slate-300 pt-1.5 border-t border-slate-800">
              <div>
                <span className="text-slate-500">Variables:</span>{' '}
                <span className="text-slate-100 font-medium">{validationInfo.variables}</span>{' '}
                <span className="text-slate-500">({validationInfo.binaries} bin, {validationInfo.continuous} cont)</span>
              </div>
              <div>
                <span className="text-slate-500">Constraints:</span>{' '}
                <span className="text-slate-100 font-medium">{validationInfo.constraints}</span>{' '}
                <span className="text-slate-500">({validationInfo.nnz} nonzeros)</span>
              </div>
            </div>
          </div>
        ) : (
          <div className="bg-slate-950 border border-rose-900/60 rounded p-3 text-xs font-mono space-y-1">
            <div className="text-rose-400 font-medium flex items-center gap-2">
              <span className="w-1.5 h-1.5 rounded-full bg-rose-500"></span>
              <span>Model invalid</span>
            </div>
            <p className="text-[11px] text-rose-300">{validationInfo.error || 'Syntax or validation error in model'}</p>
          </div>
        )}
      </div>
    );
  }

  if (!result) return null;

  const renderStatusBadge = (statusName?: string) => {
    const s = statusName || 'NotSolved';
    switch (s) {
      case 'Optimal':
        return (
          <span className="px-2 py-0.5 rounded text-xs font-medium bg-emerald-950/80 text-emerald-300 border border-emerald-800/80">
            Optimal
          </span>
        );
      case 'Feasible':
        return (
          <span className="px-2 py-0.5 rounded text-xs font-medium bg-blue-950/80 text-blue-300 border border-blue-800/80">
            Feasible
          </span>
        );
      case 'TimeLimit':
        return (
          <span className="px-2 py-0.5 rounded text-xs font-medium bg-amber-950/80 text-amber-300 border border-amber-800/80">
            Time Limit
          </span>
        );
      case 'NodeLimit':
        return (
          <span className="px-2 py-0.5 rounded text-xs font-medium bg-amber-950/80 text-amber-300 border border-amber-800/80">
            Node Limit
          </span>
        );
      case 'Infeasible':
        return (
          <span className="px-2 py-0.5 rounded text-xs font-medium bg-rose-950/80 text-rose-300 border border-rose-800/80">
            Infeasible
          </span>
        );
      case 'Unbounded':
        return (
          <span className="px-2 py-0.5 rounded text-xs font-medium bg-purple-950/80 text-purple-300 border border-purple-800/80">
            Unbounded
          </span>
        );
      case 'NumericalFailure':
        return (
          <span className="px-2 py-0.5 rounded text-xs font-medium bg-rose-950/80 text-rose-300 border border-rose-800/80">
            Numerical Failure
          </span>
        );
      case 'UserInterrupted':
        return (
          <span className="px-2 py-0.5 rounded text-xs font-medium bg-amber-950/80 text-amber-300 border border-amber-800/80">
            User Interrupted
          </span>
        );
      case 'Solving':
        return (
          <span className="px-2 py-0.5 rounded text-xs font-medium bg-amber-950/80 text-amber-300 border border-amber-800/80">
            Solving
          </span>
        );
      case 'Unresolved':
        return (
          <span className="px-2 py-0.5 rounded text-xs font-medium bg-slate-900 text-slate-300 border border-slate-700">
            Unresolved
          </span>
        );
      default:
        return (
          <span className="px-2 py-0.5 rounded text-xs font-medium bg-slate-900 text-slate-300 border border-slate-700">
            {s}
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
      : '—';

  return (
    <div className="p-3.5 border-b border-slate-800 space-y-3">
      {/* Header Status Row */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2">
          <span className="text-xs font-semibold text-slate-200">Solution Result</span>
          {renderStatusBadge(result.status_name)}
        </div>
        <div className="font-mono text-xs text-slate-400">
          Runtime: <span className="text-slate-100 font-medium">{formattedTime}</span>
        </div>
      </div>

      {/* Structured Metrics Table */}
      <div className="bg-slate-950 rounded border border-slate-800 overflow-hidden font-mono text-xs">
        <table className="w-full text-left">
          <tbody className="divide-y divide-slate-800/80">
            <tr className="hover:bg-slate-900/40">
              <td className="py-1.5 px-3 text-slate-400 font-sans text-[11px] w-36">Objective Value</td>
              <td className="py-1.5 px-3 text-slate-100 font-medium">
                {result.objective_value !== undefined ? result.objective_value.toFixed(6) : '—'}
              </td>
            </tr>
            <tr className="hover:bg-slate-900/40">
              <td className="py-1.5 px-3 text-slate-400 font-sans text-[11px]">Best Bound</td>
              <td className="py-1.5 px-3 text-slate-100 font-medium">
                {result.best_bound !== undefined ? result.best_bound.toFixed(6) : '—'}
              </td>
            </tr>
            <tr className="hover:bg-slate-900/40">
              <td className="py-1.5 px-3 text-slate-400 font-sans text-[11px]">MIP Gap</td>
              <td className="py-1.5 px-3 text-slate-100 font-medium">
                {result.mip_gap !== undefined ? `${(result.mip_gap * 100).toFixed(2)}%` : '—'}
              </td>
            </tr>
            <tr className="hover:bg-slate-900/40">
              <td className="py-1.5 px-3 text-slate-400 font-sans text-[11px]">B&B Nodes</td>
              <td className="py-1.5 px-3 text-slate-300">
                {result.nodes_explored ?? 0} explored
                {result.nodes_created !== undefined && ` · ${result.nodes_created} created`}
                {openNodes !== undefined && ` · ${openNodes} open`}
              </td>
            </tr>
            <tr className="hover:bg-slate-900/40">
              <td className="py-1.5 px-3 text-slate-400 font-sans text-[11px]">LP Relaxations</td>
              <td className="py-1.5 px-3 text-slate-300">
                {result.lp_solves ?? '—'}
              </td>
            </tr>
          </tbody>
        </table>
      </div>

      {/* Solver Message / Failure details */}
      {result.message && (
        <div className="text-[11px] font-mono bg-slate-950 p-2.5 rounded border border-slate-800 text-slate-300">
          <span className="text-slate-500">Message:</span> {result.message}
        </div>
      )}
    </div>
  );
};
