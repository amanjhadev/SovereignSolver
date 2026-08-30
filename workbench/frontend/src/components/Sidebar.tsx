import React from 'react';
import { PlusCircle, FileCode, Box, Cpu } from 'lucide-react';
import { PRESET_MODELS, PresetModel } from '../samples/preset_models';
import { BackendHealth } from '../types';

interface SidebarProps {
  activePresetId: string;
  onSelectPreset: (preset: PresetModel) => void;
  onNewProblem: () => void;
  health: BackendHealth;
}

export const Sidebar: React.FC<SidebarProps> = ({
  activePresetId,
  onSelectPreset,
  onNewProblem,
  health
}) => {
  return (
    <aside className="w-64 bg-slate-900 border-r border-slate-800 flex flex-col h-full select-none shrink-0">
      {/* Action Header */}
      <div className="p-4 border-b border-slate-800">
        <button
          onClick={onNewProblem}
          className="w-full flex items-center justify-center gap-2 py-2.5 px-4 rounded-lg bg-indigo-600 hover:bg-indigo-500 text-white font-bold text-xs tracking-wide uppercase transition-all shadow-lg shadow-indigo-600/20 active:scale-[0.98]"
        >
          <PlusCircle className="w-4 h-4" />
          <span>+ New Problem</span>
        </button>
      </div>

      {/* Models List */}
      <div className="flex-1 overflow-y-auto px-3 py-3 space-y-6">
        <div>
          <div className="px-3 mb-2 text-[10px] font-bold text-slate-400 uppercase tracking-wider">
            Recent / Saved Benchmarks
          </div>
          <div className="space-y-1">
            {PRESET_MODELS.map((preset) => {
              const isActive = activePresetId === preset.id;
              return (
                <button
                  key={preset.id}
                  onClick={() => onSelectPreset(preset)}
                  className={`w-full text-left px-3 py-2.5 rounded-lg transition-all flex items-start gap-3 ${
                    isActive
                      ? 'bg-indigo-600/15 text-indigo-200 border border-indigo-500/40 shadow-sm'
                      : 'text-slate-400 hover:bg-slate-800/60 hover:text-slate-200'
                  }`}
                >
                  <div className="mt-0.5 shrink-0">
                    {preset.type === 'structured' ? (
                      <FileCode className={`w-4 h-4 ${isActive ? 'text-indigo-400' : 'text-slate-500'}`} />
                    ) : (
                      <Box className={`w-4 h-4 ${isActive ? 'text-indigo-400' : 'text-slate-500'}`} />
                    )}
                  </div>

                  <div className="min-w-0 flex-1">
                    <div className="text-xs font-semibold truncate text-slate-100">{preset.name}</div>
                    {preset.varsSummary && (
                      <div className="text-[10px] font-mono text-indigo-400 mt-0.5 truncate font-medium">
                        {preset.varsSummary}
                      </div>
                    )}
                    <div className="text-[10px] text-slate-500 truncate mt-0.5">{preset.description}</div>
                  </div>
                </button>
              );
            })}
          </div>
        </div>
      </div>

      {/* Footer Info */}
      <div className="p-3 border-t border-slate-800 bg-slate-950/60 text-[11px] text-slate-400 flex items-center justify-between font-mono">
        <span className="flex items-center gap-1.5">
          <span className={`w-2 h-2 rounded-full ${health.connected ? 'bg-emerald-400 animate-pulse' : 'bg-rose-500'}`}></span>
          <span>{health.connected ? 'Backend Online' : 'Offline'}</span>
        </span>
        <span className="text-slate-500">C++ v0.2.0</span>
      </div>
    </aside>
  );
};
