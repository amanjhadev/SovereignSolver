const express = require('express');
const cors = require('cors');
const multer = require('multer');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');
const zlib = require('zlib');
const { parseStructuredToMps } = require('./structured_parser');

const app = express();
const PORT = process.env.PORT || 3001;
const HOST = process.env.HOST || '0.0.0.0';

app.use(cors());
app.use(express.json({ limit: '50mb' }));
app.use(express.urlencoded({ extended: true, limit: '50mb' }));

const UPLOAD_DIR = path.join(__dirname, 'temp_uploads');
if (!fs.existsSync(UPLOAD_DIR)) fs.mkdirSync(UPLOAD_DIR, { recursive: true });
const upload = multer({ dest: UPLOAD_DIR });

const TEMP_DIR = path.join(__dirname, 'temp_models');
if (!fs.existsSync(TEMP_DIR)) fs.mkdirSync(TEMP_DIR, { recursive: true });

// Dynamic resolution of C++ Sovereign Solver CLI binary path
function resolveSolverPath() {
  if (process.env.SOVEREIGN_SOLVER_PATH) {
    const envPath = path.resolve(process.env.SOVEREIGN_SOLVER_PATH);
    if (fs.existsSync(envPath)) return envPath;
  }

  const candidates = [
    // Primary CLI target: workbench_cli
    path.resolve(__dirname, '../../build/Release/workbench_cli.exe'),
    path.resolve(__dirname, '../../build/workbench_cli.exe'),
    path.resolve(__dirname, '../../build/Release/workbench_cli'),
    path.resolve(__dirname, '../../build/workbench_cli'),
    path.resolve(__dirname, '../bin/workbench_cli'),
    path.resolve(__dirname, '../../bin/workbench_cli'),
    '/app/bin/workbench_cli',
    // Secondary CLI target: sovereign_cli
    path.resolve(__dirname, '../../build/Release/sovereign_cli.exe'),
    path.resolve(__dirname, '../../build/sovereign_cli.exe'),
    path.resolve(__dirname, '../../build/Release/sovereign_cli'),
    path.resolve(__dirname, '../../build/sovereign_cli'),
    path.resolve(__dirname, '../bin/sovereign_cli'),
    path.resolve(__dirname, '../../bin/sovereign_cli'),
    '/app/bin/sovereign_cli'
  ];

  for (const candidate of candidates) {
    if (fs.existsSync(candidate)) return candidate;
  }

  // Fallback to environment variable or standard build location
  return process.env.SOVEREIGN_SOLVER_PATH
    ? path.resolve(process.env.SOVEREIGN_SOLVER_PATH)
    : path.resolve(__dirname, '../../build/Release/workbench_cli.exe');
}

function getSolverPath() {
  return resolveSolverPath();
}

// Track active solve processes for "Stop Solve" functionality
const activeProcesses = new Map(); // id -> childProcess

// Helper to write MPS content to temp file
function createTempMpsFile(type, content, modelName = "model") {
  const fileId = `model_${Date.now()}_${Math.floor(Math.random() * 1000)}`;
  const filePath = path.join(TEMP_DIR, `${fileId}.mps`);

  let mpsContent = content;
  if (type === 'structured') {
    mpsContent = parseStructuredToMps(content, modelName);
  }

  fs.writeFileSync(filePath, mpsContent, 'utf8');
  return filePath;
}

// API: Health Check
app.get('/api/health', (req, res) => {
  const solverPath = getSolverPath();
  const cliExists = fs.existsSync(solverPath);
  return res.json({
    status: 'ok',
    version: '0.2.0',
    solver_executable: solverPath,
    solver_path: solverPath,
    solver_exists: cliExists,
    solver_ready: cliExists,
    solver_version: '0.2.0'
  });
});

// API: Validate Model
app.post('/api/validate', (req, res) => {
  try {
    const { type, content, modelName } = req.body;
    if (!content) {
      return res.status(400).json({ success: false, error: 'Model content is required' });
    }

    const solverPath = getSolverPath();
    if (!fs.existsSync(solverPath)) {
      return res.status(500).json({ success: false, error: `Solver binary not found at ${solverPath}` });
    }

    const tempFile = createTempMpsFile(type, content, modelName || 'validate_model');

    const cli = spawn(solverPath, ['--mps', tempFile, '--validate-only']);
    let stdout = '';
    let stderr = '';

    cli.stdout.on('data', data => stdout += data.toString());
    cli.stderr.on('data', data => stderr += data.toString());

    cli.on('close', code => {
      // Clean up temp file
      try { fs.unlinkSync(tempFile); } catch (e) {}

      if (code !== 0 && !stdout) {
        return res.json({ success: false, error: stderr || `Validation process exited with code ${code}` });
      }

      try {
        const result = JSON.parse(stdout);
        return res.json(result);
      } catch (err) {
        return res.json({ success: false, error: `Invalid JSON from CLI: ${stdout}` });
      }
    });
  } catch (err) {
    return res.status(500).json({ success: false, error: err.message });
  }
});

