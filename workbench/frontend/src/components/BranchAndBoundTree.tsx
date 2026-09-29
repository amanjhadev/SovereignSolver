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
        return 'border-sky-500/50 bg-sky-950/40 text-sky-200';
      case 'open':
        return 'border-amber-500/50 bg-amber-950/40 text-amber-200';
      case 'fathomed':
        return 'border-white/[0.08] bg-white/[0.02] text-slate-500';
      case 'infeasible':
        return 'border-rose-500/50 bg-rose-950/40 text-rose-200';
      case 'bound_pruned':
        return 'border-purple-500/50 bg-purple-950/40 text-purple-200';
      case 'integer_feasible':
        return 'border-emerald-500/60 bg-emerald-950/50 text-emerald-200 font-medium';
      default:
        return 'border-white/[0.08] bg-[#06080c] text-slate-400';
    }
  };

  return (
    <div className="p-3.5 border-b border-white/[0.08] space-y-2.5 text-xs text-slate-200">
      <div className="flex items-center justify-between">
        <span className="font-semibold text-white tracking-tight">Branch &amp; Bound Tree</span>
        <span className="font-mono text-[10px] text-slate-400 px-2 py-0.5 rounded-[4px] bg-white/[0.04] border border-white/[0.06]">
          {treeNodes.length} {treeNodes.length === 1 ? 'node' : 'nodes'}
        </span>
      </div>

      {/* Status Legend */}
      <div className="flex flex-wrap gap-x-3.5 gap-y-1 text-[10px] font-mono text-slate-400 border-b border-white/[0.06] pb-2">
        <span className="flex items-center gap-1.5">
          <span className="w-1.5 h-1.5 rounded-full bg-sky-400"></span> Processing
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
      <div className="bg-[#06080c] p-2.5 rounded-[5px] border border-white/[0.08] max-h-40 overflow-y-auto">
        <div className="flex flex-wrap gap-1.5">
          {treeNodes.map((node) => {
            const isSelected = selectedNodeId === node.id;
            return (
              <button
                key={node.id}
                onClick={() => setSelectedNodeId(node.id)}
                className={`px-2 py-1 rounded-[4px] text-[10px] font-mono border transition-all ${getNodeBadgeClass(
                  node.status
                )} ${isSelected ? 'ring-1 ring-white/50 border-white/40' : 'hover:border-white/30'}`}
                title={`Node ${node.id} (depth ${node.depth}, bound ${node.bound !== undefined ? node.bound.toFixed(4) : 'N/A'})`}
              >
                N{node.id} <span className="opacity-60">(d={node.depth})</span>
              </button>
            );
          })}
        </div>
      </div>

      {/* Selected Node Details Inspector */}
      {selectedNode && (
        <div className="bg-[#06080c] p-2.5 rounded-[5px] border border-white/[0.08] font-mono text-[11px] text-slate-300 flex flex-wrap justify-between items-center gap-2">
          <div className="space-x-2">
            <span className="font-semibold text-white">Node #{selectedNode.id}</span>
            {selectedNode.parentId !== undefined && selectedNode.parentId !== -1 && (
              <span className="text-slate-400">parent: #{selectedNode.parentId}</span>
            )}
            <span className="text-slate-400">depth: {selectedNode.depth}</span>
            <span className="text-slate-400">
              bound: <span className="text-slate-200 tabular-nums">{selectedNode.bound.toFixed(6)}</span>
            </span>
            {selectedNode.incumbentContribution !== undefined && (
              <span className="text-slate-400">
                branch: <span className="text-slate-200">{selectedNode.incumbentContribution}</span>
              </span>
            )}
          </div>
          <span className="uppercase text-[9px] px-2 py-0.5 rounded-[4px] bg-white/[0.05] border border-white/[0.08] text-slate-300 font-medium">
            {selectedNode.status.replace(/_/g, ' ')}
          </span>
        </div>
      )}
    </div>
  );
};
