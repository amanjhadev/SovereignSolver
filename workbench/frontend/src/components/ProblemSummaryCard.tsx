import React from 'react';
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

  return (
    <div className="p-3.5 border-b border-white/[0.08] bg-[#090d14]/70 space-y-2.5 text-xs text-slate-200">
      <div className="flex items-center justify-between">
        <span className="text-xs font-semibold text-white tracking-tight">Model Dimensions</span>
        <span className="font-mono text-[10px] px-2 py-0.5 rounded-[4px] bg-white/[0.06] text-slate-300 uppercase font-medium border border-white/[0.08]">
          {type}
        </span>
      </div>

      <div className="grid grid-cols-2 sm:grid-cols-4 gap-2 font-mono text-xs">
        <div className="bg-[#06080c] px-3 py-2 rounded-[5px] border border-white/[0.06]">
          <div className="text-[10px] text-slate-400 font-sans">Instance</div>
          <div className="font-medium text-slate-100 truncate mt-0.5" title={name}>
            {name}
          </div>
        </div>

        <div className="bg-[#06080c] px-3 py-2 rounded-[5px] border border-white/[0.06]">
          <div className="text-[10px] text-slate-400 font-sans">Variables</div>
          <div className="font-medium text-slate-100 mt-0.5 tabular-nums">
            {vars !== undefined ? vars : '—'}{' '}
            {vars !== undefined && (
              <span className="text-slate-400 text-[11px] font-normal">
                ({binaries ?? 0}b / {continuous ?? 0}c)
              </span>
            )}
          </div>
        </div>

        <div className="bg-[#06080c] px-3 py-2 rounded-[5px] border border-white/[0.06]">
          <div className="text-[10px] text-slate-400 font-sans">Constraints</div>
          <div className="font-medium text-slate-100 mt-0.5 tabular-nums">
            {constraints !== undefined ? constraints : '—'}{' '}
            {nnz !== undefined && (
              <span className="text-slate-400 text-[11px] font-normal">({nnz} nz)</span>
            )}
          </div>
        </div>

        <div className="bg-[#06080c] px-3 py-2 rounded-[5px] border border-white/[0.06]">
          <div className="text-[10px] text-slate-400 font-sans">Sense</div>
          <div className="font-medium text-slate-100 mt-0.5 uppercase tracking-wide">
            {sense || '—'}
          </div>
        </div>
      </div>
    </div>
  );
};