// API: Solve Model
app.post('/api/solve', (req, res) => {
  try {
    const { type, content, modelName, timeout_sec = 30, threads = 1 } = req.body;
    if (!content) {
      return res.status(400).json({ success: false, error: 'Model content is required' });
    }

    const solverPath = getSolverPath();
    if (!fs.existsSync(solverPath)) {
      return res.status(500).json({ success: false, error: `Solver binary not found at ${solverPath}` });
    }

    const isSse = req.headers.accept === 'text/event-stream' || req.query.stream === 'true';
    const solveId = `solve_${Date.now()}_${Math.floor(Math.random() * 1000)}`;
    const tempFile = createTempMpsFile(type, content, modelName || 'solve_model');

    const args = ['--mps', tempFile, '--timeout', timeout_sec.toString(), '--threads', threads.toString(), '--stream-telemetry'];
    const cli = spawn(solverPath, args);

    activeProcesses.set(solveId, { cli, tempFile });

    if (isSse) {
      res.setHeader('Content-Type', 'text/event-stream');
      res.setHeader('Cache-Control', 'no-cache');
      res.setHeader('Connection', 'keep-alive');
      res.flushHeaders();
    }

    let lineBuffer = '';
    let finalStdout = '';
    let stderr = '';

    cli.stdout.on('data', data => {
      const str = data.toString();
      lineBuffer += str;
      const lines = lineBuffer.split('\n');
      lineBuffer = lines.pop(); // keep last incomplete chunk

      for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed) continue;
        if (trimmed.startsWith('{"event":')) {
          if (isSse) {
            res.write(`data: ${trimmed}\n\n`);
          }
        } else {
          finalStdout += trimmed + '\n';
        }
      }
    });

    cli.stderr.on('data', data => stderr += data.toString());

    cli.on('close', (code, signal) => {
      activeProcesses.delete(solveId);

      // Clean up temp file
      try { fs.unlinkSync(tempFile); } catch (e) {}

      // Flush remaining line buffer
      if (lineBuffer.trim()) {
        const trimmed = lineBuffer.trim();
        if (trimmed.startsWith('{"event":')) {
          if (isSse) res.write(`data: ${trimmed}\n\n`);
        } else {
          finalStdout += trimmed + '\n';
        }
      }

      let resultObj = null;
      if (signal === 'SIGTERM' || signal === 'SIGKILL') {
        resultObj = {
          success: false,
          status_name: 'UserInterrupted',
          message: 'Solve process was stopped by user request'
        };
      } else if (code !== 0 && !finalStdout) {
        resultObj = {
          success: false,
          status_name: 'NumericalFailure',
          message: stderr || `CLI process exited with code ${code}`
        };
      } else {
        try {
          resultObj = JSON.parse(finalStdout);
          resultObj.solveId = solveId;
        } catch (err) {
          resultObj = {
            success: false,
            status_name: 'ParserError',
            message: `CLI output parsing failed: ${finalStdout}`
          };
        }
      }

      if (isSse) {
        res.write(`data: [FINAL_RESULT] ${JSON.stringify(resultObj)}\n\n`);
        res.end();
      } else {
        return res.json(resultObj);
      }
    });

    // Handle client disconnect safely
    req.on('close', () => {
      if (isSse && !res.writableEnded) {
        std_out_log(`Client closed SSE connection for ${solveId}`);
      }
    });
  } catch (err) {
    return res.status(500).json({ success: false, error: err.message });
  }
});

// API: Stop Solve
app.post('/api/stop-solve', (req, res) => {
  const { solveId } = req.body;
  if (solveId && activeProcesses.has(solveId)) {
    const { cli } = activeProcesses.get(solveId);
    cli.kill('SIGTERM');
    activeProcesses.delete(solveId);
    return res.json({ success: true, message: 'Solve process terminated' });
  }
  return res.json({ success: false, message: 'No active solve process found for ID' });
});

// API: Upload MPS File (.mps or .mps.gz)
app.post('/api/upload-mps', upload.single('file'), (req, res) => {
  try {
    if (!req.file) {
      return res.status(400).json({ success: false, error: 'No file uploaded' });
    }

    const filePath = req.file.path;
    const isGz = req.file.originalname.endsWith('.gz');

    let mpsText = '';
    if (isGz) {
      const buffer = fs.readFileSync(filePath);
      mpsText = zlib.gunzipSync(buffer).toString('utf8');
    } else {
      mpsText = fs.readFileSync(filePath, 'utf8');
    }

    // Clean up upload temp file
    try { fs.unlinkSync(filePath); } catch (e) {}

    return res.json({
      success: true,
      filename: req.file.originalname,
      content: mpsText
    });
  } catch (err) {
    return res.status(500).json({ success: false, error: `File processing failed: ${err.message}` });
  }
});

app.listen(PORT, HOST, () => {
  std_out_log(`Sovereign Workbench Backend API listening on http://${HOST}:${PORT}`);
});

function std_out_log(msg) {
  console.log(`[SOVEREIGN BACKEND] ${msg}`);
}
