import React, { useState } from 'react';
import { Plus, Trash2, FileText, Binary } from 'lucide-react';
import { PresetModel, BackendHealth } from '../types';

interface SidebarProps {
  activePresetId: string;
  savedBenchmarks: PresetModel[];
  onSelectPreset: (preset: PresetModel) => void;
  onDeleteBenchmark: (benchmark: PresetModel) => void;
  onNewProblem: () => void;
  health: BackendHealth;
}

export const Sidebar: React.FC<SidebarProps> = ({
  activePresetId,
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
      <aside className="w-full lg:w-64 bg-[#090d14] border-b lg:border-b-0 lg:border-r border-white/[0.08] flex flex-col h-auto max-h-56 lg:max-h-none lg:h-full min-h-0 min-w-0 select-none shrink-0 text-slate-300">
        {/* New Problem Action Header */}
        <div className="p-3 border-b border-white/[0.08] shrink-0">
          <button
            onClick={onNewProblem}
            className="w-full flex items-center justify-center gap-2 py-2 px-3 rounded-[6px] border border-white/[0.10] bg-white/[0.04] hover:bg-white/[0.08] text-slate-100 font-medium text-xs transition-colors shadow-sm cursor-pointer"
          >
            <Plus className="w-3.5 h-3.5 text-slate-300" />
            <span>New Problem</span>
          </button>
        </div>

        {/* Project Explorer: Saved Models */}
        <div className="flex-1 min-h-0 min-w-0 overflow-y-auto p-2 space-y-1">
          <div className="px-2 py-1.5 text-[10px] font-semibold text-slate-400 uppercase tracking-[0.06em] flex items-center justify-between">
            <span>Saved Models</span>
            <span className="text-[10px] font-mono text-slate-500 font-normal">
              ({savedBenchmarks.length})
            </span>
          </div>

          {savedBenchmarks.length === 0 ? (
            <div className="px-3 py-6 text-center space-y-2 text-slate-500">
              <div className="text-xs font-normal">No saved models</div>
              <p className="text-[11px] text-slate-500 leading-relaxed font-sans">
                Formulate a problem and click <span className="text-slate-300 font-medium">Save</span> in the editor toolbar to persist it here.
              </p>
            </div>
          ) : (
            <div className="space-y-1">
              {savedBenchmarks.map((model) => {
                const isActive = activePresetId === model.id;
                const isStructured = model.type === 'structured';

                return (
                  <div
                    key={model.id}
                    onClick={() => onSelectPreset(model)}
                    className={`group relative flex items-center justify-between rounded-[5px] px-2.5 py-2 cursor-pointer transition-colors border ${
                      isActive
                        ? 'bg-white/[0.07] border-white/[0.14] text-white'
                        : 'border-transparent hover:bg-white/[0.03] text-slate-300 hover:text-slate-100'
                    }`}
                  >
                    <div className="flex items-start gap-2.5 min-w-0 flex-1">
                      <div className="mt-0.5 shrink-0 text-slate-400">
                        {isStructured ? (
                          <FileText className="w-3.5 h-3.5 text-sky-400" />
                        ) : (
                          <Binary className="w-3.5 h-3.5 text-emerald-400" />
                        )}
                      </div>
                      <div className="min-w-0 flex-1">
                        <div className="flex items-center gap-1.5">
                          <span className="text-xs font-medium truncate text-slate-200">
                            {model.name}
                          </span>
                        </div>
                        <div className="flex items-center gap-1.5 mt-0.5 text-[10px] font-mono text-slate-400 truncate">
                          <span
                            className={`px-1 py-0.2 rounded text-[9px] uppercase font-semibold ${
                              isStructured
                                ? 'bg-sky-950/70 text-sky-300 border border-sky-800/40'
                                : 'bg-emerald-950/70 text-emerald-300 border border-emerald-800/40'
                            }`}
                          >
                            {model.type}
                          </span>
                          <span className="truncate">
                            {model.varsSummary || model.description || 'User formulation'}
                          </span>
                        </div>
                      </div>
                    </div>

                    {/* Subtle hover delete action */}
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        setBenchmarkToDelete(model);
                      }}
                      className="opacity-0 group-hover:opacity-100 focus:opacity-100 p-1 rounded hover:bg-white/[0.10] text-slate-400 hover:text-rose-400 transition-opacity shrink-0 ml-1 cursor-pointer"
                      title="Delete saved model"
                      aria-label="Delete saved model"
                    >
                      <Trash2 className="w-3.5 h-3.5" />
                    </button>
                  </div>
                );
              })}
            </div>
          )}
        </div>

        {/* Explorer Footer Status */}
        <div className="p-2.5 border-t border-white/[0.08] bg-[#07090d] text-xs text-slate-400 flex items-center justify-between font-mono shrink-0">
          <span className="flex items-center gap-2 text-[11px]">
            <span
              className={`w-1.5 h-1.5 rounded-full ${
                health.connected ? 'bg-emerald-400' : 'bg-rose-500'
              }`}
            ></span>
            <span className="text-slate-300">
              {health.connected ? 'Core Connected' : 'Disconnected'}
            </span>
          </span>
          <span className="text-[10px] text-slate-500">C++20</span>
        </div>
      </aside>

      {/* Delete Confirmation Modal */}
      {benchmarkToDelete && (
        <div className="fixed inset-0 z-50 bg-black/75 backdrop-blur-[4px] flex items-center justify-center p-4">
          <div className="sov-glass rounded-lg max-w-sm w-full p-5 space-y-4 shadow-2xl border border-white/[0.12] text-slate-200">
            <div className="space-y-1.5">
              <h4 className="text-sm font-semibold text-white">Delete saved model?</h4>
              <p className="text-xs text-slate-300 leading-relaxed">
                This permanently removes the model formulation from your browser workspace.
              </p>
              <div className="bg-[#07090d] px-3 py-2 rounded border border-white/[0.08] font-mono text-xs text-slate-300 truncate mt-2">
                {benchmarkToDelete.name}
              </div>
            </div>

            <div className="flex justify-end gap-2.5 pt-2 border-t border-white/[0.08]">
              <button
                onClick={() => setBenchmarkToDelete(null)}
                className="px-3.5 py-1.5 rounded text-xs text-slate-300 hover:bg-white/[0.08] border border-white/[0.06] transition-colors cursor-pointer"
              >
                Cancel
              </button>
              <button
                onClick={handleConfirmDelete}
                className="px-3.5 py-1.5 rounded text-xs font-medium bg-rose-600 hover:bg-rose-500 text-white transition-colors shadow-sm cursor-pointer"
              >
                Delete Model
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  );
};
