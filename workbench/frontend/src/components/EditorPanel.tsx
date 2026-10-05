import React, { useRef, useEffect } from 'react';
import { Play, Check, Square, Upload, Bookmark, Code2, Trash2, FileText, Binary } from 'lucide-react';

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
  const textareaRef = useRef<HTMLTextAreaElement>(null);
  const gutterRef = useRef<HTMLDivElement>(null);

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    if (e.target.files && e.target.files[0]) {
      onFileUpload(e.target.files[0]);
    }
  };

  const handleScroll = (e: React.UIEvent<HTMLTextAreaElement>) => {
    if (gutterRef.current) {
      gutterRef.current.scrollTop = e.currentTarget.scrollTop;
    }
  };

  // Synchronize gutter on model content changes (e.g. model switching)
  useEffect(() => {
    if (gutterRef.current && textareaRef.current) {
      gutterRef.current.scrollTop = textareaRef.current.scrollTop;
    }
  }, [content]);

  const lines = content ? content.split('\n') : [''];
  const lineCount = lines.length;

  // Insert section template shortcut into editor
  const insertSection = (keyword: string) => {
    if (inputMode !== 'structured') return;
    const trimmed = content.trim();
    if (!trimmed.includes(keyword)) {
      const updated = trimmed ? `${trimmed}\n\n${keyword}\n` : `${keyword}\n`;
      onChangeContent(updated);
    }
  };

  return (
    <div className="flex flex-col h-full min-h-0 min-w-0 overflow-hidden bg-[#07090d] border-r border-white/[0.08] text-slate-200">
      {/* Primary Toolbar: Model Name & Format & Save */}
      <div className="shrink-0 p-3 border-b border-white/[0.08] bg-[#0a0e16] space-y-2.5">
        <div className="flex items-center justify-between gap-3">
          {/* Model Name Input */}
          <div className="flex-1 flex items-center min-w-0">
            <input
              type="text"
              value={name}
              onChange={(e) => onChangeName(e.target.value)}
              placeholder="Model Name"
              className="w-full bg-[#07090d] border border-white/[0.10] rounded-[5px] px-3 py-1.5 text-white font-medium text-xs focus:outline-none focus:border-white/30 transition-colors placeholder:text-slate-600"
            />
          </div>

          {/* Format Selector Tabs */}
          <div className="flex bg-[#07090d] rounded-[5px] border border-white/[0.10] p-0.5 font-mono text-[11px] shrink-0">
            <button
              onClick={() => onChangeInputMode('structured')}
              className={`flex items-center gap-1.5 px-2.5 py-1 rounded-[4px] transition-colors ${
                inputMode === 'structured'
                  ? 'bg-white/[0.12] text-white font-medium shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <FileText className="w-3 h-3 text-sky-400" />
              <span>Structured Model</span>
            </button>
            <button
              onClick={() => onChangeInputMode('mps')}
              className={`flex items-center gap-1.5 px-2.5 py-1 rounded-[4px] transition-colors ${
                inputMode === 'mps'
                  ? 'bg-white/[0.12] text-white font-medium shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <Binary className="w-3 h-3 text-emerald-400" />
              <span>MPS File</span>
            </button>
          </div>

          {/* Quick Action Icons */}
          <div className="flex items-center gap-1.5 shrink-0">
            {onSaveBenchmark && (
              <button
                onClick={onSaveBenchmark}
                className="px-2.5 py-1.5 rounded-[5px] border border-white/[0.10] bg-white/[0.04] hover:bg-white/[0.08] text-slate-200 text-xs font-medium flex items-center gap-1.5 transition-colors shadow-sm"
                title="Save model formulation to browser storage"
              >
                <Bookmark className="w-3.5 h-3.5 text-slate-400" />
                <span>Save</span>
              </button>
            )}

            {inputMode === 'structured' && onFormatContent && (
              <button
                onClick={onFormatContent}
                className="px-2.5 py-1.5 rounded-[5px] border border-white/[0.10] bg-white/[0.04] hover:bg-white/[0.08] text-slate-200 text-xs font-medium flex items-center gap-1.5 transition-colors shadow-sm"
                title="Format keywords and indentation"
              >
                <Code2 className="w-3.5 h-3.5 text-slate-400" />
                <span>Format</span>
              </button>
            )}

            {onClearContent && (
              <button
                onClick={onClearContent}
                className="p-1.5 rounded-[5px] border border-white/[0.08] hover:border-white/[0.16] hover:bg-white/[0.06] text-slate-400 hover:text-rose-300 transition-colors"
                title="Clear editor text"
              >
                <Trash2 className="w-3.5 h-3.5" />
              </button>
            )}
          </div>
        </div>

        {/* Secondary Bar: Syntax Section Helpers & Timeout Config */}
        <div className="flex items-center justify-between gap-3 text-xs pt-0.5">
          {inputMode === 'structured' ? (
            <div className="flex items-center gap-1.5 font-mono text-[10px] text-slate-400 overflow-x-auto py-0.5 min-w-0">
              <span className="text-slate-400 font-sans font-medium shrink-0">Sections:</span>
              {['VARIABLES', 'MINIMIZE', 'MAXIMIZE', 'SUBJECT TO', 'BOUNDS'].map((sec) => (
                <button
                  key={sec}
                  onClick={() => insertSection(sec)}
                  className="px-1.5 py-0.5 rounded bg-white/[0.03] hover:bg-white/[0.08] text-slate-400 hover:text-white border border-white/[0.06] shrink-0 transition-colors"
                >
                  {sec}
                </button>
              ))}
            </div>
          ) : (
            <div className="text-[11px] font-mono text-slate-500 truncate">
              Standard MPS formulation format
            </div>
          )}

          {/* Timeout Controller */}
          <div className="flex items-center gap-2 font-mono text-[11px] text-slate-400 bg-[#07090d] px-2.5 py-1 rounded-[5px] border border-white/[0.08] shrink-0">
            <span className="text-slate-400 font-sans">Timeout:</span>
            <input
              type="number"
              min="1"
              max="3600"
              value={timeoutSec}
              onChange={(e) => onChangeTimeoutSec(Math.max(1, Number(e.target.value)))}
              className="w-11 bg-white/[0.05] border border-white/[0.12] rounded px-1.5 py-0.5 text-right text-white focus:outline-none focus:border-white/30"
            />
            <span className="text-slate-500">sec</span>
          </div>
        </div>
      </div>

      {/* MPS Drag and Drop Zone if MPS Mode */}
      {inputMode === 'mps' && (
        <div className="shrink-0 p-3 bg-[#0a0e16]/80 border-b border-white/[0.08] flex items-center justify-between">
          <div className="flex items-center gap-2.5">
            <Upload className="w-4 h-4 text-emerald-400 shrink-0" />
            <div>
              <div className="text-xs font-medium text-slate-200">Load Benchmark File</div>
              <div className="text-[10px] text-slate-500 font-mono">Upload standard .mps or .mps.gz files</div>
            </div>
          </div>
          <button
            onClick={() => fileInputRef.current?.click()}
            className="px-3 py-1 text-xs bg-white/[0.06] hover:bg-white/[0.12] text-slate-200 border border-white/[0.10] rounded-[5px] transition-colors shrink-0"
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

      {/* Editor Main Canvas with Synchronized Line Numbers Gutter */}
      <div className="flex-1 min-h-0 min-w-0 relative flex overflow-hidden bg-[#06080c]">
        {/* Line Numbers Gutter */}
        <div
          ref={gutterRef}
          onWheel={(e) => {
            if (textareaRef.current) {
              const delta = e.deltaMode === 1 ? e.deltaY * 24 : e.deltaMode === 2 ? e.deltaY * 500 : e.deltaY;
              textareaRef.current.scrollTop += delta;
              if (gutterRef.current) {
                gutterRef.current.scrollTop = textareaRef.current.scrollTop;
              }
            }
          }}
          onClick={() => textareaRef.current?.focus()}
          aria-hidden="true"
          className="w-12 h-full shrink-0 py-3.5 pr-2 pl-1 select-none overflow-hidden text-right font-mono text-xs text-slate-600 bg-[#07090e] border-r border-white/[0.05] cursor-text"
          style={{ lineHeight: '1.5rem' }}
        >
          {Array.from({ length: lineCount }, (_, i) => (
            <div key={i + 1} className="h-6 leading-6 text-[11px]">
              {i + 1}
            </div>
          ))}
          {/* Scroll padding at bottom matching textarea */}
          <div className="h-20 shrink-0" aria-hidden="true" />
        </div>

        {/* Textarea Editor */}
        <textarea
          ref={textareaRef}
          value={content}
          onChange={(e) => onChangeContent(e.target.value)}
          onScroll={handleScroll}
          wrap="off"
          placeholder={
            inputMode === 'structured'
              ? `VARIABLES\nx1 binary\nx2 binary\nx3 continuous\n\nMINIMIZE\n5*x1 + 7*x2 + 3*x3\n\nSUBJECT TO\nc1: x1 + x2 >= 1\nc2: x3 <= 10`
              : `NAME          model_name\nROWS\n N  obj\n E  c1\nCOLUMNS\n...`
          }
          className="flex-1 min-h-0 min-w-0 w-full h-full bg-transparent p-3.5 pb-20 font-mono text-xs text-slate-100 leading-6 focus:outline-none resize-none overflow-y-auto overflow-x-auto whitespace-pre placeholder:text-slate-700 editor-scrollbar"
          spellCheck={false}
          style={{ lineHeight: '1.5rem' }}
        />
      </div>

      {/* Sticky Bottom Action Bar with Dominant Solve CTA */}
      <div className="shrink-0 px-4 py-2.5 border-t border-white/[0.08] flex items-center justify-between bg-[#0a0e16]">
        {/* Left: Validation Action */}
        <div className="flex items-center gap-3">
          <button
            onClick={onValidate}
            disabled={isValidating || isSolving}
            className="px-3.5 py-1.5 bg-white/[0.05] hover:bg-white/[0.10] text-slate-200 border border-white/[0.10] rounded-[5px] text-xs font-medium flex items-center gap-1.5 transition-colors disabled:opacity-50 shadow-sm cursor-pointer"
          >
            <Check className="w-3.5 h-3.5 text-slate-300" />
            <span>{isValidating ? 'Validating...' : 'Validate'}</span>
          </button>
          <span className="text-[11px] font-mono text-slate-500">
            {lineCount} {lineCount === 1 ? 'line' : 'lines'}
          </span>
        </div>

        {/* Right: Dominant Solve Execution Control */}
        <div className="flex items-center gap-3">
          {isSolving ? (
            <div className="flex items-center gap-2.5">
              <div className="flex items-center gap-2 text-xs font-mono text-amber-300 bg-amber-950/60 px-3 py-1.5 rounded-[5px] border border-amber-500/40">
                <span className="w-2 h-2 rounded-full bg-amber-400 animate-pulse"></span>
                <span>Solving ({elapsedSec.toFixed(1)}s)</span>
              </div>
              <button
                onClick={onStopSolve}
                className="px-3.5 py-1.5 bg-rose-950/80 hover:bg-rose-900 text-rose-200 border border-rose-800 rounded-[5px] text-xs font-medium flex items-center gap-1.5 transition-colors shadow-sm cursor-pointer"
              >
                <Square className="w-3 h-3" />
                <span>Stop Solve</span>
              </button>
            </div>
          ) : (
            <button
              onClick={onSolve}
              disabled={!content.trim()}
              className="btn-solve-primary px-5 py-2 text-xs flex items-center gap-2 disabled:opacity-40 disabled:cursor-not-allowed cursor-pointer"
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
