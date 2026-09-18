import React, { useState, useEffect, useRef, useCallback } from 'react';
import { ToastProvider, useToast } from './components/ToastContext';
import { TopBar } from './components/TopBar';
import { Sidebar } from './components/Sidebar';
import { EditorPanel } from './components/EditorPanel';
import { ProblemSummaryCard } from './components/ProblemSummaryCard';
import { PhaseTimeline } from './components/PhaseTimeline';
import { ResultPanel } from './components/ResultPanel';
import { SolutionTable } from './components/SolutionTable';
import { TelemetryPanel } from './components/TelemetryPanel';
import { MipProgressChart } from './components/MipProgressChart';
import { BranchAndBoundTree } from './components/BranchAndBoundTree';
import { SolverLogDrawer } from './components/SolverLogDrawer';
import { NewProblemModal } from './components/NewProblemModal';
import { PRESET_MODELS, PresetModel } from './samples/preset_models';
import { SolveResult, ValidationInfo, BackendHealth, PresetInputType } from './types';
import { Terminal } from 'lucide-react';

const API_BASE_URL = (import.meta.env.VITE_API_BASE_URL || '').replace(/\/$/, '');

function detectModelType(content: string, fallbackMode: PresetInputType): PresetInputType {
  const trimmed = content.trim();
  if (!trimmed) return fallbackMode;

  if (/^\s*(VARIABLES|MINIMIZE|MAXIMIZE)\b/i.test(trimmed) || /\bSUBJECT TO\b/i.test(trimmed)) {
    return 'structured';
  }

  if (/^\s*NAME\b/i.test(trimmed) || /\bROWS\b/i.test(trimmed) || /\bCOLUMNS\b/i.test(trimmed) || /\bENDATA\b/i.test(trimmed)) {
    return 'mps';
  }

  return fallbackMode;
}

const LOCAL_STORAGE_KEY = 'sovereign_saved_benchmarks';

function loadSavedBenchmarks(): PresetModel[] {
  try {
    const raw = localStorage.getItem(LOCAL_STORAGE_KEY);
    if (!raw) return [];
    const parsed = JSON.parse(raw);
    if (Array.isArray(parsed)) {
      return parsed.map((item) => ({ ...item, isBuiltIn: false }));
    }
  } catch (e) {
    console.error('Failed to load saved benchmarks from localStorage', e);
  }
  return [];
}

function persistSavedBenchmarks(benchmarks: PresetModel[]) {
  try {
    localStorage.setItem(LOCAL_STORAGE_KEY, JSON.stringify(benchmarks));
  } catch (e) {
    console.error('Failed to persist benchmarks to localStorage', e);
  }
}

