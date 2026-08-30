import React from 'react';
import { Cpu, Zap, Layers, Activity } from 'lucide-react';
import { SolverTelemetry } from '../types';

interface TelemetryPanelProps {
  telemetry?: SolverTelemetry;
}

export const TelemetryPanel: React.FC<TelemetryPanelProps> = ({ telemetry }) => {
  if (!telemetry) {
    return (
      <div className="p-4 text-slate-500 font-mono text-xs text-center border-b border-slate-800">
        Telemetry unavailable for this run.
      </div>
    );
  }

  return (
    <div className="p-4 space-y-3 border-b border-slate-800">
      <div className="flex items-center gap-2 text-xs font-bold text-slate-300 uppercase tracking-wider">
        <Cpu className="w-4 h-4 text-indigo-400" />
        <span>Solver Telemetry & Engine Diagnostics</span>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-3 text-xs font-mono">
        {/* Model Dimensions */}
        <div className="bg-slate-900 p-3 rounded-lg border border-slate-800 space-y-2">
          <div className="flex items-center gap-1.5 text-slate-300 font-semibold mb-1 border-b border-slate-800 pb-1.5">
            <Layers className="w-3.5 h-3.5 text-indigo-400" />
            <span>Model Dimensions</span>
          </div>
          <div className="flex justify-between text-slate-400">
            <span>Total Variables:</span>
            <span className="text-slate-100 font-bold">{telemetry.variables}</span>
          </div>
          <div className="flex justify-between text-slate-400">
            <span>Binary / Continuous:</span>
            <span className="text-slate-200">
              {telemetry.binaries} / {telemetry.continuous}
            </span>
          </div>
          <div className="flex justify-between text-slate-400">
            <span>Constraints / NNZ:</span>
            <span className="text-slate-200">
              {telemetry.constraints} / {telemetry.nnz}
            </span>
          </div>
          <div className="flex justify-between text-slate-400">
            <span>Objective Sense:</span>
            <span className="text-indigo-400 font-bold">{telemetry.sense}</span>
          </div>
        </div>

        {/* Root LP & Heuristics */}
        <div className="bg-slate-900 p-3 rounded-lg border border-slate-800 space-y-2">
          <div className="flex items-center gap-1.5 text-slate-300 font-semibold mb-1 border-b border-slate-800 pb-1.5">
            <Zap className="w-3.5 h-3.5 text-emerald-400" />
            <span>Root LP & Heuristics</span>
          </div>
          <div className="flex justify-between text-slate-400">
            <span>Root LP Objective:</span>
            <span className="text-emerald-400 font-bold">
              {telemetry.root_lp_objective !== undefined ? telemetry.root_lp_objective.toFixed(6) : 'N/A'}
            </span>
          </div>
          <div className="flex justify-between text-slate-400">
            <span>Root LP Iterations:</span>
            <span className="text-slate-200">{telemetry.root_lp_iterations ?? 'N/A'}</span>
          </div>
          <div className="flex justify-between text-slate-400">
            <span>Root Fractional Binaries:</span>
            <span className={`font-bold ${telemetry.fractional_binaries ? 'text-amber-400' : 'text-emerald-400'}`}>
              {telemetry.fractional_binaries ?? 'N/A'}
            </span>
          </div>
          <div className="flex justify-between text-slate-400">
            <span>Cuts Generated / Accepted:</span>
            <span className="text-slate-200">
              {telemetry.cuts_generated ?? 0} / {telemetry.cuts_accepted ?? 0}
            </span>
          </div>
          <div className="flex justify-between text-slate-400">
            <span>Heuristic Candidates:</span>
            <span className="text-indigo-300 font-bold">{telemetry.heuristic_incumbents ?? 0}</span>
          </div>
        </div>
      </div>
    </div>
  );
};
