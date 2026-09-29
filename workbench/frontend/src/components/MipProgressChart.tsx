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

  const width = 460;
  const height = 135;
  const paddingX = 30;
  const paddingY = 22;

  const minY = Math.min(...allValues);
  const maxY = Math.max(...allValues);
  const rangeY = maxY - minY === 0 ? 1 : maxY - minY;

  const maxX = Math.max(...history.map((h) => h.time_sec), 0.1);

  const getX = (time: number) => paddingX + (time / maxX) * (width - 2 * paddingX);
  const getY = (val: number) => height - paddingY - ((val - minY) / rangeY) * (height - 2 * paddingY);

  const incumbentPoints = history
    .filter((h) => h.incumbent !== undefined)
    .map((h) => `${getX(h.time_sec).toFixed(1)},${getY(h.incumbent!).toFixed(1)}`)
    .join(' ');

  const boundPoints = history
    .filter((h) => h.best_bound !== undefined)
    .map((h) => `${getX(h.time_sec).toFixed(1)},${getY(h.best_bound!).toFixed(1)}`)
    .join(' ');

  return (
    <div className="p-3.5 border-b border-white/[0.08] space-y-2.5 text-xs text-slate-200">
      <div className="flex items-center justify-between">
        <span className="font-semibold text-white tracking-tight">Convergence</span>
        {gap !== undefined && (
          <span className="font-mono text-[11px] text-slate-400">
            MIP Gap: <span className="text-white font-medium tabular-nums">{(gap * 100).toFixed(2)}%</span>
          </span>
        )}
      </div>

      <div className="bg-[#06080c] rounded-[5px] border border-white/[0.08] p-3 space-y-2">
        <svg viewBox={`0 0 ${width} ${height}`} className="w-full h-32 select-none">
          <defs>
            <linearGradient id="incumbentGrad" x1="0" y1="0" x2="0" y2="1">
              <stop offset="0%" stopColor="#38bdf8" stopOpacity="0.25" />
              <stop offset="100%" stopColor="#38bdf8" stopOpacity="0.0" />
            </linearGradient>
          </defs>

          {/* Grid lines */}
          <line
            x1={paddingX}
            y1={paddingY}
            x2={width - paddingX}
            y2={paddingY}
            stroke="rgba(255,255,255,0.06)"
            strokeDasharray="2 3"
          />
          <line
            x1={paddingX}
            y1={height / 2}
            x2={width - paddingX}
            y2={height / 2}
            stroke="rgba(255,255,255,0.04)"
            strokeDasharray="2 3"
          />
          <line
            x1={paddingX}
            y1={height - paddingY}
            x2={width - paddingX}
            y2={height - paddingY}
            stroke="rgba(255,255,255,0.08)"
          />
          <line
            x1={paddingX}
            y1={paddingY}
            x2={paddingX}
            y2={height - paddingY}
            stroke="rgba(255,255,255,0.08)"
          />

          {/* Bound Lines */}
          {boundPoints && (
            <polyline
              points={boundPoints}
              fill="none"
              stroke="#34d399"
              strokeWidth="1.5"
              strokeDasharray="4 3"
            />
          )}

          {/* Incumbent Lines */}
          {incumbentPoints && (
            <polyline
              points={incumbentPoints}
              fill="none"
              stroke="#38bdf8"
              strokeWidth="1.75"
            />
          )}
        </svg>

        {/* Legend & Metrics Bar */}
        <div className="flex items-center justify-between text-[10px] font-mono text-slate-400 pt-1.5 border-t border-white/[0.05]">
          <div className="flex items-center gap-3.5">
            <span className="flex items-center gap-1.5">
              <span className="w-2.5 h-0.5 bg-sky-400 inline-block"></span>
              <span className="text-slate-300">Incumbent</span>
            </span>
            <span className="flex items-center gap-1.5">
              <span className="w-2.5 h-0.5 bg-emerald-400 border-b border-dashed border-emerald-400 inline-block"></span>
              <span className="text-slate-300">Best Bound</span>
            </span>
          </div>
          <div className="text-slate-500 tabular-nums">
            [{minY.toFixed(2)} .. {maxY.toFixed(2)}] · {maxX.toFixed(1)}s
          </div>
        </div>
      </div>
    </div>
  );
};