function WorkbenchMain() {
  const { addToast } = useToast();

  const [savedBenchmarks, setSavedBenchmarks] = useState<PresetModel[]>(() => loadSavedBenchmarks());
  const [activePresetId, setActivePresetId] = useState<string>('assignment_4x4');
  const [problemName, setProblemName] = useState<string>('Assignment 4x4');
  const [inputMode, setInputMode] = useState<PresetInputType>('structured');
  const [content, setContent] = useState<string>(PRESET_MODELS[0].content);
  const [timeoutSec, setTimeoutSec] = useState<number>(30);

  const [isValidating, setIsValidating] = useState<boolean>(false);
  const [isSolving, setIsSolving] = useState<boolean>(false);
  const [elapsedSec, setElapsedSec] = useState<number>(0);
  const [activeSolveId, setActiveSolveId] = useState<string | null>(null);

  const [validationInfo, setValidationInfo] = useState<ValidationInfo | null>(null);
  const [solveResult, setSolveResult] = useState<SolveResult | null>(null);

  const [isLogDrawerOpen, setIsLogDrawerOpen] = useState<boolean>(false);
  const [isNewModalOpen, setIsNewModalOpen] = useState<boolean>(false);

  const [health, setHealth] = useState<BackendHealth>({
    connected: false,
    version: '0.2.0',
    solverReady: false,
    checking: true
  });

  const timerRef = useRef<any>(null);

  // Poll backend connection health
  const checkBackendHealth = useCallback(async () => {
    try {
      const res = await fetch(`${API_BASE_URL}/api/health`);
      if (res.ok) {
        const data = await res.json();
        setHealth({
          connected: true,
          version: data.version || '0.2.0',
          solverReady: data.solver_ready ?? true,
          checking: false
        });
      } else {
        setHealth((prev) => ({ ...prev, connected: false, checking: false }));
      }
    } catch (err) {
      setHealth((prev) => ({ ...prev, connected: false, checking: false }));
    }
  }, []);

  useEffect(() => {
    checkBackendHealth();
    const interval = setInterval(checkBackendHealth, 5000);
    return () => clearInterval(interval);
  }, [checkBackendHealth]);

  // Timer for solve duration
  useEffect(() => {
    if (isSolving) {
      const startTime = Date.now();
      timerRef.current = setInterval(() => {
        setElapsedSec((Date.now() - startTime) / 1000);
      }, 100);
    } else {
      if (timerRef.current) clearInterval(timerRef.current);
    }
    return () => {
      if (timerRef.current) clearInterval(timerRef.current);
    };
  }, [isSolving]);

  const handleSelectPreset = (preset: PresetModel) => {
    setActivePresetId(preset.id);
    setProblemName(preset.name);
    setInputMode(preset.type);
    setContent(preset.content);
    setValidationInfo(null);
    setSolveResult(null);
    addToast('info', 'Preset Loaded', `Loaded "${preset.name}"`);
  };

  const handleNewProblemClick = () => {
    setIsNewModalOpen(true);
  };

  const handleCreateProblem = (name: string, type: PresetInputType, initialContent = '') => {
    const newId = `custom_${Date.now()}`;
    const defaultBody = initialContent || (type === 'structured'
      ? `VARIABLES\nx1 binary\nx2 binary\n\nMAXIMIZE\n10 x1 + 15 x2\n\nSUBJECT TO\nc1: 2 x1 + 3 x2 <= 4\n`
      : `NAME          NEWPROB\nROWS\n N  OBJ\nCOLUMNS\nENDATA\n`);

    const newBenchmark: PresetModel = {
      id: newId,
      name: name.trim() || 'New Model',
      type,
      content: defaultBody,
      description: `Created on ${new Date().toLocaleDateString()}`,
      isBuiltIn: false
    };

    const updated = [newBenchmark, ...savedBenchmarks];
    setSavedBenchmarks(updated);
    persistSavedBenchmarks(updated);

    setActivePresetId(newId);
    setProblemName(newBenchmark.name);
    setInputMode(type);
    setContent(defaultBody);
    setValidationInfo(null);
    setSolveResult(null);
    addToast('success', 'Problem Created', `Created "${newBenchmark.name}"`);
  };

  const handleSaveCurrentModel = () => {
    if (!content.trim()) {
      addToast('warning', 'Save Benchmark', 'Editor content cannot be empty');
      return;
    }

    const effectiveName = problemName.trim() || 'Untitled Model';
    const isExistingCustom = savedBenchmarks.some((b) => b.id === activePresetId && !b.isBuiltIn);

    let updated: PresetModel[];
    let targetId = activePresetId;

    if (isExistingCustom) {
      updated = savedBenchmarks.map((b) =>
        b.id === activePresetId
          ? { ...b, name: effectiveName, content, type: inputMode }
          : b
      );
      addToast('success', 'Benchmark Saved', `Updated "${effectiveName}"`);
    } else {
      targetId = `custom_${Date.now()}`;
      const newBenchmark: PresetModel = {
        id: targetId,
        name: effectiveName,
        type: inputMode,
        content,
        description: `Saved on ${new Date().toLocaleDateString()}`,
        isBuiltIn: false
      };
      updated = [newBenchmark, ...savedBenchmarks];
      setActivePresetId(targetId);
      addToast('success', 'Benchmark Saved', `Saved "${effectiveName}" to workbench benchmarks`);
    }

    setSavedBenchmarks(updated);
    persistSavedBenchmarks(updated);
  };

  const handleDeleteBenchmark = (benchmark: PresetModel) => {
    if (benchmark.isBuiltIn) {
      addToast('warning', 'Protected Model', 'Built-in standard benchmarks cannot be deleted');
      return;
    }

    const updated = savedBenchmarks.filter((b) => b.id !== benchmark.id);
    setSavedBenchmarks(updated);
    persistSavedBenchmarks(updated);
    addToast('info', 'Benchmark Deleted', `Removed "${benchmark.name}" from saved benchmarks`);

    if (activePresetId === benchmark.id) {
      if (updated.length > 0) {
        const next = updated[0];
        setActivePresetId(next.id);
        setProblemName(next.name);
        setInputMode(next.type);
        setContent(next.content);
      } else {
        const fallback = PRESET_MODELS[0];
        setActivePresetId(fallback.id);
        setProblemName(fallback.name);
        setInputMode(fallback.type);
        setContent(fallback.content);
      }
      setValidationInfo(null);
      setSolveResult(null);
    }
  };

  const handleValidate = async () => {
    if (!content.trim()) {
      addToast('warning', 'Validation Error', 'Editor content cannot be empty');
      return;
    }

    const effectiveType = detectModelType(content, inputMode);
    if (effectiveType !== inputMode) {
      setInputMode(effectiveType);
    }

    setIsValidating(true);
    setValidationInfo(null);
    setSolveResult(null);

    try {
      const res = await fetch(`${API_BASE_URL}/api/validate`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          type: effectiveType,
          content,
          modelName: problemName
        })
      });

      if (!res.ok) {
        throw new Error(`Server returned HTTP ${res.status}`);
      }

      const data = await res.json();
      setValidationInfo(data);

      if (data.success) {
        addToast('success', 'Model Validated', `Valid model with ${data.variables} vars, ${data.constraints} cons`);
      } else {
        addToast('error', 'Validation Failed', data.error || 'Syntax error in model definition');
      }
    } catch (err: any) {
      const info = { success: false, error: err.message };
      setValidationInfo(info);
      addToast('error', 'Validation Failed', err.message);
    } finally {
      setIsValidating(false);
    }
  };

  const handleSolve = async () => {
    if (!content.trim()) return;

    if (!health.connected) {
      addToast('error', 'Backend Unavailable', 'Cannot solve because backend is disconnected.');
      return;
    }

    const effectiveType = detectModelType(content, inputMode);
    if (effectiveType !== inputMode) {
      setInputMode(effectiveType);
    }

    setIsSolving(true);
    setElapsedSec(0);
    setSolveResult(null);
    setValidationInfo(null);
    addToast('info', 'Solve Started', `Solving "${problemName}" with timeout ${timeoutSec}s`);

    try {
      const res = await fetch(`${API_BASE_URL}/api/solve`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'text/event-stream'
        },
        body: JSON.stringify({
          type: effectiveType,
          content,
          modelName: problemName,
          timeout_sec: timeoutSec,
          threads: 1
        })
      });

      if (!res.ok) {
        throw new Error(`Server returned HTTP ${res.status}`);
      }

      const contentType = res.headers.get('content-type') || '';
      if (!contentType.includes('text/event-stream')) {
        const data: SolveResult = await res.json();
        setSolveResult(data);
        if (data.solveId) setActiveSolveId(data.solveId);
        return;
      }

      const reader = res.body?.getReader();
      if (!reader) {
        throw new Error('ReadableStream not supported by browser environment');
      }

      const decoder = new TextDecoder('utf-8');
      let buffer = '';

      const accumulatedLogs: any[] = [];
      const accumulatedCheckpoints: any[] = [];
      const accumulatedNodes: any[] = [];

      const handleEventPayload = (eventData: any) => {
        const timestampStr = new Date().toLocaleTimeString();
        switch (eventData.event) {
          case 'solve_started':
            accumulatedLogs.push({
              timestamp: timestampStr,
              phase: 'INIT',
              message: `Solve started for ${eventData.model_name} (${eventData.sense}, ${eventData.variables} vars, ${eventData.constraints} cons)`
            });
            break;

          case 'root_lp':
            accumulatedLogs.push({
              timestamp: timestampStr,
              phase: 'ROOT_LP',
              message: `Root LP finished: objective = ${eventData.root_lp_objective?.toFixed(6)}, fractional binaries = ${eventData.fractional_binaries}`
            });
            accumulatedCheckpoints.push({
              time_sec: (eventData.timestamp_ms || 0) / 1000,
              node: 0,
              best_bound: eventData.root_lp_objective
            });
            break;

          case 'node_created':
            // Tree Invariant Checks (Phase 7)
            if (eventData.node_id !== 0 && eventData.parent_id !== -1) {
              const parent = accumulatedNodes.find((n) => n.id === eventData.parent_id);
              if (!parent) {
                console.warn(`[Telemetry Error] Missing parent_id ${eventData.parent_id} for node ${eventData.node_id}`);
              } else if (eventData.depth !== parent.depth + 1) {
                console.warn(`[Telemetry Error] Depth mismatch for node ${eventData.node_id}: got ${eventData.depth}, expected ${parent.depth + 1}`);
              }
            }
            if (!accumulatedNodes.some((n) => n.id === eventData.node_id)) {
              accumulatedNodes.push({
                id: eventData.node_id,
                parentId: eventData.parent_id,
                depth: eventData.depth,
                bound: eventData.bound,
                status: 'open',
                incumbentContribution: eventData.branch_var
              });
            }
            break;

          case 'node_solved':
            {
              const nodeIndex = accumulatedNodes.findIndex((n) => n.id === eventData.node_id);
              if (nodeIndex >= 0) {
                accumulatedNodes[nodeIndex] = {
                  ...accumulatedNodes[nodeIndex],
                  status: 'processing',
                  bound: eventData.lp_objective
                };
              }
            }
            break;

          case 'node_fathomed':
            {
              const nodeIndex = accumulatedNodes.findIndex((n) => n.id === eventData.node_id);
              const statusMap =
                eventData.reason === 'infeasible'
                  ? 'infeasible'
                  : eventData.reason === 'bound_pruned'
                  ? 'bound_pruned'
                  : eventData.reason === 'integer_feasible'
                  ? 'integer_feasible'
                  : 'fathomed';
              if (nodeIndex >= 0) {
                accumulatedNodes[nodeIndex] = {
                  ...accumulatedNodes[nodeIndex],
                  status: statusMap,
                  bound: eventData.bound
                };
              }
            }
            break;

          case 'incumbent_found':
            accumulatedLogs.push({
              timestamp: timestampStr,
              phase: 'INCUMBENT',
              message: `New incumbent found: ${eventData.objective?.toFixed(6)} via ${eventData.source || 'B&B'}`
            });
            accumulatedCheckpoints.push({
              time_sec: eventData.time_sec ?? (eventData.timestamp_ms || 0) / 1000,
              node: eventData.node_id,
              incumbent: eventData.objective
            });
            break;

          case 'bound_improved':
            accumulatedCheckpoints.push({
              time_sec: eventData.time_sec ?? (eventData.timestamp_ms || 0) / 1000,
              node: eventData.node_id,
              best_bound: eventData.best_bound
            });
            break;

          case 'solve_finished':
            accumulatedLogs.push({
              timestamp: timestampStr,
              phase: 'FINAL',
              message: `Solve finished with status ${eventData.status_name}, obj = ${eventData.objective_value}`
            });
            break;

          default:
            break;
        }

        // Live state update during solve execution
        setSolveResult({
          success: true,
          status_name: 'Solving',
          objective_value: accumulatedCheckpoints.filter((c) => c.incumbent !== undefined).pop()?.incumbent,
          best_bound: accumulatedCheckpoints.filter((c) => c.best_bound !== undefined).pop()?.best_bound,
          nodes_explored: accumulatedNodes.filter((n) => n.status !== 'open').length,
          nodes_created: accumulatedNodes.length,
          progressHistory: [...accumulatedCheckpoints],
          treeNodes: [...accumulatedNodes],
          logs: [...accumulatedLogs]
        });
      };

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;

        buffer += decoder.decode(value, { stream: true });
        const lines = buffer.split('\n');
        buffer = lines.pop() || '';

        for (const line of lines) {
          const trimmed = line.trim();
          if (!trimmed.startsWith('data: ')) continue;
          const payloadStr = trimmed.substring(6).trim();

          if (payloadStr.startsWith('[FINAL_RESULT]')) {
            const jsonStr = payloadStr.substring(14).trim();
            const finalResult: SolveResult = JSON.parse(jsonStr);

            // Reconcile final result with telemetry events (Phase 10)
            finalResult.progressHistory = accumulatedCheckpoints;
            finalResult.treeNodes = accumulatedNodes;
            finalResult.logs = accumulatedLogs;
            setSolveResult(finalResult);

            if (finalResult.solveId) setActiveSolveId(finalResult.solveId);

            if (finalResult.success && finalResult.status_name === 'Optimal') {
              addToast('success', 'Solve Complete: OPTIMAL', `Obj: ${finalResult.objective_value?.toFixed(4)} in ${finalResult.solve_time_ms?.toFixed(0)} ms`);
            } else if (finalResult.status_name === 'TimeLimit') {
              addToast('warning', 'Solve Stopped: TIME LIMIT', `Incumbent: ${finalResult.objective_value?.toFixed(4) ?? 'N/A'}, Best Bound: ${finalResult.best_bound?.toFixed(4) ?? 'N/A'}`);
            } else if (finalResult.status_name === 'UserInterrupted') {
              addToast('warning', 'Solve Cancelled', 'Solve process was stopped by user');
            } else if (!finalResult.success) {
              addToast('error', 'Solve Failed', finalResult.message || finalResult.error || 'Solver returned failure status');
            } else {
              addToast('info', `Status: ${finalResult.status_name}`, finalResult.message || 'Solve finished');
            }
            break;
          }

          try {
            const eventData = JSON.parse(payloadStr);
            handleEventPayload(eventData);
          } catch (e) {}
        }
      }
    } catch (err: any) {
      setSolveResult({
        success: false,
        status_name: 'NumericalFailure',
        message: err.message
      });
      addToast('error', 'Solve Error', err.message);
    } finally {
      setIsSolving(false);
    }
  };

  const handleStopSolve = async () => {
    if (activeSolveId) {
      try {
        await fetch(`${API_BASE_URL}/api/stop-solve`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ solveId: activeSolveId })
        });
      } catch (e) {}
    }
    setIsSolving(false);
    setSolveResult({
      success: false,
      status_name: 'UserInterrupted',
      message: 'Solve process was cancelled by user request.'
    });
    addToast('warning', 'Solve Stopped', 'Stop requested for active solve');
  };

  const handleFileUpload = async (file: File) => {
    const formData = new FormData();
    formData.append('file', file);
    addToast('info', 'Uploading File', `Processing ${file.name}...`);

    try {
      const res = await fetch(`${API_BASE_URL}/api/upload-mps`, {
        method: 'POST',
        body: formData
      });
      const data = await res.json();
      if (data.success) {
        const newId = `upload_${Date.now()}`;
        const cleanName = data.filename.replace(/\.mps|\.gz/g, '');
        const uploadedModel: PresetModel = {
          id: newId,
          name: cleanName,
          type: 'mps',
          content: data.content,
          description: `Uploaded file ${data.filename}`,
          isBuiltIn: false
        };
        const updated = [uploadedModel, ...savedBenchmarks];
        setSavedBenchmarks(updated);
        persistSavedBenchmarks(updated);

        setInputMode('mps');
        setContent(data.content);
        setProblemName(cleanName);
        setActivePresetId(newId);
        setValidationInfo(null);
        setSolveResult(null);
        addToast('success', 'Upload Successful', `Loaded "${data.filename}"`);
      } else {
        addToast('error', 'Upload Error', data.error);
      }
    } catch (err: any) {
      addToast('error', 'Upload Failed', err.message);
    }
  };

  const handleFormatContent = () => {
    if (inputMode !== 'structured') return;
    const formatted = content
      .replace(/\s*VARIABLES\s*/gi, 'VARIABLES\n')
      .replace(/\s*MINIMIZE\s*/gi, '\n\nMINIMIZE\n')
      .replace(/\s*MAXIMIZE\s*/gi, '\n\nMAXIMIZE\n')
      .replace(/\s*SUBJECT TO\s*/gi, '\n\nSUBJECT TO\n')
      .trim();
    setContent(formatted);
    addToast('info', 'Model Formatted', 'Cleaned up section keywords');
  };

  const handleClearContent = () => {
    if (content.trim() && window.confirm('Are you sure you want to clear the editor content?')) {
      setContent('');
      setValidationInfo(null);
      setSolveResult(null);
      addToast('info', 'Editor Cleared', 'Cleared model text');
    }
  };

  const handleLoadExample = () => {
    handleSelectPreset(PRESET_MODELS[0]);
  };

  return (
    <div className="flex flex-col h-screen w-screen overflow-hidden bg-slate-950 text-slate-100 font-sans">
      {/* Top Application Bar */}
      <TopBar health={health} />

      {/* Main Content Workspace */}
      <div className="flex-1 flex overflow-hidden">
        {/* Left Sidebar */}
        <Sidebar
          activePresetId={activePresetId}
          builtInPresets={PRESET_MODELS}
          savedBenchmarks={savedBenchmarks}
          onSelectPreset={handleSelectPreset}
          onDeleteBenchmark={handleDeleteBenchmark}
          onNewProblem={handleNewProblemClick}
          health={health}
        />

        {/* Central Model Editor & Right Results Panel */}
        <main className="flex-1 grid grid-cols-12 h-full overflow-hidden">
          {/* Model Editor Column (7 Cols) */}
          <div className="col-span-12 lg:col-span-7 h-full flex flex-col min-w-0">
            <EditorPanel
              name={problemName}
              onChangeName={setProblemName}
              inputMode={inputMode}
              onChangeInputMode={setInputMode}
              content={content}
              onChangeContent={setContent}
              timeoutSec={timeoutSec}
              onChangeTimeoutSec={setTimeoutSec}
              isSolving={isSolving}
              isValidating={isValidating}
              elapsedSec={elapsedSec}
              validationInfo={validationInfo}
              onValidate={handleValidate}
              onSolve={handleSolve}
              onStopSolve={handleStopSolve}
              onLoadPreset={handleSelectPreset}
              onFileUpload={handleFileUpload}
              onFormatContent={handleFormatContent}
              onClearContent={handleClearContent}
              onLoadExample={handleLoadExample}
              onSaveBenchmark={handleSaveCurrentModel}
            />
          </div>

          {/* Results, Charts & Telemetry Column (5 Cols) */}
          <div className="col-span-12 lg:col-span-5 h-full overflow-y-auto bg-slate-900 border-l border-slate-800 flex flex-col min-w-0">
            {/* Solver Log Drawer Toggle Bar */}
            <div className="p-3 bg-slate-900 border-b border-slate-800 flex items-center justify-between shrink-0">
              <span className="text-xs font-medium text-slate-400">Results</span>
              <button
                onClick={() => setIsLogDrawerOpen(true)}
                className="px-2.5 py-1 text-xs bg-slate-800 hover:bg-slate-700 text-slate-300 font-mono rounded flex items-center gap-1.5 border border-slate-700 transition-colors"
              >
                <Terminal className="w-3.5 h-3.5 text-slate-400" />
                <span>Log Drawer</span>
              </button>
            </div>

            {/* Results Body: Single Empty State vs Progressively Revealed Components */}
            {!solveResult && !validationInfo && !isSolving ? (
              <div className="flex-1 flex flex-col items-center justify-center p-8 text-center text-slate-500 font-sans space-y-2">
                <div className="text-slate-300 font-medium text-sm">No solution yet</div>
                <p className="text-xs text-slate-400 max-w-xs leading-relaxed">
                  Load a model from the sidebar or create a new problem, then run <span className="text-slate-200 font-semibold">Solve Model</span>.
                </p>
              </div>
            ) : (
              <div className="flex-1 overflow-y-auto divide-y divide-slate-800">
                {/* Model Summary Card */}
                <ProblemSummaryCard
                  name={problemName}
                  type={inputMode}
                  telemetry={solveResult?.telemetry}
                  validationInfo={validationInfo}
                />

                {/* Solver Phase Execution Timeline */}
                <PhaseTimeline
                  validationInfo={validationInfo}
                  result={solveResult}
                  isSolving={isSolving}
                  isValidating={isValidating}
                />

                {/* Primary Result Status & Metric Cards */}
                <ResultPanel result={solveResult} validationInfo={validationInfo} isSolving={isSolving} />

                {/* Solution Summary & Variable Solution Table */}
                <SolutionTable
                  solution={solveResult?.solution}
                  telemetry={solveResult?.telemetry}
                  statusName={solveResult?.status_name}
                />

                {/* Engine Diagnostics & Telemetry Panel */}
                <TelemetryPanel telemetry={solveResult?.telemetry} />

                {/* MIP Progress Convergence Chart */}
                <MipProgressChart
                  history={solveResult?.progressHistory}
                  sense={solveResult?.telemetry?.sense}
                  objective={solveResult?.objective_value}
                  bestBound={solveResult?.best_bound}
                  gap={solveResult?.mip_gap}
                  isSolving={isSolving}
                />

                {/* Branch and Bound Search Tree */}
                <BranchAndBoundTree treeNodes={solveResult?.treeNodes} isSolving={isSolving} />
              </div>
            )}
          </div>
        </main>
      </div>

      {/* New Problem Creation Modal */}
      <NewProblemModal
        isOpen={isNewModalOpen}
        onClose={() => setIsNewModalOpen(false)}
        onCreate={handleCreateProblem}
      />

      {/* Solver Log Drawer */}
      <SolverLogDrawer
        isOpen={isLogDrawerOpen}
        onClose={() => setIsLogDrawerOpen(false)}
        logs={solveResult?.logs}
        modelName={problemName}
      />
    </div>
  );
}

export function App() {
  return (
    <ToastProvider>
      <WorkbenchMain />
    </ToastProvider>
  );
}

export default App;
