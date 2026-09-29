import React from 'react';
import { ChevronRight } from 'lucide-react';
import { SolveResult, ValidationInfo } from '../types';

interface PhaseTimelineProps {
  validationInfo?: ValidationInfo | null;
  result?: SolveResult | null;
  isSolving?: boolean;
  isValidating?: boolean;
}

export type PhaseStatus = 'pending' | 'active' | 'completed' | 'failed';

export const PhaseTimeline: React.FC<PhaseTimelineProps> = ({
  validationInfo,
  result,
  isSolving,
  isValidating
}) => {
  const getParseStatus = (): PhaseStatus => {
    if (isValidating) return 'active';
    if (validationInfo) return validationInfo.success ? 'completed' : 'failed';
    if (result) return result.success ? 'completed' : 'failed';
    return 'pending';
  };

  const getValidateStatus = (): PhaseStatus => {
    if (isValidating) return 'active';
    if (validationInfo) return validationInfo.success ? 'completed' : 'failed';
    if (result) return 'completed';
    return 'pending';
  };

  const getPresolveStatus = (): PhaseStatus => {
    if (isSolving) return 'active';
    if (result) return 'completed';
    return 'pending';
  };

  const getRootLpStatus = (): PhaseStatus => {
    if (isSolving) return 'active';
    if (result?.telemetry?.root_lp_status !== undefined) return 'completed';
    if (result) return 'completed';
    return 'pending';
  };

  const getHeuristicsStatus = (): PhaseStatus => {
    if (isSolving) return 'active';
    if (result) return 'completed';
    return 'pending';
  };

  const getBbStatus = (): PhaseStatus => {
    if (isSolving) return 'active';
    if (result) return result.nodes_explored ? 'completed' : 'completed';
    return 'pending';
  };

  const getFinalStatus = (): PhaseStatus => {
    if (isSolving) return 'pending';
    if (result) return result.success ? 'completed' : 'failed';
    return 'pending';
  };

  const stages = [
    { label: 'Parse', status: getParseStatus() },
    { label: 'Validate', status: getValidateStatus() },
    { label: 'Presolve', status: getPresolveStatus() },
    { label: 'Root LP', status: getRootLpStatus() },
    { label: 'Heuristics', status: getHeuristicsStatus() },
    { label: 'B&B', status: getBbStatus() },
    { label: 'Result', status: getFinalStatus() }
  ];

  const renderStatusDot = (status: PhaseStatus) => {
    switch (status) {
      case 'completed':
        return <span className="w-1.5 h-1.5 rounded-full bg-emerald-400"></span>;
      case 'active':
        return <span className="w-1.5 h-1.5 rounded-full bg-amber-400 animate-pulse"></span>;
      case 'failed':
        return <span className="w-1.5 h-1.5 rounded-full bg-rose-500"></span>;
      default:
        return <span className="w-1.5 h-1.5 rounded-full bg-slate-700"></span>;
    }
  };

  const getBadgeStyle = (status: PhaseStatus) => {
    switch (status) {
      case 'completed':
        return 'border-white/[0.12] bg-white/[0.04] text-slate-200 font-medium';
      case 'active':
        return 'border-amber-500/40 bg-amber-950/40 text-amber-200 font-medium shadow-sm';
      case 'failed':
        return 'border-rose-500/40 bg-rose-950/40 text-rose-300 font-medium';
      default:
        return 'border-white/[0.04] bg-[#06080c] text-slate-500';
    }
  };

  return (
    <div className="p-3.5 border-b border-white/[0.08] bg-[#090d14]/40 space-y-2">
      <div className="text-[10px] font-semibold text-slate-400 uppercase tracking-[0.06em]">
        Solver progress
      </div>

      <div className="flex items-center overflow-x-auto py-1 font-mono text-[11px] gap-1.5">
        {stages.map((stage, idx) => (
          <React.Fragment key={stage.label}>
            <div
              className={`flex items-center gap-1.5 px-2.5 py-1 rounded-[4px] border shrink-0 transition-colors ${getBadgeStyle(
                stage.status
              )}`}
            >
              {renderStatusDot(stage.status)}
              <span>{stage.label}</span>
            </div>
            {idx < stages.length - 1 && (
              <ChevronRight className="w-3 h-3 text-slate-700 shrink-0" />
            )}
          </React.Fragment>
        ))}
      </div>
    </div>
  );
};
