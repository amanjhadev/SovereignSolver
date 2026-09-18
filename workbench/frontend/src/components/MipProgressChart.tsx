import React from 'react';
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
  gap
}) => {
  if (!history || history.length === 0) {
    return null;
  }

  const validIncumbents = history.map((h) => h.incumbent).filter((v): v is number => v !== undefined);
  const validBounds = history.map((h) => h.best_bound).filter((v): v is number => v !== undefined);
  const allValues = [...validIncumbents, ...validBounds];

  if (allValues.length === 0) return null;

  const width = 450;
  const height = 140;
  const padding = 28;

  const minY = Math.min(...allValues);
  const maxY = Math.max(...allValues);
  const rangeY = maxY - minY === 0 ? 1 : maxY - minY;

  const maxX = Math.max(...history.map((h) => h.time_sec), 0.1);

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
    <div className="p-3.5 border-b border-slate-800 space-y-2 text-xs">
      <div className="flex items-center justify-between">
        <span className="font-semibold text-slate-200">Convergence</span>
        {gap !== undefined && (
          <span className="font-mono text-[11px] text-slate-400">
            Gap: <span className="text-slate-100 font-medium">{(gap * 100).toFixed(2)}%</span>
          </span>
        )}
      </div>

      <div className="bg-slate-950 rounded border border-slate-800 p-2.5">
        <svg viewBox={`0 0 ${width} ${height}`} className="w-full h-32">
          {/* Subtle Grid lines */}
          <line x1={padding} y1={padding} x2={width - padding} y2={padding} stroke="#334155" strokeDasharray="2 2" />
          <line x1={padding} y1={height - padding} x2={width - padding} y2={height - padding} stroke="#334155" />
          <line x1={padding} y1={padding} x2={padding} y2={height - padding} stroke="#334155" />

          {/* Bound lines */}
          {incumbentPoints && <polyline points={incumbentPoints} fill="none" stroke="#818cf8" strokeWidth="1.5" />}
          {boundPoints && <polyline points={boundPoints} fill="none" stroke="#34d399" strokeWidth="1.5" strokeDasharray="3 3" />}
        </svg>

        <div className="flex items-center justify-between text-[10px] font-mono text-slate-400 pt-1.5 border-t border-slate-850">
          <div className="flex items-center gap-3">
            <span className="flex items-center gap-1.5">
              <span className="w-2 h-0.5 bg-indigo-400 inline-block"></span>
              <span>Incumbent</span>
            </span>
            <span className="flex items-center gap-1.5">
              <span className="w-2 h-0.5 bg-emerald-400 border-b border-dashed border-emerald-400 inline-block"></span>
              <span>Best Bound</span>
            </span>
          </div>
          <div className="text-slate-500">
            [{minY.toFixed(2)} .. {maxY.toFixed(2)}] · {maxX.toFixed(1)}s
          </div>
        </div>
      </div>
    </div>
  );
};
