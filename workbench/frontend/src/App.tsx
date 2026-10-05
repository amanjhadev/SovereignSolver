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
import { SolveResult, ValidationInfo, BackendHealth, PresetInputType, PresetModel } from './types';
import { Terminal, Crosshair } from 'lucide-react';

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
      return parsed.map((item) => ({
        id: String(item.id || `custom_${Date.now()}_${Math.random().toString(36).substring(2, 7)}`),
        name: String(item.name || 'Untitled Model'),
        type: (item.type === 'mps' ? 'mps' : 'structured') as PresetInputType,
        description: String(item.description || ''),
        content: typeof item.content === 'string' ? item.content : '',
        varsSummary: typeof item.varsSummary === 'string' ? item.varsSummary : undefined,
        consSummary: typeof item.consSummary === 'string' ? item.consSummary : undefined,
        isBuiltIn: false
      }));
    }
  } catch (e) {
    console.error('Failed to load saved models from localStorage', e);
  }
  return [];
}

function persistSavedBenchmarks(benchmarks: PresetModel[]) {
  try {
    localStorage.setItem(LOCAL_STORAGE_KEY, JSON.stringify(benchmarks));
  } catch (e) {
    console.error('Failed to persist models to localStorage', e);
  }
}

function WorkbenchMain() {
  const { addToast } = useToast();

  const [savedBenchmarks, setSavedBenchmarks] = useState<PresetModel[]>(() => loadSavedBenchmarks());
  const [activePresetId, setActivePresetId] = useState<string>('');
  const [problemName, setProblemName] = useState<string>('New Problem');
  const [inputMode, setInputMode] = useState<PresetInputType>('structured');
  const [content, setContent] = useState<string>('');
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

  const handleSelectSavedModel = (model: PresetModel) => {
    const canonical = savedBenchmarks.find((b) => b.id === model.id) || model;
    if (!canonical) return;

    setActivePresetId(canonical.id);
    setProblemName(canonical.name);
    setInputMode(canonical.type);
    setContent(canonical.content ?? '');

    // Fully clear transient run state
    setValidationInfo(null);
    setSolveResult(null);
    setActiveSolveId(null);
    setElapsedSec(0);
    setIsSolving(false);

    addToast('info', 'Model Loaded', `Loaded "${canonical.name}"`);
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
    setActiveSolveId(null);
    setElapsedSec(0);
    setIsSolving(false);
    addToast('success', 'Model Created', `Created "${newBenchmark.name}"`);
  };

  const handleSaveCurrentModel = () => {
    if (!content.trim()) {
      addToast('warning', 'Save Model', 'Editor content cannot be empty');
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
      addToast('success', 'Model Saved', `Updated "${effectiveName}"`);
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
      addToast('success', 'Model Saved', `Saved "${effectiveName}" to Saved Models`);
    }

    setSavedBenchmarks(updated);
    persistSavedBenchmarks(updated);
  };

  const handleDeleteBenchmark = (benchmark: PresetModel) => {
    const updated = savedBenchmarks.filter((b) => b.id !== benchmark.id);
    setSavedBenchmarks(updated);
    persistSavedBenchmarks(updated);
    addToast('info', 'Model Deleted', `Removed "${benchmark.name}" from saved models`);

    if (activePresetId === benchmark.id) {
      if (updated.length > 0) {
        const next = updated[0];
        setActivePresetId(next.id);
        setProblemName(next.name);
        setInputMode(next.type);
        setContent(next.content ?? '');
      } else {
        setActivePresetId('');
        setProblemName('New Problem');
        setInputMode('structured');
        setContent('');
      }
      setValidationInfo(null);
      setSolveResult(null);
      setActiveSolveId(null);
      setElapsedSec(0);
      setIsSolving(false);
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

            // Reconcile final result with telemetry events
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

  return (
    <div className="flex flex-col min-h-screen lg:h-screen w-full lg:w-screen overflow-x-hidden overflow-y-auto lg:overflow-hidden bg-[#07090d] text-slate-100 font-sans">
      {/* Top Application Bar */}
      <TopBar health={health} />

      {/* Main Content Three-Zone Workspace */}
      <div className="flex-1 min-h-0 min-w-0 flex flex-col lg:flex-row overflow-visible lg:overflow-hidden">
        {/* Left Sidebar: Saved Models Project Explorer */}
        <Sidebar
          activePresetId={activePresetId}
          savedBenchmarks={savedBenchmarks}
          onSelectPreset={handleSelectSavedModel}
          onDeleteBenchmark={handleDeleteBenchmark}
          onNewProblem={handleNewProblemClick}
          health={health}
        />

        {/* Central Model Editor & Right Results Analysis Console */}
        <main className="flex-1 min-h-0 min-w-0 flex flex-col lg:flex-row h-auto lg:h-full overflow-visible lg:overflow-hidden">
          {/* Model Formulation / Editor Column (7/12 width on desktop) */}
          <div className="w-full lg:w-[58.333%] h-[550px] lg:h-full min-h-0 min-w-0 flex flex-col overflow-hidden">
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
              onFileUpload={handleFileUpload}
              onFormatContent={handleFormatContent}
              onClearContent={handleClearContent}
              onSaveBenchmark={handleSaveCurrentModel}
            />
          </div>

          {/* Results, Diagnostics & Telemetry Column (5/12 width on desktop) */}
          <div className="w-full lg:w-[41.667%] min-h-[450px] lg:h-full min-h-0 min-w-0 bg-[#07090d] border-t lg:border-t-0 lg:border-l border-white/[0.08] flex flex-col overflow-hidden">
            {/* Header: Results Title & Log Drawer Trigger (Always Pinned at Top) */}
            <div className="p-3 bg-[#0a0e16] border-b border-white/[0.08] flex items-center justify-between shrink-0">
              <span className="text-xs font-semibold text-slate-300 tracking-tight">Optimization Results</span>
              <button
                onClick={() => setIsLogDrawerOpen(true)}
                className="px-2.5 py-1 text-xs bg-white/[0.04] hover:bg-white/[0.08] text-slate-300 font-mono rounded-[4px] flex items-center gap-1.5 border border-white/[0.08] transition-colors cursor-pointer"
                title="Open real-time solver log console"
              >
                <Terminal className="w-3.5 h-3.5 text-slate-400" />
                <span>Log Stream</span>
              </button>
            </div>

            {/* Results Body: Single Empty State vs Progressively Revealed Components */}
            {!solveResult && !validationInfo && !isSolving ? (
              <div className="flex-1 min-h-0 overflow-y-auto flex flex-col items-center justify-center p-8 text-center text-slate-500 font-sans space-y-3">
                <div className="w-10 h-10 rounded-full bg-white/[0.03] border border-white/[0.06] flex items-center justify-center">
                  <Crosshair className="w-4 h-4 text-slate-600" />
                </div>
                <div className="text-slate-300 font-medium text-sm tracking-tight">No solution yet</div>
                <p className="text-xs text-slate-500 max-w-xs leading-relaxed font-sans">
                  Select a model from the sidebar or define equations in the editor, then click{' '}
                  <span className="text-slate-200 font-medium">Solve Model</span>.
                </p>
              </div>
            ) : (
              <div className="flex-1 min-h-0 min-w-0 overflow-y-auto divide-y divide-white/[0.06]">
                {/* 1. Model Summary Card */}
                <ProblemSummaryCard
                  name={problemName}
                  type={inputMode}
                  telemetry={solveResult?.telemetry}
                  validationInfo={validationInfo}
                />

                {/* 2. Solver Phase Execution Timeline */}
                <PhaseTimeline
                  validationInfo={validationInfo}
                  result={solveResult}
                  isSolving={isSolving}
                  isValidating={isValidating}
                />

                {/* 3. Primary Solution Result & Metrics */}
                <ResultPanel result={solveResult} validationInfo={validationInfo} isSolving={isSolving} />

                {/* 4. Primal Solution Variable Table */}
                <SolutionTable
                  solution={solveResult?.solution}
                  telemetry={solveResult?.telemetry}
                  statusName={solveResult?.status_name}
                />

                {/* 5. Engine Diagnostics Panel */}
                <TelemetryPanel telemetry={solveResult?.telemetry} />

                {/* 6. MIP Convergence Trajectory Chart */}
                <MipProgressChart
                  history={solveResult?.progressHistory}
                  sense={solveResult?.telemetry?.sense}
                  objective={solveResult?.objective_value}
                  bestBound={solveResult?.best_bound}
                  gap={solveResult?.mip_gap}
                  isSolving={isSolving}
                />

                {/* 7. Branch and Bound Search Tree */}
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
