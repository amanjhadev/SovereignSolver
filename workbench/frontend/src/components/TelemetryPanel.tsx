import React from 'react';
import { SolverTelemetry } from '../types';

interface TelemetryPanelProps {
  telemetry?: SolverTelemetry;
}

export const TelemetryPanel: React.FC<TelemetryPanelProps> = ({ telemetry }) => {
  if (!telemetry) {
    return null;
  }

  const hasDiagnostics =
    telemetry.root_lp_objective !== undefined ||
    telemetry.root_lp_iterations !== undefined ||
    telemetry.fractional_binaries !== undefined ||
    telemetry.cuts_generated !== undefined ||
    telemetry.cuts_accepted !== undefined ||
    telemetry.heuristic_incumbents !== undefined;

  if (!hasDiagnostics) {
    return null;
  }

  return (
    <div className="p-3.5 space-y-2.5 border-b border-white/[0.08] text-xs text-slate-200">
      <div className="flex items-center justify-between">
        <span className="font-semibold text-white tracking-tight">Engine Diagnostics</span>
        <span className="font-mono text-[10px] px-2 py-0.5 rounded-[4px] bg-white/[0.04] text-slate-400 border border-white/[0.06]">
          Root &amp; Cuts
        </span>
      </div>

      <div className="bg-[#06080c] rounded-[5px] border border-white/[0.08] overflow-hidden font-mono text-xs">
        <table className="w-full text-left">
          <tbody className="divide-y divide-white/[0.05]">
            {telemetry.root_lp_objective !== undefined && (
              <tr className="hover:bg-white/[0.02] transition-colors">
                <td className="py-1.5 px-3 text-slate-400 font-sans text-xs w-48">Root LP Objective</td>
                <td className="py-1.5 px-3 text-white font-medium tabular-nums">
                  {telemetry.root_lp_objective.toFixed(6)}
                </td>
              </tr>
            )}
            {telemetry.root_lp_iterations !== undefined && (
              <tr className="hover:bg-white/[0.02] transition-colors">
                <td className="py-1.5 px-3 text-slate-400 font-sans text-xs">Root LP Iterations</td>
                <td className="py-1.5 px-3 text-slate-200 tabular-nums">
                  {telemetry.root_lp_iterations}
                </td>
              </tr>
            )}
            {telemetry.fractional_binaries !== undefined && (
              <tr className="hover:bg-white/[0.02] transition-colors">
                <td className="py-1.5 px-3 text-slate-400 font-sans text-xs">Root Fractional Binaries</td>
                <td className="py-1.5 px-3 text-slate-200 tabular-nums">
                  {telemetry.fractional_binaries}
                </td>
              </tr>
            )}
            {(telemetry.cuts_generated !== undefined || telemetry.cuts_accepted !== undefined) && (
              <tr className="hover:bg-white/[0.02] transition-colors">
                <td className="py-1.5 px-3 text-slate-400 font-sans text-xs">Cuts (Generated / Accepted)</td>
                <td className="py-1.5 px-3 text-slate-200 tabular-nums">
                  {telemetry.cuts_generated ?? 0} / {telemetry.cuts_accepted ?? 0}
                </td>
              </tr>
            )}
            {telemetry.heuristic_incumbents !== undefined && (
              <tr className="hover:bg-white/[0.02] transition-colors">
                <td className="py-1.5 px-3 text-slate-400 font-sans text-xs">Heuristic Incumbents</td>
                <td className="py-1.5 px-3 text-slate-200 tabular-nums">
                  {telemetry.heuristic_incumbents}
                </td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
};
