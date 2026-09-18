import React from 'react';
import { Terminal, X } from 'lucide-react';
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
    <div className="fixed inset-0 z-50 bg-slate-950/70 flex justify-end">
      <div className="w-full max-w-xl bg-slate-900 border-l border-slate-800 h-full flex flex-col shadow-xl animate-in slide-in-from-right duration-200">
        {/* Drawer Header */}
        <div className="p-3.5 border-b border-slate-800 flex items-center justify-between bg-slate-950">
          <div className="flex items-center gap-2">
            <Terminal className="w-4 h-4 text-slate-400" />
            <div>
              <h3 className="font-semibold text-xs text-slate-100">Solver Log</h3>
              <p className="text-[11px] font-mono text-slate-400">{modelName || 'Session'}</p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="p-1 text-slate-400 hover:text-slate-200 rounded hover:bg-slate-800 transition-colors"
          >
            <X className="w-4 h-4" />
          </button>
        </div>

        {/* Drawer Body */}
        <div className="flex-1 p-3.5 overflow-y-auto font-mono text-xs space-y-1.5 bg-slate-950 text-slate-200">
          {!logs || logs.length === 0 ? (
            <div className="p-8 text-center text-slate-500 flex flex-col items-center justify-center h-full space-y-2">
              <p className="font-sans text-slate-400 text-xs">No solver log events recorded</p>
              <p className="text-[11px] text-slate-600 max-w-xs font-mono">
                Log events stream here during solver execution.
              </p>
            </div>
          ) : (
            logs.map((log, index) => (
              <div
                key={index}
                className="p-2 rounded border border-slate-850 bg-slate-900/70 flex items-start gap-2.5 hover:bg-slate-900 transition-colors"
              >
                <span className="text-[10px] text-slate-500 shrink-0">{log.timestamp}</span>
                <span className="text-[10px] px-1.5 py-0.5 rounded bg-slate-800 text-slate-300 border border-slate-700 shrink-0 uppercase">
                  {log.phase}
                </span>
                <span className="text-slate-200 leading-relaxed flex-1 text-[11px]">{log.message}</span>
              </div>
            ))
          )}
        </div>

        {/* Drawer Footer */}
        <div className="p-2.5 border-t border-slate-800 bg-slate-950 flex justify-end text-xs">
          <button
            onClick={onClose}
            className="px-3 py-1 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded text-xs font-medium transition-colors"
          >
            Close
          </button>
        </div>
      </div>
    </div>
  );
};
