import React from 'react';
import { SolveResult, ValidationInfo } from '../types';

interface ResultPanelProps {
  result: SolveResult | null;
  validationInfo: ValidationInfo | null;
  isSolving: boolean;
}

export const ResultPanel: React.FC<ResultPanelProps> = ({ result, validationInfo, isSolving }) => {
  // Pure Validation Result State
  if (validationInfo && !result && !isSolving) {
    return (
      <div className="p-3.5 border-b border-white/[0.08] bg-[#090d14]/60 space-y-2.5 text-xs text-slate-200">
        <div className="flex items-center justify-between">
          <span className="font-semibold text-white tracking-tight">Validation Result</span>
          <span className="font-mono text-[11px] text-slate-400">
            {validationInfo.model_name || 'Model'}
          </span>
        </div>

        {validationInfo.success ? (
          <div className="bg-[#06080c] border border-emerald-500/20 rounded-[5px] p-3 space-y-2 font-mono text-xs">
            <div className="text-emerald-400 font-medium flex items-center gap-2">
              <span className="w-1.5 h-1.5 rounded-full bg-emerald-400"></span>
              <span>Model definition valid</span>
            </div>
            <div className="grid grid-cols-2 gap-2 text-[11px] text-slate-300 pt-2 border-t border-white/[0.06]">
              <div>
                <span className="text-slate-500 font-sans">Variables:</span>{' '}
                <span className="text-white font-medium tabular-nums">{validationInfo.variables}</span>{' '}
                <span className="text-slate-500 font-normal">
                  ({validationInfo.binaries}b / {validationInfo.continuous}c)
                </span>
              </div>
              <div>
                <span className="text-slate-500 font-sans">Constraints:</span>{' '}
                <span className="text-white font-medium tabular-nums">{validationInfo.constraints}</span>{' '}
                <span className="text-slate-500 font-normal">({validationInfo.nnz} nonzeros)</span>
              </div>
            </div>
          </div>
        ) : (
          <div className="bg-[#06080c] border border-rose-500/30 rounded-[5px] p-3 text-xs font-mono space-y-1">
            <div className="text-rose-400 font-medium flex items-center gap-2">
              <span className="w-1.5 h-1.5 rounded-full bg-rose-500"></span>
              <span>Model invalid</span>
            </div>
            <p className="text-[11px] text-rose-300 leading-relaxed">
              {validationInfo.error || 'Syntax or validation error in model'}
            </p>
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
          <span className="px-2 py-0.5 rounded-[4px] text-[11px] font-medium bg-emerald-950/80 text-emerald-300 border border-emerald-700/60 font-mono">
            Optimal
          </span>
        );
      case 'Feasible':
        return (
          <span className="px-2 py-0.5 rounded-[4px] text-[11px] font-medium bg-sky-950/80 text-sky-300 border border-sky-700/60 font-mono">
            Feasible
          </span>
        );
      case 'TimeLimit':
        return (
          <span className="px-2 py-0.5 rounded-[4px] text-[11px] font-medium bg-amber-950/80 text-amber-300 border border-amber-700/60 font-mono">
            Time Limit
          </span>
        );
      case 'NodeLimit':
        return (
          <span className="px-2 py-0.5 rounded-[4px] text-[11px] font-medium bg-amber-950/80 text-amber-300 border border-amber-700/60 font-mono">
            Node Limit
          </span>
        );
      case 'Infeasible':
        return (
          <span className="px-2 py-0.5 rounded-[4px] text-[11px] font-medium bg-rose-950/80 text-rose-300 border border-rose-700/60 font-mono">
            Infeasible
          </span>
        );
      case 'Unbounded':
        return (
          <span className="px-2 py-0.5 rounded-[4px] text-[11px] font-medium bg-purple-950/80 text-purple-300 border border-purple-700/60 font-mono">
            Unbounded
          </span>
        );
      case 'NumericalFailure':
        return (
          <span className="px-2 py-0.5 rounded-[4px] text-[11px] font-medium bg-rose-950/80 text-rose-300 border border-rose-700/60 font-mono">
            Numerical Failure
          </span>
        );
      case 'UserInterrupted':
        return (
          <span className="px-2 py-0.5 rounded-[4px] text-[11px] font-medium bg-amber-950/80 text-amber-300 border border-amber-700/60 font-mono">
            User Interrupted
          </span>
        );
      case 'Solving':
        return (
          <span className="px-2 py-0.5 rounded-[4px] text-[11px] font-medium bg-amber-950/80 text-amber-300 border border-amber-700/60 font-mono">
            Solving
          </span>
        );
      default:
        return (
          <span className="px-2 py-0.5 rounded-[4px] text-[11px] font-medium bg-white/[0.05] text-slate-300 border border-white/[0.10] font-mono">
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
    <div className="p-3.5 border-b border-white/[0.08] space-y-3 text-slate-200">
      {/* Header Status Row */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2.5">
          <span className="text-xs font-semibold text-white tracking-tight">Solution Result</span>
          {renderStatusBadge(result.status_name)}
        </div>
        <div className="font-mono text-xs text-slate-400">
          Runtime: <span className="text-white font-medium tabular-nums">{formattedTime}</span>
        </div>
      </div>

      {/* Property-Table Style Scientific Metrics */}
      <div className="bg-[#06080c] rounded-[5px] border border-white/[0.08] overflow-hidden font-mono text-xs">
        <table className="w-full text-left">
          <tbody className="divide-y divide-white/[0.05]">
            <tr className="hover:bg-white/[0.02] transition-colors">
              <td className="py-2 px-3 text-slate-400 font-sans text-xs w-36">Objective Value</td>
              <td className="py-2 px-3 text-white font-medium text-sm tabular-nums">
                {result.objective_value !== undefined ? result.objective_value.toFixed(6) : '—'}
              </td>
            </tr>
            <tr className="hover:bg-white/[0.02] transition-colors">
              <td className="py-1.5 px-3 text-slate-400 font-sans text-xs">Best Bound</td>
              <td className="py-1.5 px-3 text-slate-200 font-medium tabular-nums">
                {result.best_bound !== undefined ? result.best_bound.toFixed(6) : '—'}
              </td>
            </tr>
            <tr className="hover:bg-white/[0.02] transition-colors">
              <td className="py-1.5 px-3 text-slate-400 font-sans text-xs">MIP Gap</td>
              <td className="py-1.5 px-3 text-slate-200 font-medium tabular-nums">
                {result.mip_gap !== undefined ? `${(result.mip_gap * 100).toFixed(2)}%` : '—'}
              </td>
            </tr>
            <tr className="hover:bg-white/[0.02] transition-colors">
              <td className="py-1.5 px-3 text-slate-400 font-sans text-xs">B&amp;B Nodes</td>
              <td className="py-1.5 px-3 text-slate-300 tabular-nums">
                <span className="text-white font-medium">{result.nodes_explored ?? 0}</span> explored
                {result.nodes_created !== undefined && ` · ${result.nodes_created} created`}
                {openNodes !== undefined && ` · ${openNodes} open`}
              </td>
            </tr>
            <tr className="hover:bg-white/[0.02] transition-colors">
              <td className="py-1.5 px-3 text-slate-400 font-sans text-xs">LP Relaxations</td>
              <td className="py-1.5 px-3 text-slate-300 tabular-nums">
                {result.lp_solves !== undefined ? result.lp_solves : '—'}
              </td>
            </tr>
          </tbody>
        </table>
      </div>

      {/* Solver Message / Failure details */}
      {result.message && (
        <div className="text-[11px] font-mono bg-[#06080c] p-2.5 rounded-[5px] border border-white/[0.06] text-slate-300 leading-relaxed">
          <span className="text-slate-500 font-sans font-medium">Message:</span> {result.message}
        </div>
      )}
    </div>
  );
};
