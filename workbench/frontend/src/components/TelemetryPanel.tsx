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
    <div className="p-3.5 space-y-2.5 border-b border-slate-800 text-xs">
      <div className="flex items-center justify-between">
        <span className="font-semibold text-slate-200">Engine Diagnostics</span>
        <span className="font-mono text-[11px] text-slate-500">Root & Cuts</span>
      </div>

      <div className="bg-slate-950 rounded border border-slate-800 overflow-hidden font-mono text-xs">
        <table className="w-full text-left">
          <tbody className="divide-y divide-slate-800/80">
            {telemetry.root_lp_objective !== undefined && (
              <tr className="hover:bg-slate-900/40">
                <td className="py-1.5 px-3 text-slate-400 font-sans text-[11px] w-44">Root LP Objective</td>
                <td className="py-1.5 px-3 text-slate-100 font-medium">
                  {telemetry.root_lp_objective.toFixed(6)}
                </td>
              </tr>
            )}
            {telemetry.root_lp_iterations !== undefined && (
              <tr className="hover:bg-slate-900/40">
                <td className="py-1.5 px-3 text-slate-400 font-sans text-[11px]">Root LP Iterations</td>
                <td className="py-1.5 px-3 text-slate-300">{telemetry.root_lp_iterations}</td>
              </tr>
            )}
            {telemetry.fractional_binaries !== undefined && (
              <tr className="hover:bg-slate-900/40">
                <td className="py-1.5 px-3 text-slate-400 font-sans text-[11px]">Root Fractional Binaries</td>
                <td className="py-1.5 px-3 text-slate-300">{telemetry.fractional_binaries}</td>
              </tr>
            )}
            {(telemetry.cuts_generated !== undefined || telemetry.cuts_accepted !== undefined) && (
              <tr className="hover:bg-slate-900/40">
                <td className="py-1.5 px-3 text-slate-400 font-sans text-[11px]">Cuts (Generated / Accepted)</td>
                <td className="py-1.5 px-3 text-slate-300">
                  {telemetry.cuts_generated ?? 0} / {telemetry.cuts_accepted ?? 0}
                </td>
              </tr>
            )}
            {telemetry.heuristic_incumbents !== undefined && (
              <tr className="hover:bg-slate-900/40">
                <td className="py-1.5 px-3 text-slate-400 font-sans text-[11px]">Heuristic Incumbents</td>
                <td className="py-1.5 px-3 text-slate-300">{telemetry.heuristic_incumbents}</td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
};
