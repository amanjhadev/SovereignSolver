import React, { useState } from 'react';
import { FileCode, Upload, X } from 'lucide-react';
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
  const [name, setName] = useState('New Optimization Model');
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
    <div className="fixed inset-0 z-50 bg-slate-950/70 flex items-center justify-center p-4">
      <div className="bg-slate-900 border border-slate-800 rounded-md p-5 max-w-md w-full shadow-lg space-y-4">
        <div className="flex items-center justify-between border-b border-slate-800 pb-2.5">
          <h3 className="font-semibold text-sm text-slate-100">Create Optimization Model</h3>
          <button
            onClick={onClose}
            className="text-slate-400 hover:text-slate-200 p-1 rounded hover:bg-slate-800"
          >
            <X className="w-4 h-4" />
          </button>
        </div>

        <form onSubmit={handleSubmit} className="space-y-3.5 text-xs">
          <div>
            <label className="block text-slate-400 font-medium mb-1">Model Name</label>
            <input
              type="text"
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="e.g. Production Planning Model"
              className="w-full bg-slate-950 border border-slate-700 rounded px-2.5 py-1.5 text-slate-100 font-medium focus:outline-none focus:border-indigo-500 transition-colors"
              autoFocus
            />
          </div>

          <div>
            <label className="block text-slate-400 font-medium mb-1.5">Input Format</label>
            <div className="grid grid-cols-2 gap-2.5">
              <button
                type="button"
                onClick={() => setType('structured')}
                className={`p-3 rounded border text-left flex flex-col justify-between transition-colors ${
                  type === 'structured'
                    ? 'bg-slate-800 border-indigo-500 text-slate-100'
                    : 'bg-slate-950 border-slate-800 text-slate-400 hover:text-slate-200 hover:border-slate-700'
                }`}
              >
                <FileCode className="w-4 h-4 mb-2 text-slate-400" />
                <div>
                  <div className="text-xs font-semibold text-slate-200">Structured Syntax</div>
                  <div className="text-[10px] text-slate-500 mt-0.5">VARIABLES, MIN/MAX, SUBJECT TO</div>
                </div>
              </button>

              <button
                type="button"
                onClick={() => setType('mps')}
                className={`p-3 rounded border text-left flex flex-col justify-between transition-colors ${
                  type === 'mps'
                    ? 'bg-slate-800 border-indigo-500 text-slate-100'
                    : 'bg-slate-950 border-slate-800 text-slate-400 hover:text-slate-200 hover:border-slate-700'
                }`}
              >
                <Upload className="w-4 h-4 mb-2 text-slate-400" />
                <div>
                  <div className="text-xs font-semibold text-slate-200">MPS Format</div>
                  <div className="text-[10px] text-slate-500 mt-0.5">Standard .mps / .mps.gz files</div>
                </div>
              </button>
            </div>
          </div>

          <div className="flex justify-end gap-2 pt-2 border-t border-slate-800">
            <button
              type="button"
              onClick={onClose}
              className="px-3 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded text-xs font-medium transition-colors"
            >
              Cancel
            </button>
            <button
              type="submit"
              className="px-3.5 py-1.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded text-xs font-semibold transition-colors"
            >
              Create Model
            </button>
          </div>
        </form>
      </div>
    </div>
  );
};
