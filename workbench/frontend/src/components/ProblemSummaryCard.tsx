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
    <div className="p-3.5 border-b border-slate-800 bg-slate-900/50 space-y-2 text-xs">
      <div className="flex items-center justify-between text-slate-300">
        <span className="font-semibold text-slate-200">Model Dimensions</span>
        <span className="font-mono text-[11px] px-1.5 py-0.5 rounded bg-slate-800 text-slate-400 uppercase">
          {type}
        </span>
      </div>

      <div className="grid grid-cols-2 sm:grid-cols-4 gap-2 font-mono text-[11px]">
        <div className="bg-slate-950 px-2.5 py-1.5 rounded border border-slate-800/80">
          <div className="text-[10px] text-slate-500 font-sans">Model</div>
          <div className="font-medium text-slate-200 truncate mt-0.5">{name}</div>
        </div>

        <div className="bg-slate-950 px-2.5 py-1.5 rounded border border-slate-800/80">
          <div className="text-[10px] text-slate-500 font-sans">Variables</div>
          <div className="font-medium text-slate-200 mt-0.5">
            {vars ?? '—'}{' '}
            {vars !== undefined && (
              <span className="text-slate-500 font-normal">
                ({binaries ?? 0}b / {continuous ?? 0}c)
              </span>
            )}
          </div>
        </div>

        <div className="bg-slate-950 px-2.5 py-1.5 rounded border border-slate-800/80">
          <div className="text-[10px] text-slate-500 font-sans">Constraints</div>
          <div className="font-medium text-slate-200 mt-0.5">
            {constraints ?? '—'}{' '}
            {nnz !== undefined && <span className="text-slate-500 font-normal">({nnz} nz)</span>}
          </div>
        </div>

        <div className="bg-slate-950 px-2.5 py-1.5 rounded border border-slate-800/80">
          <div className="text-[10px] text-slate-500 font-sans">Sense</div>
          <div className="font-medium text-slate-200 mt-0.5">{sense ?? 'MINIMIZE'}</div>
        </div>
      </div>
    </div>
  );
};
