import React, { useState } from 'react';
import { GitBranch, Info } from 'lucide-react';
import { BbTreeNode } from '../types';

interface BranchAndBoundTreeProps {
  treeNodes?: BbTreeNode[];
  isSolving?: boolean;
}

export const BranchAndBoundTree: React.FC<BranchAndBoundTreeProps> = ({ treeNodes, isSolving }) => {
  const [selectedNodeId, setSelectedNodeId] = useState<number | null>(null);

  if (!treeNodes || treeNodes.length === 0) {
    return (
      <div className="p-4 border-b border-slate-800 bg-slate-900/40 space-y-2">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2 text-xs font-bold text-slate-300 uppercase tracking-wider">
            <GitBranch className="w-4 h-4 text-indigo-400" />
            <span>Branch & Bound Search Tree</span>
          </div>
        </div>

        <div className="h-32 bg-slate-950 rounded-lg border border-slate-800 flex flex-col items-center justify-center p-4 text-center text-slate-500 text-xs font-mono">
          <Info className="w-6 h-6 mb-1.5 text-slate-700" />
          <p className="text-slate-400 font-sans">
            {isSolving ? 'Waiting for B&B search tree telemetry...' : 'B&B tree telemetry unavailable for this solve run.'}
          </p>
          <p className="text-[11px] text-slate-600 mt-0.5">
            {isSolving ? 'Node events will appear as B&B search branches.' : 'The C++ solver engine ran without tree log emission.'}
          </p>
        </div>
      </div>
    );
  }

  const selectedNode = treeNodes.find((n) => n.id === selectedNodeId);

  const getStatusColor = (status: BbTreeNode['status']) => {
    switch (status) {
      case 'processing':
        return 'bg-blue-500 text-blue-100 border-blue-400';
      case 'open':
        return 'bg-amber-500 text-amber-950 border-amber-400';
      case 'fathomed':
        return 'bg-slate-700 text-slate-200 border-slate-600';
      case 'infeasible':
        return 'bg-rose-600 text-rose-100 border-rose-400';
      case 'bound_pruned':
        return 'bg-purple-600 text-purple-100 border-purple-400';
      case 'integer_feasible':
        return 'bg-emerald-500 text-emerald-950 border-emerald-300 font-bold';
      default:
        return 'bg-slate-800 text-slate-300 border-slate-700';
    }
  };

  return (
    <div className="p-4 border-b border-slate-800 bg-slate-900/40 space-y-3">
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2 text-xs font-bold text-slate-300 uppercase tracking-wider">
          <GitBranch className="w-4 h-4 text-indigo-400" />
          <span>Branch & Bound Search Tree ({treeNodes.length} nodes)</span>
        </div>
      </div>

      {/* Status Legend */}
      <div className="flex flex-wrap gap-2 text-[10px] font-mono border-b border-slate-800 pb-2">
        <span className="flex items-center gap-1 text-blue-300">
          <span className="w-2.5 h-2.5 rounded-full bg-blue-500"></span> Processing
        </span>
        <span className="flex items-center gap-1 text-amber-300">
          <span className="w-2.5 h-2.5 rounded-full bg-amber-500"></span> Open
        </span>
        <span className="flex items-center gap-1 text-slate-400">
          <span className="w-2.5 h-2.5 rounded-full bg-slate-700"></span> Fathomed
        </span>
        <span className="flex items-center gap-1 text-rose-300">
          <span className="w-2.5 h-2.5 rounded-full bg-rose-600"></span> Infeasible
        </span>
        <span className="flex items-center gap-1 text-purple-300">
          <span className="w-2.5 h-2.5 rounded-full bg-purple-600"></span> Bound-Pruned
        </span>
        <span className="flex items-center gap-1 text-emerald-300 font-semibold">
          <span className="w-2.5 h-2.5 rounded-full bg-emerald-500"></span> Integer Feasible
        </span>
      </div>

      {/* Compact Tree Nodes Grid */}
      <div className="bg-slate-950 p-3 rounded-lg border border-slate-800 max-h-48 overflow-y-auto">
        <div className="flex flex-wrap gap-2">
          {treeNodes.map((node) => (
            <button
              key={node.id}
              onClick={() => setSelectedNodeId(node.id)}
              className={`px-2 py-1 rounded text-xs font-mono border transition-all ${getStatusColor(
                node.status
              )} ${selectedNodeId === node.id ? 'ring-2 ring-indigo-400 scale-105' : ''}`}
            >
              N{node.id} (d={node.depth})
            </button>
          ))}
        </div>
      </div>

      {/* Selected Node Details */}
      {selectedNode && (
        <div className="bg-slate-900 p-2.5 rounded-lg border border-indigo-800/60 font-mono text-xs text-slate-200 flex justify-between items-center">
          <div>
            <span className="text-indigo-400 font-bold">Node #{selectedNode.id}</span> | Depth:{' '}
            {selectedNode.depth} | Bound: {selectedNode.bound.toFixed(4)}
          </div>
          <span className="uppercase text-[10px] px-2 py-0.5 rounded bg-slate-800 font-semibold">
            {selectedNode.status}
          </span>
        </div>
      )}
    </div>
  );
};
