import React, { useRef } from 'react';
import { Play, CheckCircle, Square, Upload, FileCode, Sliders } from 'lucide-react';
import { PRESET_MODELS, PresetModel } from '../samples/preset_models';

interface EditorPanelProps {
  name: string;
  onChangeName: (val: string) => void;
  inputMode: 'structured' | 'mps';
  onChangeInputMode: (mode: 'structured' | 'mps') => void;
  content: string;
  onChangeContent: (val: string) => void;
  timeoutSec: number;
  onChangeTimeoutSec: (val: number) => void;
  isSolving: boolean;
  isValidating: boolean;
  elapsedSec: number;
  validationInfo?: any;
  onValidate: () => void;
  onSolve: () => void;
  onStopSolve: () => void;
  onLoadPreset?: (preset: PresetModel) => void;
  onFileUpload: (file: File) => void;
  onFormatContent?: () => void;
  onClearContent?: () => void;
  onLoadExample?: () => void;
}

export const EditorPanel: React.FC<EditorPanelProps> = ({
  name,
  onChangeName,
  inputMode,
  onChangeInputMode,
  content,
  onChangeContent,
  timeoutSec,
  onChangeTimeoutSec,
  isSolving,
  isValidating,
  elapsedSec,
  onValidate,
  onSolve,
  onStopSolve,
  onLoadPreset,
  onFileUpload
}) => {
  const fileInputRef = useRef<HTMLInputElement>(null);

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    if (e.target.files && e.target.files[0]) {
      onFileUpload(e.target.files[0]);
    }
  };

  return (
    <div className="flex flex-col h-full overflow-hidden bg-slate-950 border-r border-slate-800">
      {/* Top Fixed Toolbar */}
      <div className="shrink-0 p-4 border-b border-slate-800 space-y-4 bg-slate-950">
        <div className="flex items-center justify-between gap-4">
          <input
            type="text"
            value={name}
            onChange={(e) => onChangeName(e.target.value)}
            placeholder="Problem Name (e.g. My Optimization Model)"
            className="flex-1 bg-slate-900 border border-slate-800 rounded-lg px-3.5 py-2 text-slate-100 font-semibold text-base focus:outline-none focus:border-indigo-500 transition-all placeholder:text-slate-600"
          />

          {/* Preset Buttons */}
          <div className="flex items-center gap-2">
            <span className="text-xs text-slate-500 font-medium">Load Preset:</span>
            <button
              onClick={() => onLoadPreset && onLoadPreset(PRESET_MODELS[0])}
              className="text-xs px-2.5 py-1.5 rounded bg-slate-800 hover:bg-slate-700 text-slate-300 transition-all"
            >
              4x4 Assignment
            </button>
            <button
              onClick={() => onLoadPreset && onLoadPreset(PRESET_MODELS[1])}
              className="text-xs px-2.5 py-1.5 rounded bg-slate-800 hover:bg-slate-700 text-slate-300 transition-all"
            >
              Facility Location
            </button>
          </div>
        </div>

        {/* Input Mode Selector & Timeout Settings */}
        <div className="flex items-center justify-between gap-4">
          <div className="flex bg-slate-900 p-1 rounded-lg border border-slate-800">
            <button
              onClick={() => onChangeInputMode('structured')}
              className={`px-3 py-1.5 rounded-md text-xs font-medium transition-all flex items-center gap-1.5 ${
                inputMode === 'structured'
                  ? 'bg-indigo-600 text-white shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <FileCode className="w-3.5 h-3.5" />
              Structured Model
            </button>
            <button
              onClick={() => onChangeInputMode('mps')}
              className={`px-3 py-1.5 rounded-md text-xs font-medium transition-all flex items-center gap-1.5 ${
                inputMode === 'mps'
                  ? 'bg-indigo-600 text-white shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <Upload className="w-3.5 h-3.5" />
              MPS File Upload
            </button>
          </div>

          {/* Timeout Config Slider */}
          <div className="flex items-center gap-3 bg-slate-900/60 px-3 py-1.5 rounded-lg border border-slate-800/80">
            <Sliders className="w-3.5 h-3.5 text-slate-400" />
            <span className="text-xs text-slate-400 font-mono">Timeout:</span>
            <input
              type="range"
              min="5"
              max="300"
              step="5"
              value={timeoutSec}
              onChange={(e) => onChangeTimeoutSec(Number(e.target.value))}
              className="w-24 accent-indigo-500 cursor-pointer"
            />
            <span className="text-xs font-mono font-bold text-indigo-400 w-10">{timeoutSec}s</span>
          </div>
        </div>
      </div>

      {/* Independently Scrollable Editor Content Area */}
      <div className="flex-1 min-h-0 relative flex flex-col p-4 overflow-hidden">
        {inputMode === 'mps' && (
          <div className="shrink-0 mb-3 flex items-center justify-between bg-slate-900/80 border border-dashed border-slate-700 p-3 rounded-lg">
            <div className="flex items-center gap-3">
              <Upload className="w-5 h-5 text-indigo-400" />
              <div>
                <div className="text-xs font-semibold text-slate-200">Upload MPS or MPS.GZ File</div>
                <div className="text-[10px] text-slate-500">Supports standard .mps and compressed .mps.gz files</div>
              </div>
            </div>
            <button
              onClick={() => fileInputRef.current?.click()}
              className="px-3 py-1.5 text-xs bg-slate-800 hover:bg-slate-700 text-slate-200 rounded font-medium transition-all"
            >
              Browse File...
            </button>
            <input
              ref={fileInputRef}
              type="file"
              accept=".mps,.gz"
              onChange={handleFileChange}
              className="hidden"
            />
          </div>
        )}

        <textarea
          value={content}
          onChange={(e) => onChangeContent(e.target.value)}
          placeholder={
            inputMode === 'structured'
              ? `VARIABLES\nx1 binary\nx2 binary\nx3 continuous\n\nMINIMIZE\n5*x1 + 7*x2 + 3*x3\n\nSUBJECT TO\nc1: x1 + x2 >= 1\nc2: x3 <= 10`
              : `NAME          model_name\nROWS\n N  obj\n E  c1\nCOLUMNS\n...`
          }
          className="flex-1 min-h-0 w-full bg-slate-900 border border-slate-800 rounded-lg p-4 font-mono text-xs text-slate-200 leading-relaxed focus:outline-none focus:border-indigo-500/50 resize-none overflow-y-auto transition-all placeholder:text-slate-700"
          spellCheck={false}
        />
      </div>

      {/* Sticky Fixed Bottom Action Bar */}
      <div className="shrink-0 p-4 border-t border-slate-800 flex items-center justify-between bg-slate-900/95 backdrop-blur z-10 shadow-xl">
        <div className="flex items-center gap-3">
          <button
            onClick={onValidate}
            disabled={isValidating || isSolving}
            className="px-4 py-2 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-lg font-medium text-xs flex items-center gap-2 transition-all disabled:opacity-50"
          >
            <CheckCircle className="w-4 h-4 text-emerald-400" />
            <span>{isValidating ? 'Validating...' : 'VALIDATE'}</span>
          </button>
        </div>

        <div className="flex items-center gap-3">
          {isSolving ? (
            <div className="flex items-center gap-3">
              <div className="flex items-center gap-2 text-xs font-mono text-amber-400 bg-amber-500/10 px-3 py-1.5 rounded border border-amber-500/20">
                <span className="w-2 h-2 rounded-full bg-amber-400 animate-ping"></span>
                <span>SOLVING... {elapsedSec.toFixed(1)}s</span>
              </div>
              <button
                onClick={onStopSolve}
                className="px-4 py-2 bg-rose-600/20 hover:bg-rose-600/30 text-rose-300 border border-rose-500/40 rounded-lg font-medium text-xs flex items-center gap-2 transition-all"
              >
                <Square className="w-3.5 h-3.5" />
                <span>STOP SOLVE</span>
              </button>
            </div>
          ) : (
            <button
              onClick={onSolve}
              disabled={!content.trim()}
              className="px-5 py-2.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded-lg font-bold text-xs flex items-center gap-2 shadow-lg shadow-indigo-600/20 transition-all active:scale-[0.98] disabled:opacity-50"
            >
              <Play className="w-4 h-4 fill-white" />
              <span>SOLVE MODEL</span>
            </button>
          )}
        </div>
      </div>
    </div>
  );
};
