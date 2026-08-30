import React from 'react';
import { CheckCircle2, Clock, XCircle, ChevronRight, Activity } from 'lucide-react';
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
    { label: 'B & B', status: getBbStatus() },
    { label: 'Final', status: getFinalStatus() }
  ];

  const renderStatusIcon = (status: PhaseStatus) => {
    switch (status) {
      case 'completed':
        return <CheckCircle2 className="w-3.5 h-3.5 text-emerald-400" />;
      case 'active':
        return <Activity className="w-3.5 h-3.5 text-amber-400 animate-spin" />;
      case 'failed':
        return <XCircle className="w-3.5 h-3.5 text-rose-400" />;
      default:
        return <Clock className="w-3.5 h-3.5 text-slate-600" />;
    }
  };

  const getBadgeClass = (status: PhaseStatus) => {
    switch (status) {
      case 'completed':
        return 'bg-emerald-950/80 text-emerald-300 border-emerald-700/60 font-semibold';
      case 'active':
        return 'bg-amber-950/80 text-amber-300 border-amber-500/60 font-bold';
      case 'failed':
        return 'bg-rose-950/80 text-rose-300 border-rose-700/60 font-semibold';
      default:
        return 'bg-slate-950 text-slate-500 border-slate-800';
    }
  };

  return (
    <div className="p-4 border-b border-slate-800 bg-slate-900/40 space-y-2">
      <div className="text-xs font-bold text-slate-300 uppercase tracking-wider mb-2">
        Solver Phase Execution Timeline
      </div>

      <div className="flex items-center justify-between overflow-x-auto py-1 font-mono text-[11px] gap-1">
        {stages.map((stage, idx) => (
          <React.Fragment key={stage.label}>
            <div
              className={`flex items-center gap-1.5 px-2.5 py-1 rounded-lg border transition-all shrink-0 ${getBadgeClass(
                stage.status
              )}`}
            >
              {renderStatusIcon(stage.status)}
              <span>{stage.label}</span>
            </div>
            {idx < stages.length - 1 && <ChevronRight className="w-3.5 h-3.5 text-slate-700 shrink-0" />}
          </React.Fragment>
        ))}
      </div>
    </div>
  );
};
