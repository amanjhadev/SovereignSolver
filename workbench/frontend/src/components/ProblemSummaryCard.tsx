import React from 'react';
import { Layers, FileCode, Target, Hash } from 'lucide-react';
import { SolverTelemetry, ValidationInfo } from '../types';

interface ProblemSummaryCardProps {
  name: string;
  type: 'structured' | 'mps';
  telemetry?: SolverTelemetry;
  validationInfo?: ValidationInfo | null;
}

export const ProblemSummaryCard: React.FC<ProblemSummaryCardProps> = ({
  name,
  type,
  telemetry,
  validationInfo
}) => {
  const vars = telemetry?.variables ?? validationInfo?.variables;
  const binaries = telemetry?.binaries ?? validationInfo?.binaries;
  const continuous = telemetry?.continuous ?? validationInfo?.continuous;
  const constraints = telemetry?.constraints ?? validationInfo?.constraints;
  const nnz = telemetry?.nnz ?? validationInfo?.nnz;
  const sense = telemetry?.sense ?? validationInfo?.sense;

  if (vars === undefined && constraints === undefined) {
    return (
      <div className="p-3 bg-slate-900/60 border-b border-slate-800 text-xs font-mono text-slate-500 flex items-center justify-between">
        <div className="flex items-center gap-2">
          <Layers className="w-4 h-4 text-indigo-400" />
          <span className="font-semibold text-slate-300">{name}</span>
        </div>
        <span className="uppercase text-[10px] px-2 py-0.5 rounded bg-slate-800 text-slate-400">
          {type} model
        </span>
      </div>
    );
  }

  return (
    <div className="p-4 border-b border-slate-800 bg-slate-900/40 space-y-3">
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2">
          <Layers className="w-4 h-4 text-indigo-400" />
          <h4 className="text-xs font-bold text-slate-200 tracking-wide">MODEL SUMMARY</h4>
        </div>
        <span className="text-[10px] font-mono font-bold px-2 py-0.5 rounded bg-indigo-950 text-indigo-300 border border-indigo-800/60 uppercase">
          {type}
        </span>
      </div>

      <div className="grid grid-cols-4 gap-2 text-xs font-mono">
        <div className="bg-slate-950 p-2 rounded border border-slate-800/80">
          <div className="text-[10px] text-slate-500 font-sans font-semibold">Model Name</div>
          <div className="text-xs font-bold text-indigo-300 truncate mt-0.5">{name}</div>
        </div>

        <div className="bg-slate-950 p-2 rounded border border-slate-800/80">
          <div className="text-[10px] text-slate-500 font-sans font-semibold">Variables</div>
          <div className="text-xs font-bold text-slate-100 mt-0.5">
            {vars ?? 'N/A'}{' '}
            <span className="text-[10px] text-slate-400 font-normal">
              ({binaries ?? 0} bin, {continuous ?? 0} cont)
            </span>
          </div>
        </div>

        <div className="bg-slate-950 p-2 rounded border border-slate-800/80">
          <div className="text-[10px] text-slate-500 font-sans font-semibold">Constraints / NNZ</div>
          <div className="text-xs font-bold text-slate-100 mt-0.5">
            {constraints ?? 'N/A'} <span className="text-[10px] text-slate-400 font-normal">({nnz ?? 0} nnz)</span>
          </div>
        </div>

        <div className="bg-slate-950 p-2 rounded border border-slate-800/80">
          <div className="text-[10px] text-slate-500 font-sans font-semibold">Objective Sense</div>
          <div className="text-xs font-bold text-indigo-400 mt-0.5">{sense ?? 'MINIMIZE'}</div>
        </div>
      </div>
    </div>
  );
};
