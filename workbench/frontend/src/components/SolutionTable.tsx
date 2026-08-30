import React, { useState, useMemo } from 'react';
import { Search, Table as TableIcon, CheckCircle2, ArrowUpDown, Filter } from 'lucide-react';
import { SolveResult } from '../types';

interface SolutionTableProps {
  solution?: Record<string, number>;
  telemetry?: SolveResult['telemetry'];
  statusName?: string;
}

export const SolutionTable: React.FC<SolutionTableProps> = ({ solution, telemetry, statusName }) => {
  const [filter, setFilter] = useState('');
  const [showNonZeroOnly, setShowNonZeroOnly] = useState(true);
  const [sortBy, setSortBy] = useState<'name' | 'value'>('name');
  const [sortAsc, setSortAsc] = useState(true);

  const rawEntries = useMemo(() => {
    if (!solution) return [];
    return Object.entries(solution);
  }, [solution]);

  const filteredEntries = useMemo(() => {
    return rawEntries
      .filter(([varName, val]) => {
        const matchesFilter = varName.toLowerCase().includes(filter.toLowerCase());
        const matchesNonZero = showNonZeroOnly ? Math.abs(val) > 1e-6 : true;
        return matchesFilter && matchesNonZero;
      })
      .sort((a, b) => {
        if (sortBy === 'name') {
          return sortAsc ? a[0].localeCompare(b[0]) : b[0].localeCompare(a[0]);
        } else {
          return sortAsc ? a[1] - b[1] : b[1] - a[1];
        }
      });
  }, [rawEntries, filter, showNonZeroOnly, sortBy, sortAsc]);

  if (!solution || rawEntries.length === 0) {
    return (
      <div className="p-4 border-b border-slate-800 text-slate-500 font-mono text-xs text-center">
        No primal solution data available for this solve.
      </div>
    );
  }

  // Solution Summary metrics
  const nonZeroCount = rawEntries.filter(([_, v]) => Math.abs(v) > 1e-6).length;
  const activeCount = rawEntries.length;
  const isOptimal = statusName === 'Optimal';
  const isFeasible = statusName === 'Feasible' || isOptimal;

  return (
    <div className="p-4 border-b border-slate-800 space-y-3">
      {/* Solution Summary Header */}
      <div className="flex flex-wrap items-center justify-between gap-2 border-b border-slate-800/80 pb-2.5">
        <div className="flex items-center gap-2">
          <TableIcon className="w-4 h-4 text-indigo-400" />
          <h4 className="text-xs font-bold text-slate-200 uppercase tracking-wider">Solution Summary</h4>
          {isFeasible && (
            <span className="inline-flex items-center gap-1 text-[10px] font-bold px-2 py-0.5 rounded bg-emerald-950 text-emerald-400 border border-emerald-800/60">
              <CheckCircle2 className="w-3 h-3 text-emerald-400" />
              Integer Feasible ✓
            </span>
          )}
        </div>

        <div className="flex items-center gap-3 text-[11px] font-mono text-slate-400">
          <div>
            Active Vars: <span className="text-indigo-300 font-bold">{activeCount}</span>
          </div>
          <div>
            Non-Zero Vars: <span className="text-emerald-400 font-bold">{nonZeroCount}</span>
          </div>
        </div>
      </div>

      {/* Controls Bar: Filter, Non-zero checkbox, Sort toggle */}
      <div className="flex flex-wrap items-center justify-between gap-3 text-xs">
        {/* Search input */}
        <div className="relative w-44">
          <Search className="w-3.5 h-3.5 absolute left-2.5 top-2.5 text-slate-500" />
          <input
            type="text"
            value={filter}
            onChange={(e) => setFilter(e.target.value)}
            placeholder="Search variable..."
            className="w-full bg-slate-900 border border-slate-800 rounded pl-8 pr-2 py-1 text-xs text-slate-200 focus:outline-none focus:border-indigo-500 font-mono placeholder:text-slate-600"
          />
        </div>

        <div className="flex items-center gap-3">
          {/* Show non-zero toggle */}
          <label className="flex items-center gap-1.5 cursor-pointer text-slate-400 hover:text-slate-200 text-xs font-mono select-none">
            <input
              type="checkbox"
              checked={showNonZeroOnly}
              onChange={(e) => setShowNonZeroOnly(e.target.checked)}
              className="accent-indigo-500 rounded cursor-pointer"
            />
            <span>Show non-zero only</span>
          </label>

          {/* Sort selector */}
          <button
            onClick={() => {
              if (sortBy === 'name') {
                setSortAsc(!sortAsc);
              } else {
                setSortBy('name');
                setSortAsc(true);
              }
            }}
            className={`px-2 py-1 rounded font-mono text-[11px] flex items-center gap-1 border transition-colors ${
              sortBy === 'name' ? 'bg-indigo-950 text-indigo-300 border-indigo-700' : 'bg-slate-900 text-slate-400 border-slate-800'
            }`}
          >
            <ArrowUpDown className="w-3 h-3" />
            Sort Name
          </button>

          <button
            onClick={() => {
              if (sortBy === 'value') {
                setSortAsc(!sortAsc);
              } else {
                setSortBy('value');
                setSortAsc(false);
              }
            }}
            className={`px-2 py-1 rounded font-mono text-[11px] flex items-center gap-1 border transition-colors ${
              sortBy === 'value' ? 'bg-indigo-950 text-indigo-300 border-indigo-700' : 'bg-slate-900 text-slate-400 border-slate-800'
            }`}
          >
            <ArrowUpDown className="w-3 h-3" />
            Sort Value
          </button>
        </div>
      </div>

      {/* Solution Table */}
      <div className="max-h-64 overflow-y-auto border border-slate-800 rounded-lg bg-slate-950">
        <table className="w-full text-left text-xs font-mono">
          <thead className="bg-slate-900 text-slate-400 sticky top-0 border-b border-slate-800">
            <tr>
              <th className="py-2 px-3 font-semibold">Variable Name</th>
              <th className="py-2 px-3 font-semibold text-right">Value</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-800/60">
            {filteredEntries.length > 0 ? (
              filteredEntries.map(([varName, val]) => (
                <tr key={varName} className="hover:bg-slate-900/60 transition-colors">
                  <td className="py-1.5 px-3 text-indigo-300 font-medium">{varName}</td>
                  <td className="py-1.5 px-3 text-right font-bold text-slate-100">
                    {val.toFixed(6)}
                  </td>
                </tr>
              ))
            ) : (
              <tr>
                <td colSpan={2} className="py-4 text-center text-slate-500 font-mono">
                  No matching variables found
                </td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
};
