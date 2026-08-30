import React, { useState } from 'react';
import { PlusCircle, FileCode, Upload, X } from 'lucide-react';
import { PresetInputType } from '../types';

interface NewProblemModalProps {
  isOpen: boolean;
  onClose: () => void;
  onCreate: (name: string, type: PresetInputType, templateContent?: string) => void;
}

const DEFAULT_STRUCTURED_TEMPLATE = `VARIABLES
x1 binary
x2 binary
x3 continuous

MINIMIZE
5*x1 + 7*x2 + 3*x3

SUBJECT TO
c1: x1 + x2 >= 1
c2: x3 <= 10`;

export const NewProblemModal: React.FC<NewProblemModalProps> = ({ isOpen, onClose, onCreate }) => {
  const [name, setName] = useState('New Optimization Problem');
  const [type, setType] = useState<PresetInputType>('structured');

  if (!isOpen) return null;

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (!name.trim()) return;
    const initialContent = type === 'structured' ? DEFAULT_STRUCTURED_TEMPLATE : '';
    onCreate(name.trim(), type, initialContent);
    onClose();
  };

  return (
    <div className="fixed inset-0 z-50 bg-slate-950/80 backdrop-blur-sm flex items-center justify-center p-4">
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 max-w-md w-full shadow-2xl space-y-5">
        <div className="flex items-center justify-between border-b border-slate-800 pb-3">
          <div className="flex items-center gap-2">
            <PlusCircle className="w-5 h-5 text-indigo-400" />
            <h3 className="font-bold text-base text-slate-100">Create New Optimization Problem</h3>
          </div>
          <button onClick={onClose} className="text-slate-400 hover:text-white p-1 rounded">
            <X className="w-4 h-4" />
          </button>
        </div>

        <form onSubmit={handleSubmit} className="space-y-4 text-xs">
          <div>
            <label className="block text-slate-400 font-semibold mb-1">Problem Name</label>
            <input
              type="text"
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="e.g. Production Planning Q3"
              className="w-full bg-slate-950 border border-slate-800 rounded-lg px-3 py-2 text-slate-100 font-medium focus:outline-none focus:border-indigo-500 transition-colors"
              autoFocus
            />
          </div>

          <div>
            <label className="block text-slate-400 font-semibold mb-2">Input Format Type</label>
            <div className="grid grid-cols-2 gap-3">
              <button
                type="button"
                onClick={() => setType('structured')}
                className={`p-3 rounded-lg border text-left flex flex-col justify-between transition-all ${
                  type === 'structured'
                    ? 'bg-indigo-600/10 border-indigo-500 text-indigo-300 font-semibold'
                    : 'bg-slate-950 border-slate-800 text-slate-400 hover:text-slate-200'
                }`}
              >
                <FileCode className="w-5 h-5 mb-2 text-indigo-400" />
                <div>
                  <div className="text-xs text-slate-100 font-bold">Structured Model</div>
                  <div className="text-[10px] text-slate-500 mt-0.5">VARIABLES, MIN/MAX, SUBJECT TO text syntax</div>
                </div>
              </button>

              <button
                type="button"
                onClick={() => setType('mps')}
                className={`p-3 rounded-lg border text-left flex flex-col justify-between transition-all ${
                  type === 'mps'
                    ? 'bg-indigo-600/10 border-indigo-500 text-indigo-300 font-semibold'
                    : 'bg-slate-950 border-slate-800 text-slate-400 hover:text-slate-200'
                }`}
              >
                <Upload className="w-5 h-5 mb-2 text-indigo-400" />
                <div>
                  <div className="text-xs text-slate-100 font-bold">MPS Upload</div>
                  <div className="text-[10px] text-slate-500">Upload standard .mps or .mps.gz files</div>
                </div>
              </button>
            </div>
          </div>

          <div className="flex justify-end gap-3 pt-3 border-t border-slate-800">
            <button
              type="button"
              onClick={onClose}
              className="px-4 py-2 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-lg font-medium"
            >
              Cancel
            </button>
            <button
              type="submit"
              className="px-5 py-2 bg-indigo-600 hover:bg-indigo-500 text-white rounded-lg font-bold shadow-lg shadow-indigo-600/20"
            >
              Create Problem
            </button>
          </div>
        </form>
      </div>
    </div>
  );
};
