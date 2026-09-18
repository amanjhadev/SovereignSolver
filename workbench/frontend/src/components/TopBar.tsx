import React, { useState } from 'react';
import { Info, X } from 'lucide-react';
import { BackendHealth } from '../types';

interface TopBarProps {
  health: BackendHealth;
  onOpenSettings?: () => void;
}

export const TopBar: React.FC<TopBarProps> = ({ health }) => {
  const [showInfoModal, setShowInfoModal] = useState(false);

  return (
    <>
      <header className="h-12 bg-slate-900 border-b border-slate-800 px-4 flex items-center justify-between select-none z-20 shrink-0">
        {/* Title Bar Left */}
        <div className="flex items-center gap-3">
          <div className="flex items-baseline gap-2">
            <span className="font-bold text-slate-100 text-sm tracking-wide">SOVEREIGN</span>
            <span className="text-xs text-slate-400 font-normal">Optimization Workbench</span>
          </div>
        </div>

        {/* Title Bar Right */}
        <div className="flex items-center gap-4 text-xs">
          {/* Unobtrusive Backend Status Indicator */}
          <div className="flex items-center gap-2 text-slate-300">
            {health.checking ? (
              <>
                <span className="w-2 h-2 rounded-full bg-amber-400"></span>
                <span className="text-slate-400 text-xs font-mono">Connecting...</span>
              </>
            ) : health.connected ? (
              <>
                <span className="w-2 h-2 rounded-full bg-emerald-400"></span>
                <span className="text-slate-300 text-xs font-mono">Backend Connected</span>
              </>
            ) : (
              <>
                <span className="w-2 h-2 rounded-full bg-rose-500"></span>
                <span className="text-rose-400 text-xs font-mono">Backend Offline</span>
              </>
            )}
          </div>

          {/* Secondary Engine / Mode Information */}
          <div className="hidden sm:flex items-center text-slate-500 font-mono text-[11px] border-l border-slate-800 pl-3">
            <span>v0.2.0 · Dual Simplex / MILP</span>
          </div>

          {/* System Specification Info Button */}
          <button
            onClick={() => setShowInfoModal(true)}
            className="p-1.5 text-slate-400 hover:text-slate-200 hover:bg-slate-800 rounded transition-colors"
            title="System Specifications"
          >
            <Info className="w-4 h-4" />
          </button>
        </div>
      </header>

      {/* Specifications Modal */}
      {showInfoModal && (
        <div className="fixed inset-0 z-50 bg-slate-950/70 flex items-center justify-center p-4">
          <div className="bg-slate-900 border border-slate-800 rounded-md p-5 max-w-md w-full shadow-lg space-y-4 font-sans text-slate-200">
            <div className="flex items-center justify-between border-b border-slate-800 pb-3">
              <div>
                <h3 className="font-semibold text-sm text-slate-100">Sovereign Optimization Workbench</h3>
                <p className="text-xs text-slate-400 mt-0.5">System Architecture & Solver Specifications</p>
              </div>
              <button
                onClick={() => setShowInfoModal(false)}
                className="text-slate-400 hover:text-slate-200 p-1 rounded hover:bg-slate-800"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            <div className="space-y-3 text-xs">
              <p className="text-slate-300 leading-relaxed">
                Mathematical optimization engine powered by the native Sovereign C++ core.
              </p>

              <div className="bg-slate-950 p-3 rounded border border-slate-800 space-y-1.5 font-mono text-[11px]">
                <div className="flex justify-between">
                  <span className="text-slate-500">Core Engine:</span>
                  <span className="text-slate-200">Sovereign C++20</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">LP Algorithm:</span>
                  <span className="text-slate-200">Dual Simplex + SparseLU + Devex</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">MILP Engine:</span>
                  <span className="text-slate-200">Parallel B&B + Feasibility Pump + Cuts</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">Backend Server:</span>
                  <span className="text-slate-200">Express API (port 3001)</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">Connection:</span>
                  <span className={health.connected ? 'text-emerald-400' : 'text-rose-400'}>
                    {health.connected ? 'Online' : 'Offline'}
                  </span>
                </div>
              </div>
            </div>

            <div className="flex justify-end pt-1">
              <button
                onClick={() => setShowInfoModal(false)}
                className="px-3.5 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded text-xs font-medium transition-colors"
              >
                Close
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  );
};
