import React, { useState } from 'react';
import { Cpu, Info, CheckCircle2, AlertTriangle, XCircle, SlidersHorizontal, Layers } from 'lucide-react';
import { BackendHealth } from '../types';

interface TopBarProps {
  health: BackendHealth;
  onOpenSettings?: () => void;
}

export const TopBar: React.FC<TopBarProps> = ({ health, onOpenSettings }) => {
  const [showInfoModal, setShowInfoModal] = useState(false);

  return (
    <>
      <header className="h-14 bg-slate-900 border-b border-slate-800 px-4 flex items-center justify-between select-none z-20 shrink-0">
        {/* Branding Left */}
        <div className="flex items-center gap-3">
          <div className="p-2 rounded-lg bg-indigo-600/20 text-indigo-400 border border-indigo-500/30">
            <Cpu className="w-5 h-5" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <span className="font-extrabold text-slate-100 tracking-wider text-sm uppercase">Sovereign</span>
              <span className="text-[10px] px-1.5 py-0.5 rounded bg-indigo-950 text-indigo-300 font-mono border border-indigo-800/60">
                CLI v0.2.0
              </span>
            </div>
            <p className="text-[11px] text-slate-400 font-mono leading-tight">Optimization Workbench</p>
          </div>
        </div>

        {/* Right Controls */}
        <div className="flex items-center gap-4">
          {/* Connection Indicator */}
          <div className="flex items-center gap-2 bg-slate-950/80 px-3 py-1.5 rounded-lg border border-slate-800 text-xs font-mono">
            {health.checking ? (
              <>
                <span className="w-2.5 h-2.5 rounded-full bg-amber-400 animate-ping"></span>
                <span className="text-amber-300">Backend Connecting...</span>
              </>
            ) : health.connected ? (
              <>
                <span className="w-2.5 h-2.5 rounded-full bg-emerald-400 shadow-[0_0_8px_rgba(52,211,153,0.6)]"></span>
                <span className="text-emerald-300 font-semibold flex items-center gap-1">
                  Backend <span className="text-slate-500">•</span> Connected
                </span>
              </>
            ) : (
              <>
                <span className="w-2.5 h-2.5 rounded-full bg-rose-500 shadow-[0_0_8px_rgba(244,63,94,0.6)]"></span>
                <span className="text-rose-400 font-semibold">Backend Disconnected</span>
              </>
            )}
          </div>

          {/* Engine Version Badge */}
          <div className="hidden sm:flex items-center gap-1.5 text-xs text-slate-400 bg-slate-950/40 px-2.5 py-1.5 rounded-lg border border-slate-800/80 font-mono">
            <Layers className="w-3.5 h-3.5 text-indigo-400" />
            <span>DualSimplex + ParallelMILP</span>
          </div>

          {/* Info Modal Button */}
          <button
            onClick={() => setShowInfoModal(true)}
            className="p-2 text-slate-400 hover:text-slate-200 hover:bg-slate-800 rounded-lg transition-colors"
            title="System Info & Specifications"
          >
            <Info className="w-4 h-4" />
          </button>
        </div>
      </header>

      {/* Info Modal */}
      {showInfoModal && (
        <div className="fixed inset-0 z-50 bg-slate-950/80 backdrop-blur-sm flex items-center justify-center p-4">
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 max-w-lg w-full shadow-2xl space-y-4 font-sans text-slate-200">
            <div className="flex items-center justify-between border-b border-slate-800 pb-3">
              <div className="flex items-center gap-2">
                <Cpu className="w-5 h-5 text-indigo-400" />
                <h3 className="font-bold text-base text-white">Sovereign Optimization Workbench</h3>
              </div>
              <button
                onClick={() => setShowInfoModal(false)}
                className="text-slate-400 hover:text-white text-xs px-2 py-1 bg-slate-800 rounded"
              >
                Close
              </button>
            </div>

            <div className="space-y-3 text-xs">
              <p className="text-slate-300 leading-relaxed">
                Industrial-grade LP/MILP mathematical optimization GUI powered by the Sovereign C++ solver core engine.
              </p>

              <div className="bg-slate-950 p-3 rounded-lg border border-slate-800 space-y-2 font-mono text-[11px]">
                <div className="flex justify-between">
                  <span className="text-slate-500">Core Engine:</span>
                  <span className="text-indigo-300">Sovereign C++20 (MSVC/GCC)</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">LP Algorithm:</span>
                  <span className="text-indigo-300">Dual Simplex + SparseLU + Devex</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">MILP Engine:</span>
                  <span className="text-indigo-300">Parallel B&B + Feasibility Pump + Cuts</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">Backend Server:</span>
                  <span className="text-indigo-300">Node.js Express (http://localhost:3001)</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">Backend Status:</span>
                  <span className={health.connected ? 'text-emerald-400 font-bold' : 'text-rose-400 font-bold'}>
                    {health.connected ? 'Connected & Ready' : 'Disconnected'}
                  </span>
                </div>
              </div>
            </div>

            <div className="flex justify-end pt-2">
              <button
                onClick={() => setShowInfoModal(false)}
                className="px-4 py-2 bg-indigo-600 hover:bg-indigo-500 text-white rounded-lg text-xs font-semibold"
              >
                Done
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  );
};
