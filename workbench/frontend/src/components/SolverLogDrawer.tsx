import React from 'react';
import { Terminal, X, Info } from 'lucide-react';
import { SolverLogItem } from '../types';

interface SolverLogDrawerProps {
  isOpen: boolean;
  onClose: () => void;
  logs?: SolverLogItem[];
  modelName?: string;
}

export const SolverLogDrawer: React.FC<SolverLogDrawerProps> = ({ isOpen, onClose, logs, modelName }) => {
  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 z-50 bg-slate-950/80 backdrop-blur-sm flex justify-end">
      <div className="w-full max-w-xl bg-slate-900 border-l border-slate-800 h-full flex flex-col shadow-2xl animate-in slide-in-from-right duration-300">
        {/* Drawer Header */}
        <div className="p-4 border-b border-slate-800 flex items-center justify-between bg-slate-950">
          <div className="flex items-center gap-2">
            <Terminal className="w-5 h-5 text-indigo-400" />
            <div>
              <h3 className="font-bold text-sm text-slate-100">Solver Log Output</h3>
              <p className="text-[11px] font-mono text-slate-400">{modelName || 'Active Model Session'}</p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="p-1.5 text-slate-400 hover:text-white rounded bg-slate-800 hover:bg-slate-700 transition-colors"
          >
            <X className="w-4 h-4" />
          </button>
        </div>

        {/* Drawer Body */}
        <div className="flex-1 p-4 overflow-y-auto font-mono text-xs space-y-2 bg-slate-950/90 text-slate-200">
          {!logs || logs.length === 0 ? (
            <div className="p-8 text-center text-slate-500 flex flex-col items-center justify-center h-full space-y-2">
              <Info className="w-8 h-8 text-slate-700" />
              <p className="font-sans text-slate-400 text-sm">Solver log unavailable for this run</p>
              <p className="text-[11px] text-slate-600 max-w-xs">
                The Sovereign C++ CLI engine outputs structured JSON metrics to standard stdout.
              </p>
            </div>
          ) : (
            logs.map((log, index) => (
              <div
                key={index}
                className="p-2.5 rounded border border-slate-800/80 bg-slate-900/60 flex items-start gap-3 hover:bg-slate-900 transition-colors"
              >
                <span className="text-[10px] text-slate-500 shrink-0 font-bold">{log.timestamp}</span>
                <span className="text-[10px] px-1.5 py-0.5 rounded bg-indigo-950 text-indigo-300 border border-indigo-800/60 shrink-0 uppercase">
                  {log.phase}
                </span>
                <span className="text-slate-200 leading-relaxed flex-1">{log.message}</span>
              </div>
            ))
          )}
        </div>

        {/* Drawer Footer */}
        <div className="p-3 border-t border-slate-800 bg-slate-950 flex justify-end text-xs">
          <button
            onClick={onClose}
            className="px-4 py-1.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded font-bold"
          >
            Close Log
          </button>
        </div>
      </div>
    </div>
  );
};
