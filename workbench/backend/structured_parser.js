const fs = require('fs');

/**
 * Parses Structured Model text into an MPS string.
 * Example syntax:
 * VARIABLES
 * x1 binary
 * x2 binary
 * x3 continuous
 * 
 * MINIMIZE
 * 9*x11 + 2*x12 + 7*x13
 * 
 * SUBJECT TO
 * c1: x11 + x12 + x13 = 1
 * c2: x11 + x21 <= 1
 */
function parseStructuredToMps(text, modelName = "structured_model") {
  if (!text || typeof text !== 'string') return "";

  const trimmed = text.trim();
  const upperTrimmed = trimmed.toUpperCase();
  // Auto-detect MPS format: if text contains MPS section markers or comments
  if (
    upperTrimmed.startsWith('NAME') ||
    upperTrimmed.startsWith('*') ||
    upperTrimmed.includes('\nROWS') ||
    upperTrimmed.includes('\r\nROWS') ||
    upperTrimmed.includes('\nCOLUMNS') ||
    upperTrimmed.includes('\r\nCOLUMNS') ||
    upperTrimmed.includes('\nENDATA') ||
    upperTrimmed.includes('\r\nENDATA')
  ) {
    return text;
  }

  const lines = text.split(/\r?\n/);
  let currentSection = "";

  const variables = new Map(); // name -> { type: 'binary'|'continuous'|'integer', lb: 0, ub: Infinity }
  let sense = "MINIMIZE";
  const objCoeffs = new Map(); // varName -> coeff
  const constraints = []; // { name, sense: 'E'|'L'|'G', rhs, coeffs: Map(varName -> coeff) }

  function parseExpression(exprStr) {
    const coeffs = new Map();
    // Normalize string: replace '-' with '+-'
    const normalized = exprStr.replace(/-/g, '+-');
    const terms = normalized.split('+');

    for (let term of terms) {
      term = term.trim();
      if (!term) continue;
      let coeff = 1.0;
      let varName = term;

      if (term.includes('*')) {
        const parts = term.split('*');
        coeff = parseFloat(parts[0].trim());
        varName = parts[1].trim();
      } else {
        const spaceMatch = term.match(/^([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s+([a-zA-Z_][a-zA-Z0-9_]*)$/);
        if (spaceMatch) {
          coeff = parseFloat(spaceMatch[1]);
          varName = spaceMatch[2];
        } else if (!isNaN(parseFloat(term)) && !/[a-zA-Z]/.test(term)) {
          // Just a constant term
          continue;
        } else if (term.startsWith('-')) {
          const rest = term.substring(1).trim();
          if (rest === '' || isNaN(parseFloat(rest))) {
            coeff = -1.0;
            varName = rest || varName;
          } else if (rest.includes('*')) {
            const parts = rest.split('*');
            coeff = -parseFloat(parts[0].trim());
            varName = parts[1].trim();
          }
        }
      }

      if (varName) {
        // Handle standalone negative var like -x1
        if (varName.startsWith('-')) {
          coeff = -coeff;
          varName = varName.substring(1).trim();
        }
        const prev = coeffs.get(varName) || 0.0;
        coeffs.set(varName, prev + coeff);
        if (!variables.has(varName)) {
          variables.set(varName, { type: 'continuous', lb: 0.0, ub: Infinity });
        }
      }
    }
    return coeffs;
  }

  for (let line of lines) {
    line = line.trim();
    if (!line || line.startsWith('#') || line.startsWith('//')) continue;

    const upper = line.toUpperCase();
    if (upper === 'VARIABLES' || upper === 'VARS') {
      currentSection = 'VARIABLES';
      continue;
    } else if (upper === 'MINIMIZE' || upper === 'MIN' || upper.startsWith('MINIMIZE ') || upper.startsWith('MIN ')) {
      currentSection = 'OBJECTIVE';
      sense = 'MINIMIZE';
      const firstSpace = line.indexOf(' ');
      if (firstSpace !== -1) {
        const rest = line.substring(firstSpace + 1).trim();
        if (rest) {
          const coeffs = parseExpression(rest);
          for (let [vName, cVal] of coeffs.entries()) {
            const prev = objCoeffs.get(vName) || 0.0;
            objCoeffs.set(vName, prev + cVal);
          }
        }
      }
      continue;
    } else if (upper === 'MAXIMIZE' || upper === 'MAX' || upper.startsWith('MAXIMIZE ') || upper.startsWith('MAX ')) {
      currentSection = 'OBJECTIVE';
      sense = 'MAXIMIZE';
      const firstSpace = line.indexOf(' ');
      if (firstSpace !== -1) {
        const rest = line.substring(firstSpace + 1).trim();
        if (rest) {
          const coeffs = parseExpression(rest);
          for (let [vName, cVal] of coeffs.entries()) {
            const prev = objCoeffs.get(vName) || 0.0;
            objCoeffs.set(vName, prev + cVal);
          }
        }
      }
      continue;
    } else if (upper === 'SUBJECT TO' || upper === 'CONSTRAINTS' || upper === 'ST') {
      currentSection = 'CONSTRAINTS';
      continue;
    } else if (upper === 'BOUNDS') {
      currentSection = 'BOUNDS';
      continue;
    }

    if (currentSection === 'VARIABLES') {
      const parts = line.split(/\s+/);
      const varName = parts[0];
      const varType = (parts[1] || 'continuous').toLowerCase();
      let lb = 0.0;
      let ub = (varType === 'binary') ? 1.0 : Infinity;
      variables.set(varName, { type: varType, lb, ub });
    } else if (currentSection === 'OBJECTIVE') {
      const coeffs = parseExpression(line);
      for (let [vName, cVal] of coeffs.entries()) {
        const prev = objCoeffs.get(vName) || 0.0;
        objCoeffs.set(vName, prev + cVal);
      }
    } else if (currentSection === 'CONSTRAINTS') {
      let name = `c${constraints.length + 1}`;
      let body = line;
      if (line.includes(':')) {
        const parts = line.split(':');
        name = parts[0].trim();
        body = parts[1].trim();
      }

      let cSense = 'E';
      let left = body;
      let right = '0';

      if (body.includes('<=')) {
        cSense = 'L';
        const p = body.split('<=');
        left = p[0].trim();
        right = p[1].trim();
      } else if (body.includes('>=')) {
        cSense = 'G';
        const p = body.split('>=');
        left = p[0].trim();
        right = p[1].trim();
      } else if (body.includes('=')) {
        cSense = 'E';
        const p = body.split('=');
        left = p[0].trim();
        right = p[1].trim();
      }

      const rhs = parseFloat(right);
      const coeffs = parseExpression(left);
      constraints.push({ name, sense: cSense, rhs, coeffs });
    } else if (currentSection === 'BOUNDS') {
      // e.g. x1 >= 0, x2 <= 10
      let vName = "";
      if (line.includes('>=')) {
        const p = line.split('>=');
        vName = p[0].trim();
        const val = parseFloat(p[1].trim());
        if (variables.has(vName)) variables.get(vName).lb = val;
      } else if (line.includes('<=')) {
        const p = line.split('<=');
        vName = p[0].trim();
        const val = parseFloat(p[1].trim());
        if (variables.has(vName)) variables.get(vName).ub = val;
      }
    }
  }

  // Build MPS Output String
  if (variables.size === 0) {
    throw new Error("Structured model parser produced 0 variables. Please verify model syntax or select 'MPS File Upload' mode.");
  }

  const mps = [];
  mps.push(`NAME          ${modelName}`);
  mps.push("OBJSENSE");
  mps.push(`  ${sense === 'MAXIMIZE' ? 'MAX' : 'MIN'}`);
  mps.push("ROWS");
  mps.push(" N  obj");
  for (let c of constraints) {
    mps.push(` ${c.sense}  ${c.name}`);
  }

  mps.push("COLUMNS");

  // Separate binary/integer and continuous vars
  const binVars = [];
  const contVars = [];
  for (let [vName, info] of variables.entries()) {
    if (info.type === 'binary' || info.type === 'integer') {
      binVars.push(vName);
    } else {
      contVars.push(vName);
    }
  }

  if (binVars.length > 0) {
    mps.push("    MARK0000  'MARKER'                 'INTORG'");
    for (let vName of binVars) {
      const objC = objCoeffs.get(vName) || 0.0;
      mps.push(`    ${vName.padEnd(10)} obj       ${objC.toString().padEnd(10)}`);
      for (let c of constraints) {
        if (c.coeffs.has(vName)) {
          const val = c.coeffs.get(vName);
          mps.push(`    ${vName.padEnd(10)} ${c.name.padEnd(10)} ${val.toString().padEnd(10)}`);
        }
      }
    }
    mps.push("    MARK0001  'MARKER'                 'INTEND'");
  }

  for (let vName of contVars) {
    const objC = objCoeffs.get(vName) || 0.0;
    mps.push(`    ${vName.padEnd(10)} obj       ${objC.toString().padEnd(10)}`);
    for (let c of constraints) {
      if (c.coeffs.has(vName)) {
        const val = c.coeffs.get(vName);
        mps.push(`    ${vName.padEnd(10)} ${c.name.padEnd(10)} ${val.toString().padEnd(10)}`);
      }
    }
  }

  mps.push("RHS");
  for (let c of constraints) {
    mps.push(`    rhs       ${c.name.padEnd(10)} ${c.rhs.toString().padEnd(10)}`);
  }

  mps.push("BOUNDS");
  for (let [vName, info] of variables.entries()) {
    if (info.type === 'binary') {
      mps.push(` BV bnd       ${vName}`);
    } else if (info.lb !== 0.0) {
      mps.push(` LO bnd       ${vName.padEnd(10)} ${info.lb}`);
    }
    if (info.type !== 'binary' && info.ub !== Infinity) {
      mps.push(` UP bnd       ${vName.padEnd(10)} ${info.ub}`);
    }
  }

  mps.push("ENDATA");

  return mps.join("\n");
}

module.exports = { parseStructuredToMps };
