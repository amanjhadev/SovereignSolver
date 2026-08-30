import React from 'react';
import { TrendingDown, Activity, Info } from 'lucide-react';
import { MipCheckpoint } from '../types';

interface MipProgressChartProps {
  history?: MipCheckpoint[];
  sense?: string;
  objective?: number;
  bestBound?: number;
  gap?: number;
  isSolving?: boolean;
}

export const MipProgressChart: React.FC<MipProgressChartProps> = ({
  history,
  sense = 'MINIMIZE',
  objective,
  bestBound,
  gap,
  isSolving
}) => {
  // If telemetry history is unavailable, show clear empty state according to user rules
  if (!history || history.length === 0) {
    return (
      <div className="p-4 border-b border-slate-800 bg-slate-900/40 space-y-2">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2 text-xs font-bold text-slate-300 uppercase tracking-wider">
            <TrendingDown className="w-4 h-4 text-indigo-400" />
            <span>MIP Progress Convergence</span>
          </div>
          {gap !== undefined && (
            <span className="text-[11px] font-mono text-amber-400 font-bold bg-amber-950/60 px-2 py-0.5 rounded border border-amber-800/60">
              Gap: {(gap * 100).toFixed(2)}%
            </span>
          )}
        </div>

        <div className="h-36 bg-slate-950 rounded-lg border border-slate-800 flex flex-col items-center justify-center p-4 text-center text-slate-500 text-xs font-mono">
          <Info className="w-6 h-6 mb-1.5 text-slate-700" />
          <p className="text-slate-400 font-sans">
            {isSolving ? 'Waiting for solver telemetry...' : 'MIP progress trajectory telemetry unavailable for this run.'}
          </p>
          <p className="text-[11px] text-slate-600 mt-0.5">
            Single-point objective: {objective !== undefined ? objective.toFixed(4) : 'N/A'} | Best bound:{' '}
            {bestBound !== undefined ? bestBound.toFixed(4) : 'N/A'}
          </p>
        </div>
      </div>
    );
  }

  // Render SVG Chart if history is provided
  const width = 450;
  const height = 150;
  const padding = 30;

  const validIncumbents = history.map((h) => h.incumbent).filter((v): v is number => v !== undefined);
  const validBounds = history.map((h) => h.best_bound).filter((v): v is number => v !== undefined);
  const allValues = [...validIncumbents, ...validBounds];

  if (allValues.length === 0) return null;

  const minY = Math.min(...allValues);
  const maxY = Math.max(...allValues);
  const rangeY = maxY - minY === 0 ? 1 : maxY - minY;

  const maxX = Math.max(...history.map((h) => h.time_sec), 1);

  const getX = (time: number) => padding + (time / maxX) * (width - 2 * padding);
  const getY = (val: number) => height - padding - ((val - minY) / rangeY) * (height - 2 * padding);

  const incumbentPoints = history
    .filter((h) => h.incumbent !== undefined)
    .map((h) => `${getX(h.time_sec)},${getY(h.incumbent!)}`)
    .join(' ');

  const boundPoints = history
    .filter((h) => h.best_bound !== undefined)
    .map((h) => `${getX(h.time_sec)},${getY(h.best_bound!)}`)
    .join(' ');

  return (
    <div className="p-4 border-b border-slate-800 bg-slate-900/40 space-y-3">
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2 text-xs font-bold text-slate-300 uppercase tracking-wider">
          <TrendingDown className="w-4 h-4 text-indigo-400" />
          <span>MIP Progress Convergence</span>
        </div>
        {gap !== undefined && (
          <span className="text-[11px] font-mono text-amber-400 font-bold bg-amber-950/60 px-2 py-0.5 rounded border border-amber-800/60">
            Gap: {(gap * 100).toFixed(2)}%
          </span>
        )}
      </div>

      <div className="bg-slate-950 rounded-lg border border-slate-800 p-3">
        <svg viewBox={`0 0 ${width} ${height}`} className="w-full h-36">
          {/* Grid lines */}
          <line x1={padding} y1={padding} x2={width - padding} y2={padding} stroke="#1e293b" strokeDasharray="3 3" />
          <line x1={padding} y1={height - padding} x2={width - padding} y2={height - padding} stroke="#1e293b" />
          <line x1={padding} y1={padding} x2={padding} y2={height - padding} stroke="#1e293b" />

          {/* Lines */}
          {incumbentPoints && <polyline points={incumbentPoints} fill="none" stroke="#6366f1" strokeWidth="2" />}
          {boundPoints && <polyline points={boundPoints} fill="none" stroke="#34d399" strokeWidth="2" strokeDasharray="4 4" />}
        </svg>

        <div className="flex items-center justify-between text-[11px] font-mono text-slate-400 pt-2 border-t border-slate-800">
          <div className="flex items-center gap-2">
            <span className="w-2.5 h-2.5 rounded-full bg-indigo-500 inline-block"></span>
            <span>Incumbent</span>
          </div>
          <div className="flex items-center gap-2">
            <span className="w-2.5 h-2.5 rounded bg-emerald-400 inline-block"></span>
            <span>Best Bound</span>
          </div>
        </div>
      </div>
    </div>
  );
};
