import React, { useState } from 'react';
import { Plus, FileCode, Box, Trash2, Lock } from 'lucide-react';
import { PresetModel, BackendHealth } from '../types';

interface SidebarProps {
  activePresetId: string;
  builtInPresets: PresetModel[];
  savedBenchmarks: PresetModel[];
  onSelectPreset: (preset: PresetModel) => void;
  onDeleteBenchmark: (benchmark: PresetModel) => void;
  onNewProblem: () => void;
  health: BackendHealth;
}

export const Sidebar: React.FC<SidebarProps> = ({
  activePresetId,
  builtInPresets,
  savedBenchmarks,
  onSelectPreset,
  onDeleteBenchmark,
  onNewProblem,
  health
}) => {
  const [benchmarkToDelete, setBenchmarkToDelete] = useState<PresetModel | null>(null);

  const handleConfirmDelete = () => {
    if (benchmarkToDelete) {
      onDeleteBenchmark(benchmarkToDelete);
      setBenchmarkToDelete(null);
    }
  };

  return (
    <>
      <aside className="w-64 bg-slate-900 border-r border-slate-800 flex flex-col h-full select-none shrink-0">
        {/* Action Header */}
        <div className="p-3 border-b border-slate-800">
          <button
            onClick={onNewProblem}
            className="w-full flex items-center justify-center gap-1.5 py-1.5 px-3 rounded border border-slate-700 bg-slate-800 hover:bg-slate-700 text-slate-200 font-medium text-xs transition-colors"
          >
            <Plus className="w-4 h-4 text-slate-300" />
            <span>New Problem</span>
          </button>
        </div>

        {/* Models List */}
        <div className="flex-1 overflow-y-auto p-2 space-y-4">
          {/* User-Saved Benchmarks */}
          <div>
            <div className="px-2 mb-1.5 text-[11px] font-semibold text-slate-400 uppercase tracking-wider flex items-center justify-between">
              <span>Saved Benchmarks</span>
              <span className="text-[10px] font-mono text-slate-500 font-normal">({savedBenchmarks.length})</span>
            </div>

            {savedBenchmarks.length === 0 ? (
              <div className="px-2 py-3 text-[11px] text-slate-500 font-sans italic">
                No user-saved models. Use Save in the editor to store custom models.
              </div>
            ) : (
              <div className="space-y-0.5">
                {savedBenchmarks.map((model) => {
                  const isActive = activePresetId === model.id;
                  return (
                    <div
                      key={model.id}
                      className={`group relative flex items-center justify-between rounded px-2.5 py-2 cursor-pointer transition-colors ${
                        isActive
                          ? 'bg-slate-800 text-slate-100 border-l-2 border-indigo-500'
                          : 'text-slate-300 hover:bg-slate-800/60 hover:text-slate-100'
                      }`}
                      onClick={() => onSelectPreset(model)}
                    >
                      <div className="flex items-start gap-2.5 min-w-0 flex-1">
                        <div className="mt-0.5 shrink-0 text-slate-400">
                          {model.type === 'structured' ? (
                            <FileCode className="w-3.5 h-3.5 text-slate-400" />
                          ) : (
                            <Box className="w-3.5 h-3.5 text-slate-400" />
                          )}
                        </div>
                        <div className="min-w-0 flex-1">
                          <div className="text-xs font-medium truncate text-slate-200">{model.name}</div>
                          <div className="text-[11px] font-mono text-slate-400 truncate mt-0.5">
                            {model.varsSummary || `${model.type.toUpperCase()} · User model`}
                          </div>
                        </div>
                      </div>

                      {/* Subtle hover delete button */}
                      <button
                        onClick={(e) => {
                          e.stopPropagation();
                          setBenchmarkToDelete(model);
                        }}
                        className="opacity-0 group-hover:opacity-100 p-1 rounded hover:bg-slate-700 text-slate-400 hover:text-rose-400 transition-opacity shrink-0 ml-1.5"
                        title="Delete saved benchmark"
                        aria-label="Delete saved benchmark"
                      >
                        <Trash2 className="w-3.5 h-3.5" />
                      </button>
                    </div>
                  );
                })}
              </div>
            )}
          </div>

          {/* Built-in Presets */}
          <div>
            <div className="px-2 mb-1.5 text-[11px] font-semibold text-slate-400 uppercase tracking-wider flex items-center justify-between">
              <span>Standard Benchmarks</span>
              <span className="text-[10px] font-mono text-slate-500 flex items-center gap-1 font-normal">
                <Lock className="w-2.5 h-2.5" /> Presets
              </span>
            </div>

            <div className="space-y-0.5">
              {builtInPresets.map((preset) => {
                const isActive = activePresetId === preset.id;
                return (
                  <button
                    key={preset.id}
                    onClick={() => onSelectPreset(preset)}
                    className={`w-full text-left rounded px-2.5 py-2 transition-colors flex items-start gap-2.5 ${
                      isActive
                        ? 'bg-slate-800 text-slate-100 border-l-2 border-indigo-500'
                        : 'text-slate-300 hover:bg-slate-800/60 hover:text-slate-100'
                    }`}
                  >
                    <div className="mt-0.5 shrink-0 text-slate-400">
                      {preset.type === 'structured' ? (
                        <FileCode className="w-3.5 h-3.5 text-slate-400" />
                      ) : (
                        <Box className="w-3.5 h-3.5 text-slate-400" />
                      )}
                    </div>

                    <div className="min-w-0 flex-1">
                      <div className="text-xs font-medium truncate text-slate-200">{preset.name}</div>
                      {preset.varsSummary && (
                        <div className="text-[11px] font-mono text-slate-400 truncate mt-0.5 font-normal">
                          {preset.varsSummary}
                        </div>
                      )}
                    </div>
                  </button>
                );
              })}
            </div>
          </div>
        </div>

        {/* Footer Info */}
        <div className="p-2.5 border-t border-slate-800 bg-slate-950 text-xs text-slate-400 flex items-center justify-between font-mono">
          <span className="flex items-center gap-1.5 text-[11px]">
            <span className={`w-1.5 h-1.5 rounded-full ${health.connected ? 'bg-emerald-400' : 'bg-rose-500'}`}></span>
            <span>{health.connected ? 'Core Connected' : 'Disconnected'}</span>
          </span>
          <span className="text-[11px] text-slate-500">C++20</span>
        </div>
      </aside>

      {/* Delete Confirmation Modal */}
      {benchmarkToDelete && (
        <div className="fixed inset-0 z-50 bg-slate-950/70 flex items-center justify-center p-4">
          <div className="bg-slate-900 border border-slate-800 rounded-md p-5 max-w-sm w-full shadow-lg space-y-4">
            <div className="space-y-1.5">
              <h4 className="text-sm font-semibold text-slate-100">Delete saved benchmark?</h4>
              <p className="text-xs text-slate-300">
                This removes the saved copy from your workbench.
              </p>
              <div className="bg-slate-950 px-2.5 py-1.5 rounded border border-slate-800 font-mono text-xs text-slate-300 truncate">
                {benchmarkToDelete.name}
              </div>
            </div>

            <div className="flex justify-end gap-2 pt-2 border-t border-slate-800">
              <button
                onClick={() => setBenchmarkToDelete(null)}
                className="px-3 py-1.5 rounded text-xs text-slate-300 hover:bg-slate-800 transition-colors"
              >
                Cancel
              </button>
              <button
                onClick={handleConfirmDelete}
                className="px-3.5 py-1.5 rounded text-xs font-medium bg-rose-600 hover:bg-rose-500 text-white transition-colors"
              >
                Delete
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  );
};
