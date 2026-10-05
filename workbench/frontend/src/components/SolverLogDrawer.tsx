import React, { useState, useEffect } from 'react';
import { Terminal, X, Search, Copy, Check } from 'lucide-react';
import { SolverLogItem } from '../types';

interface SolverLogDrawerProps {
  isOpen: boolean;
  onClose: () => void;
  logs?: SolverLogItem[];
  modelName?: string;
}

export const SolverLogDrawer: React.FC<SolverLogDrawerProps> = ({
  isOpen,
  onClose,
  logs,
  modelName
}) => {
  const [filter, setFilter] = useState('');
  const [copied, setCopied] = useState(false);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape' && isOpen) {
        onClose();
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [isOpen, onClose]);

  if (!isOpen) return null;

  const filteredLogs = (logs || []).filter(
    (log) =>
      log.message.toLowerCase().includes(filter.toLowerCase()) ||
      log.phase.toLowerCase().includes(filter.toLowerCase())
  );

  const handleCopyLogs = () => {
    if (!logs || logs.length === 0) return;
    const text = logs
      .map((l) => `[${l.timestamp}] [${l.phase}] ${l.message}`)
      .join('\n');
    navigator.clipboard.writeText(text);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div className="fixed inset-0 z-50 bg-black/70 backdrop-blur-[4px] flex justify-end">
      <div className="w-full max-w-xl bg-[#090d14] border-l border-white/[0.12] h-full flex flex-col shadow-2xl animate-in slide-in-from-right duration-150 overflow-hidden">
        {/* Drawer Header */}
        <div className="p-3.5 border-b border-white/[0.08] flex items-center justify-between bg-[#06080c] shrink-0">
          <div className="flex items-center gap-2.5">
            <Terminal className="w-4 h-4 text-slate-400" />
            <div>
              <h3 className="font-semibold text-xs text-white">Solver Log Stream</h3>
              <p className="text-[11px] font-mono text-slate-400">{modelName || 'Session'}</p>
            </div>
          </div>
          <div className="flex items-center gap-2">
            {logs && logs.length > 0 && (
              <button
                onClick={handleCopyLogs}
                className="flex items-center gap-1 px-2.5 py-1 rounded-[4px] bg-white/[0.04] hover:bg-white/[0.08] text-slate-400 hover:text-white border border-white/[0.08] text-[11px] font-mono transition-colors cursor-pointer"
                title="Copy all logs to clipboard"
              >
                {copied ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3" />}
                <span>{copied ? 'Copied' : 'Copy'}</span>
              </button>
            )}
            <button
              onClick={onClose}
              className="p-1.5 text-slate-400 hover:text-white rounded-[4px] hover:bg-white/[0.08] transition-colors cursor-pointer"
              aria-label="Close log drawer"
            >
              <X className="w-4 h-4" />
            </button>
          </div>
        </div>

        {/* Filter bar */}
        {logs && logs.length > 0 && (
          <div className="p-2.5 border-b border-white/[0.06] bg-[#07090e] flex items-center gap-2 shrink-0">
            <div className="relative flex-1">
              <Search className="w-3 h-3 absolute left-2.5 top-2 text-slate-500" />
              <input
                type="text"
                value={filter}
                onChange={(e) => setFilter(e.target.value)}
                placeholder="Filter logs by message or phase..."
                className="w-full bg-[#06080c] border border-white/[0.08] rounded-[4px] pl-7 pr-2 py-1 text-xs font-mono text-slate-200 focus:outline-none focus:border-white/30"
              />
            </div>
            <span className="text-[10px] font-mono text-slate-500 shrink-0">
              {filteredLogs.length} of {logs.length}
            </span>
          </div>
        )}

        {/* Drawer Stream Body with strict min-h-0 and internal scrolling */}
        <div className="flex-1 min-h-0 min-w-0 p-3.5 overflow-y-auto font-mono text-xs space-y-1.5 bg-[#06080c] text-slate-200">
          {!logs || logs.length === 0 ? (
            <div className="p-8 text-center text-slate-500 flex flex-col items-center justify-center h-full space-y-2">
              <Terminal className="w-8 h-8 text-slate-700 mb-1" />
              <p className="font-sans text-slate-400 text-xs font-medium">No solver log events recorded</p>
              <p className="text-[11px] text-slate-600 max-w-xs font-mono">
                Real-time solver events and checkpoint telemetry will stream here during model execution.
              </p>
            </div>
          ) : filteredLogs.length === 0 ? (
            <div className="p-6 text-center text-slate-500 font-mono text-xs">
              No logs matching "{filter}"
            </div>
          ) : (
            filteredLogs.map((log, index) => (
              <div
                key={index}
                className="p-2 rounded-[4px] border border-white/[0.04] bg-white/[0.02] flex items-start gap-2.5 hover:bg-white/[0.04] transition-colors"
              >
                <span className="text-[10px] text-slate-400 shrink-0 tabular-nums font-mono mt-0.5">
                  {log.timestamp}
                </span>
                <span className="text-[9px] px-1.5 py-0.5 rounded-[3px] bg-white/[0.06] text-slate-300 border border-white/[0.08] shrink-0 uppercase font-mono font-medium">
                  {log.phase}
                </span>
                <span className="text-slate-200 leading-relaxed flex-1 text-[11px] select-text">
                  {log.message}
                </span>
              </div>
            ))
          )}
        </div>

        {/* Drawer Footer */}
        <div className="p-2.5 border-t border-white/[0.08] bg-[#07090e] flex justify-end text-xs shrink-0">
          <button
            onClick={onClose}
            className="px-3.5 py-1.5 bg-white/[0.06] hover:bg-white/[0.12] text-slate-200 rounded-[4px] text-xs font-medium transition-colors border border-white/[0.08] cursor-pointer"
          >
            Close
          </button>
        </div>
      </div>
    </div>
  );
};
