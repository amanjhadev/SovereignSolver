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
        return <span className="w-1.5 h-1.5 rounded-full bg-amber-400"></span>;
      case 'failed':
        return <span className="w-1.5 h-1.5 rounded-full bg-rose-500"></span>;
      default:
        return <span className="w-1.5 h-1.5 rounded-full bg-slate-700"></span>;
    }
  };

  const getTextStyle = (status: PhaseStatus) => {
    switch (status) {
      case 'completed':
        return 'text-slate-200 font-medium';
      case 'active':
        return 'text-amber-300 font-semibold';
      case 'failed':
        return 'text-rose-400 font-medium';
      default:
        return 'text-slate-500';
    }
  };

  return (
    <div className="p-3 border-b border-slate-800 bg-slate-900/30 space-y-1.5">
      <div className="text-[11px] font-semibold text-slate-400">Solver progress</div>

      <div className="flex items-center overflow-x-auto py-0.5 font-mono text-[11px] gap-1">
        {stages.map((stage, idx) => (
          <React.Fragment key={stage.label}>
            <div
              className={`flex items-center gap-1.5 px-2 py-0.5 rounded border border-slate-800 bg-slate-950 shrink-0 ${getTextStyle(
                stage.status
              )}`}
            >
              {renderStatusDot(stage.status)}
              <span>{stage.label}</span>
            </div>
            {idx < stages.length - 1 && <ChevronRight className="w-3 h-3 text-slate-700 shrink-0" />}
          </React.Fragment>
        ))}
      </div>
    </div>
  );
};
