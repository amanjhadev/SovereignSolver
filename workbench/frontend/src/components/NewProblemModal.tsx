import React, { useState, useEffect } from 'react';
import { FileText, Binary, X } from 'lucide-react';
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

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape' && isOpen) {
        onClose();
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [isOpen, onClose]);

  if (!isOpen) return null;

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (!name.trim()) return;
    const initialContent = type === 'structured' ? DEFAULT_STRUCTURED_TEMPLATE : '';
    onCreate(name.trim(), type, initialContent);
    onClose();
  };

  return (
    <div className="fixed inset-0 z-50 bg-black/75 backdrop-blur-[6px] flex items-center justify-center p-4">
      <div className="sov-glass rounded-lg max-w-md w-full p-5 space-y-4 shadow-2xl border border-white/[0.12] text-slate-200">
        <div className="flex items-center justify-between border-b border-white/[0.08] pb-3">
          <h3 className="font-semibold text-sm text-white tracking-tight">Create Optimization Model</h3>
          <button
            onClick={onClose}
            className="text-slate-400 hover:text-white p-1 rounded hover:bg-white/[0.08] transition-colors"
          >
            <X className="w-4 h-4" />
          </button>
        </div>

        <form onSubmit={handleSubmit} className="space-y-4 text-xs">
          <div>
            <label className="block text-slate-300 font-medium mb-1.5">Model Name</label>
            <input
              type="text"
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="e.g. Facility Assignment"
              className="w-full bg-[#06080c] border border-white/[0.10] rounded-[5px] px-3 py-1.5 text-white font-medium focus:outline-none focus:border-white/30 transition-colors"
              autoFocus
            />
          </div>

          <div>
            <label className="block text-slate-300 font-medium mb-1.5">Formulation Format</label>
            <div className="grid grid-cols-2 gap-2.5">
              <button
                type="button"
                onClick={() => setType('structured')}
                className={`p-3 rounded-[6px] border text-left flex flex-col justify-between transition-colors ${
                  type === 'structured'
                    ? 'bg-white/[0.08] border-white/[0.20] text-white shadow-sm'
                    : 'bg-[#06080c] border-white/[0.06] text-slate-400 hover:text-slate-200 hover:border-white/[0.12]'
                }`}
              >
                <FileText className="w-4 h-4 mb-2 text-sky-400" />
                <div>
                  <div className="text-xs font-semibold text-white">Structured Syntax</div>
                  <div className="text-[10px] text-slate-500 mt-0.5">VARIABLES, MIN/MAX, SUBJECT TO</div>
                </div>
              </button>

              <button
                type="button"
                onClick={() => setType('mps')}
                className={`p-3 rounded-[6px] border text-left flex flex-col justify-between transition-colors ${
                  type === 'mps'
                    ? 'bg-white/[0.08] border-white/[0.20] text-white shadow-sm'
                    : 'bg-[#06080c] border-white/[0.06] text-slate-400 hover:text-slate-200 hover:border-white/[0.12]'
                }`}
              >
                <Binary className="w-4 h-4 mb-2 text-emerald-400" />
                <div>
                  <div className="text-xs font-semibold text-white">MPS Format</div>
                  <div className="text-[10px] text-slate-500 mt-0.5">Standard .mps / .mps.gz files</div>
                </div>
              </button>
            </div>
          </div>

          <div className="flex justify-end gap-2.5 pt-2 border-t border-white/[0.08]">
            <button
              type="button"
              onClick={onClose}
              className="px-3.5 py-1.5 bg-white/[0.04] hover:bg-white/[0.08] text-slate-300 rounded-[5px] text-xs font-medium border border-white/[0.08] transition-colors"
            >
              Cancel
            </button>
            <button
              type="submit"
              className="btn-solve-primary px-4 py-1.5 text-xs font-semibold cursor-pointer"
            >
              Create Model
            </button>
          </div>
        </form>
      </div>
    </div>
  );
};
