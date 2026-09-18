import React, { useState, useMemo } from 'react';
import { Search, ArrowUpDown } from 'lucide-react';
import { SolveResult } from '../types';

interface SolutionTableProps {
  solution?: Record<string, number>;
  telemetry?: SolveResult['telemetry'];
  statusName?: string;
}

export const SolutionTable: React.FC<SolutionTableProps> = ({ solution, statusName }) => {
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
    return null;
  }

  const nonZeroCount = rawEntries.filter(([_, v]) => Math.abs(v) > 1e-6).length;
  const activeCount = rawEntries.length;

  return (
    <div className="p-3.5 border-b border-slate-800 space-y-2.5 text-xs">
      {/* Header */}
      <div className="flex flex-wrap items-center justify-between gap-2">
        <div className="flex items-center gap-2">
          <span className="font-semibold text-slate-200">Primal Solution</span>
          <span className="font-mono text-[11px] text-slate-500">
            ({nonZeroCount} non-zero / {activeCount} total)
          </span>
        </div>

        {/* Controls */}
        <div className="flex items-center gap-2 font-mono text-[11px]">
          <div className="relative w-36">
            <Search className="w-3 h-3 absolute left-2 top-2 text-slate-500" />
            <input
              type="text"
              value={filter}
              onChange={(e) => setFilter(e.target.value)}
              placeholder="Filter..."
              className="w-full bg-slate-950 border border-slate-700 rounded pl-6 pr-2 py-1 text-slate-200 focus:outline-none focus:border-indigo-500"
            />
          </div>

          <label className="flex items-center gap-1.5 cursor-pointer text-slate-400 hover:text-slate-200 select-none">
            <input
              type="checkbox"
              checked={showNonZeroOnly}
              onChange={(e) => setShowNonZeroOnly(e.target.checked)}
              className="accent-slate-500 rounded cursor-pointer"
            />
            <span>Non-zero</span>
          </label>

          <button
            onClick={() => {
              if (sortBy === 'name') {
                setSortAsc(!sortAsc);
              } else {
                setSortBy('name');
                setSortAsc(true);
              }
            }}
            className={`px-2 py-0.5 rounded border flex items-center gap-1 transition-colors ${
              sortBy === 'name' ? 'bg-slate-800 text-slate-200 border-slate-600' : 'bg-slate-950 text-slate-400 border-slate-800'
            }`}
          >
            <ArrowUpDown className="w-2.5 h-2.5" />
            Name
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
            className={`px-2 py-0.5 rounded border flex items-center gap-1 transition-colors ${
              sortBy === 'value' ? 'bg-slate-800 text-slate-200 border-slate-600' : 'bg-slate-950 text-slate-400 border-slate-800'
            }`}
          >
            <ArrowUpDown className="w-2.5 h-2.5" />
            Value
          </button>
        </div>
      </div>

      {/* Table */}
      <div className="max-h-56 overflow-y-auto border border-slate-800 rounded bg-slate-950">
        <table className="w-full text-left font-mono text-xs">
          <thead className="bg-slate-900 text-slate-400 sticky top-0 border-b border-slate-800 text-[11px]">
            <tr>
              <th className="py-1.5 px-3 font-medium">Variable</th>
              <th className="py-1.5 px-3 font-medium text-right">Value</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-800/60">
            {filteredEntries.length > 0 ? (
              filteredEntries.map(([varName, val]) => (
                <tr key={varName} className="hover:bg-slate-900/50 transition-colors">
                  <td className="py-1 px-3 text-slate-300 font-medium">{varName}</td>
                  <td className="py-1 px-3 text-right text-slate-100 font-mono">
                    {val.toFixed(6)}
                  </td>
                </tr>
              ))
            ) : (
              <tr>
                <td colSpan={2} className="py-3 text-center text-slate-500 font-mono text-[11px]">
                  No matching variables
                </td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
};
