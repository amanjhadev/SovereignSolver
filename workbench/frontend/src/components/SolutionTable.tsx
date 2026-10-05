import React, { useState, useMemo } from 'react';
import { Search, ArrowUpDown, Copy, Check } from 'lucide-react';
import { SolveResult } from '../types';

interface SolutionTableProps {
  solution?: Record<string, number>;
  telemetry?: SolveResult['telemetry'];
  statusName?: string;
}

export const SolutionTable: React.FC<SolutionTableProps> = ({ solution }) => {
  const [filter, setFilter] = useState('');
  const [showNonZeroOnly, setShowNonZeroOnly] = useState(true);
  const [sortBy, setSortBy] = useState<'name' | 'value'>('name');
  const [sortAsc, setSortAsc] = useState(true);
  const [copied, setCopied] = useState(false);

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

  const handleCopySolution = () => {
    if (!solution) return;
    const text = filteredEntries.map(([k, v]) => `${k} = ${v}`).join('\n');
    navigator.clipboard.writeText(text);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  if (!solution || rawEntries.length === 0) {
    return null;
  }

  const nonZeroCount = rawEntries.filter(([_, v]) => Math.abs(v) > 1e-6).length;
  const activeCount = rawEntries.length;

  return (
    <div className="p-3.5 border-b border-white/[0.08] space-y-2.5 text-xs text-slate-200">
      {/* Header & Inspector Controls */}
      <div className="flex flex-wrap items-center justify-between gap-2">
        <div className="flex items-center gap-2">
          <span className="font-semibold text-white tracking-tight">Primal Solution</span>
          <span className="font-mono text-[11px] text-slate-400">
            ({nonZeroCount} non-zero / {activeCount} total)
          </span>
        </div>

        {/* Copy / Export Button */}
        <button
          onClick={handleCopySolution}
          className="flex items-center gap-1 px-2.5 py-1 rounded-[4px] bg-white/[0.04] hover:bg-white/[0.08] text-slate-300 hover:text-white border border-white/[0.08] transition-colors font-mono text-[10px] cursor-pointer"
          title="Copy visible variables to clipboard"
        >
          {copied ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3" />}
          <span>{copied ? 'Copied' : 'Copy Values'}</span>
        </button>
      </div>

      {/* Filter and Sorting Bar */}
      <div className="flex flex-wrap items-center justify-between gap-2 font-mono text-[11px]">
        {/* Search Input */}
        <div className="relative w-36 sm:w-44">
          <Search className="w-3 h-3 absolute left-2.5 top-2 text-slate-500" />
          <input
            type="text"
            value={filter}
            onChange={(e) => setFilter(e.target.value)}
            placeholder="Search variable..."
            className="w-full bg-[#06080c] border border-white/[0.10] rounded-[5px] pl-7 pr-2 py-1 text-slate-200 focus:outline-none focus:border-white/30 text-xs"
          />
        </div>

        <div className="flex items-center gap-2">
          <label className="flex items-center gap-1.5 cursor-pointer text-slate-400 hover:text-slate-200 select-none">
            <input
              type="checkbox"
              checked={showNonZeroOnly}
              onChange={(e) => setShowNonZeroOnly(e.target.checked)}
              className="accent-slate-400 rounded cursor-pointer w-3.5 h-3.5"
            />
            <span className="font-sans text-[11px]">Non-zero</span>
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
            className={`px-2 py-0.5 rounded-[4px] border flex items-center gap-1 transition-colors cursor-pointer ${
              sortBy === 'name'
                ? 'bg-white/[0.10] text-white border-white/[0.18]'
                : 'bg-[#06080c] text-slate-400 border-white/[0.06] hover:text-slate-200'
            }`}
          >
            <ArrowUpDown className="w-2.5 h-2.5" />
            <span>Name {sortBy === 'name' ? (sortAsc ? '▲' : '▼') : ''}</span>
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
            className={`px-2 py-0.5 rounded-[4px] border flex items-center gap-1 transition-colors cursor-pointer ${
              sortBy === 'value'
                ? 'bg-white/[0.10] text-white border-white/[0.18]'
                : 'bg-[#06080c] text-slate-400 border-white/[0.06] hover:text-slate-200'
            }`}
          >
            <ArrowUpDown className="w-2.5 h-2.5" />
            <span>Value {sortBy === 'value' ? (sortAsc ? '▲' : '▼') : ''}</span>
          </button>
        </div>
      </div>

      {/* Inspector Table with Vertical AND Horizontal Scrolling */}
      <div className="max-h-72 overflow-auto border border-white/[0.08] rounded-[5px] bg-[#06080c]">
        <table className="min-w-full text-left font-mono text-xs">
          <thead className="bg-[#0a0e16] text-slate-400 sticky top-0 z-10 border-b border-white/[0.08] text-[11px]">
            <tr>
              <th className="py-2 px-3 font-medium whitespace-nowrap">Variable</th>
              <th className="py-2 px-3 font-medium text-right whitespace-nowrap">Value</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-white/[0.04]">
            {filteredEntries.length > 0 ? (
              filteredEntries.map(([varName, val]) => (
                <tr key={varName} className="hover:bg-white/[0.02] transition-colors">
                  <td className="py-1.5 px-3 text-slate-300 font-medium whitespace-nowrap">
                    {varName}
                  </td>
                  <td className="py-1.5 px-3 text-right text-white font-mono tabular-nums whitespace-nowrap">
                    {val.toFixed(6)}
                  </td>
                </tr>
              ))
            ) : (
              <tr>
                <td colSpan={2} className="py-4 text-center text-slate-500 font-mono text-[11px]">
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
