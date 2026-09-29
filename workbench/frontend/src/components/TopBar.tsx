import React, { useState, useEffect } from 'react';
import { Info, X, Cpu } from 'lucide-react';
import { BackendHealth } from '../types';

interface TopBarProps {
  health: BackendHealth;
  onOpenSettings?: () => void;
}

export const TopBar: React.FC<TopBarProps> = ({ health }) => {
  const [showInfoModal, setShowInfoModal] = useState(false);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape' && showInfoModal) {
        setShowInfoModal(false);
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [showInfoModal]);

  return (
    <>
      <header className="h-11 bg-[#090d14] border-b border-white/[0.08] px-4 flex items-center justify-between select-none z-20 shrink-0">
        {/* Brand & Identity */}
        <div className="flex items-center gap-3.5">
          <div className="flex items-center gap-2">
            <div className="w-5 h-5 rounded-[4px] bg-gradient-to-br from-white/20 via-white/5 to-transparent border border-white/20 flex items-center justify-center shadow-sm">
              <div className="w-2 h-2 rotate-45 border-t border-l border-white/90"></div>
            </div>
            <span className="font-semibold text-white text-[13px] tracking-[0.08em] font-sans">
              SOVEREIGN
            </span>
          </div>
          <div className="h-3.5 w-px bg-white/[0.12] hidden sm:block"></div>
          <span className="text-[11px] text-slate-400 font-normal hidden sm:inline tracking-normal">
            Optimization Workbench
          </span>
        </div>

        {/* System & Telemetry Controls */}
        <div className="flex items-center gap-4 text-xs">
          {/* Subtle Backend Connection */}
          <div className="flex items-center gap-2 px-2 py-1 rounded bg-white/[0.03] border border-white/[0.05]">
            <span
              className={`w-1.5 h-1.5 rounded-full ${
                health.checking
                  ? 'bg-amber-400 animate-pulse'
                  : health.connected
                  ? 'bg-emerald-400'
                  : 'bg-rose-500'
              }`}
            ></span>
            <span className="text-[11px] font-mono text-slate-300">
              {health.checking
                ? 'Connecting...'
                : health.connected
                ? 'Backend Connected'
                : 'Backend Disconnected'}
            </span>
          </div>

          {/* Engine Capability Spec */}
          <div className="hidden md:flex items-center gap-1.5 text-slate-400 font-mono text-[11px] border-l border-white/[0.08] pl-3.5">
            <Cpu className="w-3.5 h-3.5 text-slate-500" />
            <span>C++ Core v0.2.0</span>
            <span className="text-slate-600">·</span>
            <span className="text-slate-400">Dual Simplex / B&amp;B</span>
          </div>

          {/* System Specification Info */}
          <button
            onClick={() => setShowInfoModal(true)}
            className="p-1.5 text-slate-400 hover:text-slate-200 hover:bg-white/[0.06] rounded transition-colors"
            title="System Specifications & Architecture"
            aria-label="System Specifications"
          >
            <Info className="w-3.5 h-3.5" />
          </button>
        </div>
      </header>

      {/* Specifications Modal */}
      {showInfoModal && (
        <div className="fixed inset-0 z-50 bg-black/70 backdrop-blur-[6px] flex items-center justify-center p-4">
          <div className="sov-glass rounded-lg max-w-md w-full p-5 space-y-4 shadow-2xl text-slate-200 border border-white/[0.12]">
            <div className="flex items-center justify-between border-b border-white/[0.08] pb-3">
              <div>
                <h3 className="font-semibold text-sm text-white tracking-tight">Sovereign Optimization Core</h3>
                <p className="text-[11px] text-slate-400 mt-0.5">High-Performance C++20 Dual Simplex &amp; MILP Engine</p>
              </div>
              <button
                onClick={() => setShowInfoModal(false)}
                className="text-slate-400 hover:text-white p-1 rounded hover:bg-white/[0.08] transition-colors"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            <div className="space-y-3 text-xs leading-relaxed text-slate-300">
              <p>
                Native standalone optimization solver designed for linear programming (LP) and mixed-integer linear programming (MILP).
              </p>

              <div className="bg-[#07090e] p-3 rounded border border-white/[0.06] space-y-2 font-mono text-[11px]">
                <div className="flex justify-between items-center py-0.5 border-b border-white/[0.04]">
                  <span className="text-slate-500 font-sans">Core Architecture:</span>
                  <span className="text-slate-200 font-medium">Native C++20 Executable</span>
                </div>
                <div className="flex justify-between items-center py-0.5 border-b border-white/[0.04]">
                  <span className="text-slate-500 font-sans">LP Core Solver:</span>
                  <span className="text-slate-200">Dual Simplex + SparseLU + Devex</span>
                </div>
                <div className="flex justify-between items-center py-0.5 border-b border-white/[0.04]">
                  <span className="text-slate-500 font-sans">MILP Core Search:</span>
                  <span className="text-slate-200">Branch &amp; Bound + Heuristics + Cuts</span>
                </div>
                <div className="flex justify-between items-center py-0.5 border-b border-white/[0.04]">
                  <span className="text-slate-500 font-sans">IPC / Telemetry:</span>
                  <span className="text-slate-200">SSE Event Stream + REST Gateway</span>
                </div>
                <div className="flex justify-between items-center py-0.5">
                  <span className="text-slate-500 font-sans">Engine Status:</span>
                  <span className={`font-medium ${health.connected ? 'text-emerald-400' : 'text-rose-400'}`}>
                    {health.connected ? 'Online (Ready)' : 'Disconnected'}
                  </span>
                </div>
              </div>
            </div>

            <div className="flex justify-end pt-1">
              <button
                onClick={() => setShowInfoModal(false)}
                className="px-4 py-1.5 bg-white/[0.08] hover:bg-white/[0.14] text-slate-200 rounded text-xs font-medium border border-white/[0.08] transition-colors"
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
