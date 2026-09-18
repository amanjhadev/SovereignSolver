import React, { useState } from 'react';
import { BbTreeNode } from '../types';

interface BranchAndBoundTreeProps {
  treeNodes?: BbTreeNode[];
  isSolving?: boolean;
}

export const BranchAndBoundTree: React.FC<BranchAndBoundTreeProps> = ({ treeNodes }) => {
  const [selectedNodeId, setSelectedNodeId] = useState<number | null>(null);

  if (!treeNodes || treeNodes.length === 0) {
    return null;
  }

  const selectedNode = treeNodes.find((n) => n.id === selectedNodeId);

  const getNodeBadgeClass = (status: BbTreeNode['status']) => {
    switch (status) {
      case 'processing':
        return 'border-blue-500/80 bg-blue-950/60 text-blue-200';
      case 'open':
        return 'border-amber-500/80 bg-amber-950/60 text-amber-200';
      case 'fathomed':
        return 'border-slate-700 bg-slate-900 text-slate-400';
      case 'infeasible':
        return 'border-rose-500/80 bg-rose-950/60 text-rose-200';
      case 'bound_pruned':
        return 'border-purple-500/80 bg-purple-950/60 text-purple-200';
      case 'integer_feasible':
        return 'border-emerald-500/80 bg-emerald-950/60 text-emerald-200 font-medium';
      default:
        return 'border-slate-800 bg-slate-900 text-slate-300';
    }
  };

  return (
    <div className="p-3.5 border-b border-slate-800 space-y-2.5 text-xs">
      <div className="flex items-center justify-between">
        <span className="font-semibold text-slate-200">Branch & Bound Tree</span>
        <span className="font-mono text-[11px] text-slate-400">{treeNodes.length} nodes</span>
      </div>

      {/* Status Legend */}
      <div className="flex flex-wrap gap-x-3 gap-y-1 text-[10px] font-mono text-slate-400 border-b border-slate-800/80 pb-2">
        <span className="flex items-center gap-1.5">
          <span className="w-1.5 h-1.5 rounded-full bg-blue-400"></span> Processing
        </span>
        <span className="flex items-center gap-1.5">
          <span className="w-1.5 h-1.5 rounded-full bg-amber-400"></span> Open
        </span>
        <span className="flex items-center gap-1.5">
          <span className="w-1.5 h-1.5 rounded-full bg-emerald-400"></span> Feasible
        </span>
        <span className="flex items-center gap-1.5">
          <span className="w-1.5 h-1.5 rounded-full bg-rose-400"></span> Infeasible
        </span>
        <span className="flex items-center gap-1.5">
          <span className="w-1.5 h-1.5 rounded-full bg-purple-400"></span> Pruned
        </span>
        <span className="flex items-center gap-1.5">
          <span className="w-1.5 h-1.5 rounded-full bg-slate-500"></span> Fathomed
        </span>
      </div>

      {/* Nodes Matrix */}
      <div className="bg-slate-950 p-2.5 rounded border border-slate-800 max-h-40 overflow-y-auto">
        <div className="flex flex-wrap gap-1.5">
          {treeNodes.map((node) => {
            const isSelected = selectedNodeId === node.id;
            return (
              <button
                key={node.id}
                onClick={() => setSelectedNodeId(node.id)}
                className={`px-2 py-1 rounded text-[11px] font-mono border transition-colors ${getNodeBadgeClass(
                  node.status
                )} ${isSelected ? 'ring-1 ring-slate-300' : ''}`}
                title={`Node ${node.id} (depth ${node.depth}, bound ${node.bound.toFixed(4)})`}
              >
                N{node.id} (d={node.depth})
              </button>
            );
          })}
        </div>
      </div>

      {/* Selected Node Details */}
      {selectedNode && (
        <div className="bg-slate-950 p-2 rounded border border-slate-800 font-mono text-[11px] text-slate-300 flex justify-between items-center">
          <div>
            <span className="font-semibold text-slate-200">Node #{selectedNode.id}</span> · Depth {selectedNode.depth} · Bound {selectedNode.bound.toFixed(6)}
          </div>
          <span className="uppercase text-[10px] px-1.5 py-0.5 rounded bg-slate-900 border border-slate-800 text-slate-400">
            {selectedNode.status.replace('_', ' ')}
          </span>
        </div>
      )}
    </div>
  );
};
