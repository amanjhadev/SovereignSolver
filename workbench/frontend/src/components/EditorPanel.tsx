import React, { useRef } from 'react';
import { Play, Check, Square, Upload, Bookmark, Code2, Trash2 } from 'lucide-react';
import { PresetModel } from '../types';

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
  onFileUpload: (file: File) => void;
  onFormatContent?: () => void;
  onClearContent?: () => void;
  onSaveBenchmark?: () => void;
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
  onFileUpload,
  onFormatContent,
  onClearContent,
  onSaveBenchmark
}) => {
  const fileInputRef = useRef<HTMLInputElement>(null);

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    if (e.target.files && e.target.files[0]) {
      onFileUpload(e.target.files[0]);
    }
  };

  const lineCount = content ? content.split('\n').length : 0;

  return (
    <div className="flex flex-col h-full overflow-hidden bg-slate-950 border-r border-slate-800">
      {/* Top Toolbar */}
      <div className="shrink-0 p-3 border-b border-slate-800 bg-slate-900/60 space-y-2.5">
        {/* Model Name & Primary Actions */}
        <div className="flex items-center justify-between gap-3">
          <div className="flex-1 flex items-center gap-2">
            <input
              type="text"
              value={name}
              onChange={(e) => onChangeName(e.target.value)}
              placeholder="Model Name"
              className="w-full bg-slate-950 border border-slate-700 rounded px-2.5 py-1.5 text-slate-100 font-medium text-xs focus:outline-none focus:border-indigo-500 transition-colors placeholder:text-slate-600"
            />
          </div>

          <div className="flex items-center gap-1.5 shrink-0">
            {onSaveBenchmark && (
              <button
                onClick={onSaveBenchmark}
                className="px-2.5 py-1 rounded border border-slate-700 bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs font-medium flex items-center gap-1.5 transition-colors"
                title="Save model to your benchmarks"
              >
                <Bookmark className="w-3.5 h-3.5 text-slate-400" />
                <span>Save</span>
              </button>
            )}

            {inputMode === 'structured' && onFormatContent && (
              <button
                onClick={onFormatContent}
                className="px-2.5 py-1 rounded border border-slate-700 bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs font-medium flex items-center gap-1.5 transition-colors"
                title="Format model sections"
              >
                <Code2 className="w-3.5 h-3.5 text-slate-400" />
                <span>Format</span>
              </button>
            )}

            {onClearContent && (
              <button
                onClick={onClearContent}
                className="p-1 rounded border border-slate-800 hover:border-slate-700 text-slate-500 hover:text-slate-300 transition-colors"
                title="Clear editor content"
              >
                <Trash2 className="w-3.5 h-3.5" />
              </button>
            )}
          </div>
        </div>

        {/* Input Mode Selector & Timeout Configuration */}
        <div className="flex items-center justify-between gap-3 text-xs">
          {/* Format Tabs */}
          <div className="flex bg-slate-950 rounded border border-slate-800 p-0.5 font-mono text-[11px]">
            <button
              onClick={() => onChangeInputMode('structured')}
              className={`px-2.5 py-1 rounded transition-colors ${
                inputMode === 'structured'
                  ? 'bg-slate-800 text-slate-100 font-medium'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              Structured Model
            </button>
            <button
              onClick={() => onChangeInputMode('mps')}
              className={`px-2.5 py-1 rounded transition-colors ${
                inputMode === 'mps'
                  ? 'bg-slate-800 text-slate-100 font-medium'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              MPS File
            </button>
          </div>

          {/* Timeout Setting */}
          <div className="flex items-center gap-2 font-mono text-[11px] text-slate-400 bg-slate-950 px-2.5 py-1 rounded border border-slate-800">
            <span>Timeout:</span>
            <input
              type="number"
              min="1"
              max="3600"
              value={timeoutSec}
              onChange={(e) => onChangeTimeoutSec(Math.max(1, Number(e.target.value)))}
              className="w-12 bg-slate-900 border border-slate-700 rounded px-1.5 py-0.5 text-right text-slate-200 focus:outline-none focus:border-indigo-500"
            />
            <span className="text-slate-500">sec</span>
          </div>
        </div>
      </div>

      {/* Structured Syntax Guide Bar */}
      {inputMode === 'structured' && (
        <div className="px-4 py-1.5 bg-slate-950 border-b border-slate-800/80 font-mono text-[11px] text-slate-500 flex items-center justify-between">
          <div className="flex items-center gap-3">
            <span className="text-slate-400 font-medium">Sections:</span>
            <span>VARIABLES</span>
            <span className="text-slate-700">·</span>
            <span>MINIMIZE / MAXIMIZE</span>
            <span className="text-slate-700">·</span>
            <span>SUBJECT TO</span>
            <span className="text-slate-700">·</span>
            <span>BOUNDS</span>
          </div>
          <div>{lineCount} lines</div>
        </div>
      )}

      {/* Editor Content Area */}
      <div className="flex-1 min-h-0 relative flex flex-col p-3 overflow-hidden">
        {inputMode === 'mps' && (
          <div className="shrink-0 mb-2 flex items-center justify-between bg-slate-900 border border-slate-800 p-2.5 rounded">
            <div className="flex items-center gap-2">
              <Upload className="w-4 h-4 text-slate-400" />
              <div>
                <div className="text-xs font-medium text-slate-200">Load MPS / MPS.GZ file</div>
                <div className="text-[10px] text-slate-500">Standard MPS or gzip-compressed instances</div>
              </div>
            </div>
            <button
              onClick={() => fileInputRef.current?.click()}
              className="px-3 py-1 text-xs bg-slate-800 hover:bg-slate-700 text-slate-200 border border-slate-700 rounded transition-colors"
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
          className="flex-1 min-h-0 w-full bg-slate-950 border border-slate-800 rounded p-3.5 font-mono text-xs text-slate-100 leading-relaxed focus:outline-none focus:border-slate-700 resize-none overflow-y-auto placeholder:text-slate-700"
          spellCheck={false}
        />
      </div>

      {/* Sticky Bottom Action Bar */}
      <div className="shrink-0 px-4 py-2.5 border-t border-slate-800 flex items-center justify-between bg-slate-900">
        <div className="flex items-center gap-3">
          <button
            onClick={onValidate}
            disabled={isValidating || isSolving}
            className="px-3.5 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 border border-slate-700 rounded text-xs font-medium flex items-center gap-1.5 transition-colors disabled:opacity-50"
          >
            <Check className="w-3.5 h-3.5 text-slate-300" />
            <span>{isValidating ? 'Validating...' : 'Validate'}</span>
          </button>
        </div>

        <div className="flex items-center gap-3">
          {isSolving ? (
            <div className="flex items-center gap-2.5">
              <div className="flex items-center gap-2 text-xs font-mono text-amber-300 bg-amber-950/60 px-2.5 py-1 rounded border border-amber-800/60">
                <span className="w-2 h-2 rounded-full bg-amber-400"></span>
                <span>Solving ({elapsedSec.toFixed(1)}s)</span>
              </div>
              <button
                onClick={onStopSolve}
                className="px-3 py-1.5 bg-rose-950/80 hover:bg-rose-900 text-rose-200 border border-rose-800 rounded text-xs font-medium flex items-center gap-1.5 transition-colors"
              >
                <Square className="w-3 h-3" />
                <span>Stop Solve</span>
              </button>
            </div>
          ) : (
            <button
              onClick={onSolve}
              disabled={!content.trim()}
              className="px-4 py-1.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded text-xs font-semibold flex items-center gap-1.5 transition-colors disabled:opacity-50 shadow-sm"
            >
              <Play className="w-3.5 h-3.5 fill-current" />
              <span>Solve Model</span>
            </button>
          )}
        </div>
      </div>
    </div>
  );
};
