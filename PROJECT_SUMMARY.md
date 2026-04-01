# Marvels Compiler — Complete Project Summary

This document explains **everything** about the project so that anyone can understand what it is, how it works, and how to use or extend it.

---

## What This Project Does (Short Summary)

**Marvels Compiler** is a web-based **compiler and runner** for a small custom language called **Mukku**. You type Mukku code in a browser, click Run, and see the result—plus all the internal steps a real compiler goes through.

**In one sentence:** The project lets you write and run programs in Mukku (variables, math, if/else, print) in a browser; a React frontend sends your code to a Flask server, which runs a C++ compiler that tokenizes, parses, checks, generates intermediate and assembly code, then executes the program and sends the full output back to the browser.

**Why it exists:** It demonstrates a full compiler pipeline (lexer → parser → semantic analysis → intermediate code → assembly) and a tree-walk interpreter in one place, with a simple web UI so anyone can try the language without installing the C++ toolchain. It’s built for learning and teaching how compilers and runtimes work.

---

## 1. What Is This Project?

**Marvels Compiler** is a **full-stack educational compiler** for a custom programming language called **Mukku**. The project has three parts:

| Layer | Technology | Role |
|-------|------------|------|
| **Frontend** | React (Create React App) | Web UI: code editor + output panel; sends code to the backend and shows results. |
| **Backend** | Flask (Python) | HTTP API that receives code, runs the C++ compiler on it, and returns the compiler’s output. |
| **Compiler** | C++ (single executable) | Reads a source file, runs the full compiler pipeline (lexer → parser → semantic analysis → intermediate code → assembly), then executes the program via a built-in interpreter and prints all phases + result. |

The **Mukku** language uses Hindi-inspired keywords (`val`, `prt`, `agar`, `nhi-to`, `bhejo`) and supports variables, arithmetic, comparisons, conditionals, print, and return. The C++ compiler both **compiles** (token list, AST, three-address code, assembly) and **runs** the program by interpreting the AST.

---

## 2. The Mukku Language — Full Reference

### 2.1 Keywords (Reserved Words)

| Keyword | Meaning | Example |
|---------|---------|--------|
| `val` | Declare a variable (and optionally assign) | `val x = 5;` |
| `prt` | Print (like `print`) | `prt(x);` or `prt("Hello");` |
| `agar` | If | `agar (x > 0) { ... }` |
| `nhi-to` | Else | `nhi-to { ... }` |
| `bhejo` | Return (sends a value back; compiler currently prints it) | `bhejo x;` |

These words **cannot** be used as variable names.

### 2.2 Syntax Rules

- **Statements** end with `;`.
- **Variables** are declared with `val name = expression;` or `val name;` (implicit 0). No re-declaration; variables are identified by name in a symbol table.
- **Identifiers**: Start with a letter or `_`, then letters, digits, or `_`. Must not be a keyword.
- **Numbers**: Non-negative integers (e.g. `0`, `42`).
- **Strings**: Double-quoted, e.g. `"Hello"`. Used only inside `prt(...)`.
- **Blocks**: `{` … `}` for `agar` and `nhi-to` bodies.
- **Condition**: Expression in parentheses after `agar`, e.g. `agar (a < b) { ... }`.

### 2.3 Operators

**Arithmetic** (binary): `+`, `-`, `*`, `/`  
**Comparison**: `==`, `!=`, `<`, `>`, `<=`, `>=`  
**Assignment**: `=` (only in `val x = expr;`)

**Precedence** (low to high):

1. Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
2. Additive: `+`, `-`
3. Multiplicative: `*`, `/`

Expression parsing is precedence-climbing (binary expressions only; no unary minus in the grammar).

### 2.4 Example Programs

**Simple variables and print:**
```text
val x = 2;
val y = x + 8;
prt(y);
```
Output: `10`

**If-else and strings:**
```text
val a = 10;
val b = 20;
prt("Hello, Johny!");
prt(a + b);
agar (a < b) {
    prt("Hello buddy");
} nhi-to {
    prt("Cool man");
}
```

**Return:**
```text
val n = 100;
bhejo n * 2;
```
Compiler prints something like: `Return: 200`

### 2.5 What the Compiler Does Not Support (Current Design)

- Loops (`while` / `for`)
- Functions (only top-level statements; `bhejo` is parsed but “return” is simulated by printing)
- Floating-point (integers only)
- Comments
- Multiple files

---

## 3. High-Level Architecture

```text
┌─────────────────────────────────────────────────────────────────┐
│  BROWSER (User)                                                 │
│  http://localhost:3000                                          │
└────────────────────────────┬────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  FRONTEND (React)                                               │
│  - Code editor (textarea)                                        │
│  - "Run Code" button                                             │
│  - Output panel (success / error styling)                        │
│  - POST body: { "code": "<user Mukku source>" }                  │
└────────────────────────────┬────────────────────────────────────┘
                              │  POST http://localhost:5000/compile
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  BACKEND (Flask, Python)                                         │
│  - Receives JSON { code }                                        │
│  - Writes code to a temp .mukku file                             │
│  - Runs: compiler.exe <path>  (or ./compiler on Unix)           │
│  - Captures stdout + stderr, return code                         │
│  - Returns JSON: { "output": "...", "type": "success"|"error" }  │
└────────────────────────────┬────────────────────────────────────┘
                              │  subprocess
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  COMPILER (C++ executable)                                       │
│  - Reads source file path as argv[1]                             │
│  - Runs full pipeline (see Section 4)                            │
│  - All phase outputs + final program output go to stdout        │
│  - Errors / invalid input → stderr, non-zero exit code           │
└─────────────────────────────────────────────────────────────────┘
```

- **Frontend** and **backend** are separate processes (React dev server on 3000, Flask on 5000).
- The **compiler** is a single C++ program; the backend runs it as a subprocess and streams its output back to the client.

---

## 4. The C++ Compiler Pipeline (main.cpp)

The compiler in `main.cpp` implements a classic multi-phase pipeline. Execution order:

### Phase 1: Lexical Analysis (Tokenization)

- **Input**: Raw source code string.
- **Process**: Regex-based scanner; longest match; tokens have type, value, line, column.
- **Output**: List of tokens printed (for debugging/teaching). Token types include:
  - **Keywords**: VAL, PRT, AGAR, NHI_TO, BHEJO  
  - **Identifiers**: ID  
  - **Literals**: NUMBER, STRING  
  - **Operators**: OP (`+`, `-`, `*`, `/`), COMPARE (`==`, `!=`, `<`, `>`, `<=`, `>=`), ASSIGN (`=`)  
  - **Punctuation**: LPAREN, RPAREN, LBRACE, RBRACE, SEMI  
  - **END** (one past last token)
- **Errors**: Illegal characters; reported with line/column, compilation stops after printing errors.

### Phase 2: Syntax Analysis (Parsing)

- **Input**: Token stream.
- **Process**: Recursive-descent parser; builds an **Abstract Syntax Tree (AST)**. Grammar (conceptually):
  - Program → Statement*
  - Statement → Declaration | Print | IfElse | Return
  - Declaration → `val` ID [`=` Expression] `;`
  - Print → `prt` `(` (Expression | StringLiteral) `)` `;`
  - IfElse → `agar` `(` Expression `)` `{` Statement* `}` [`nhi-to` `{` Statement* `}`]
  - Return → `bhejo` Expression `;`
  - Expression → binary expressions with precedence (Primary plus OP/COMPARE)
  - Primary → ID | NUMBER
- **Output**: AST printed as **JSON** (type, value, children) for visualization/debugging.
- **Errors**: Unexpected tokens, missing `;` or `)`, etc.; collected and printed, then stop.

### Phase 3: Semantic Analysis

- **Input**: AST.
- **Process**: Single pass over the tree:
  - On **Declaration**: add variable to symbol table; error if already declared.
  - On **Identifier**: error if not in symbol table (use before declaration).
  - Recurses into children for other nodes.
- **Output**: Symbol table printed (name → kind, e.g. "variable").
- **Errors**: Duplicate declaration, undeclared variable; compilation stops after printing.

### Phase 4: Intermediate Code Generation

- **Input**: AST.
- **Process**: Tree walk that emits **three-address style** instructions:
  - Assignments: `x = expr` or `T1 = a + b`
  - `print` for `prt`
  - `return` for `bhejo`
  - Conditional: `ifnot cond goto L1`, `goto L2`, `L1:`, else block, `L2:`
- **Output**: List of intermediate instructions printed (numbered).
- **Temporaries**: T1, T2, …; labels L1, L2, … for if/else.

### Phase 5: Assembly Code Generation

- **Input**: AST again (separate tree walk).
- **Process**: Generates a simple **register-based assembly** (conceptual x86-style):
  - Registers: eax, ebx, ecx, edx (rotated by index).
  - `mov`, `add`, `sub`, `imul`, `idiv`/`cdq` for arithmetic.
  - Variables and literals loaded into registers; results stored back for declarations.
  - Return puts value in eax and emits `ret`.
- **Output**: Assembly lines printed (numbered). This is for teaching; the program is **not** actually assembled or run as native code.

### Phase 6: Execution (Interpreter)

- **Input**: AST.
- **Process**: **Tree-walk interpreter** over the same AST:
  - **Program**: execute each child statement in order.
  - **Declaration**: evaluate RHS (expression), store in a map (variable name → integer).
  - **Print**: if child is StringLiteral, print string (without quotes); else evaluate child and print integer.
  - **IfElse**: evaluate condition (integer; non-zero = true), execute then-branch or else-branch.
  - **Block**: execute each child statement.
  - **Return**: evaluate expression and print `Return: <value>` (no real function call).
- **Expression evaluation**: NumberLiteral → integer; Identifier → lookup in map; BinaryExpr → arithmetic or comparison (result 0/1).
- **Output**: All program output (and “Return: …”) is printed to **stdout**, which the Flask server captures and sends to the frontend.

So: the same AST is used for intermediate code, assembly, and execution. The “real” execution is the interpreter; assembly and three-address code are for demonstration.

---

## 5. Important Data Structures (C++)

### Tokens

- `TokenType`: enum (VAL, PRT, AGAR, NHI_TO, BHEJO, ID, NUMBER, STRING, OP, COMPARE, ASSIGN, LPAREN, RPAREN, LBRACE, RBRACE, SEMI, END).
- `Token`: type, value (string), line, column.

### AST Nodes

- **Node**: `nodeType` (string), `value` (string), `children` (list of AST nodes).
- **Types**: Program, Declaration, Print, IfElse, Block, Return, BinaryExpr, Identifier, NumberLiteral, StringLiteral.
- Methods: `print()`, `printJSON()`, `generateIntermediateCode()`, `generateAssembly()`. Interpreter is a separate class that walks the AST.

### Symbol Table

- `map<string, string>`: variable name → kind (e.g. `"variable"`). Used only in semantic analysis and for error messages.

---

## 6. Backend (Flask) — server.py

- **Framework**: Flask with CORS enabled so the React app (different port) can call the API.
- **Compiler binary**: `compiler.exe` on Windows (`os.name == "nt"`), `compiler` on Unix. If the binary is missing, the server tries to build it with `g++ main.cpp -o <COMPILER_EXE>` at startup.
- **Routes**:
  - `GET /`: Returns a short “Mukku Compiler Backend is running.” message.
  - `POST /compile`: Body `{ "code": "<Mukku source>" }`.
    1. Validates that `code` is non-empty (else 400).
    2. Creates a temporary directory and a unique file (e.g. `input_<uuid>.mukku`) with the user code.
    3. Runs the compiler executable with that file path (Windows: `compiler.exe path`; Unix: `./compiler path`). Current working directory is the project root so the executable is found.
    4. Timeout: 10 seconds to avoid infinite loops.
    5. Deletes the temp file and directory.
    6. If return code ≠ 0: returns JSON `{ "output": "<stderr>", "type": "error" }` with 400.
    7. Else: returns JSON `{ "output": "<stdout>", "type": "success" }` with 200.
- **Errors**: Timeout, empty code, or any exception → JSON with `"type": "error"` and appropriate message; 500 for server errors.
- **Port**: Default Flask port 5000 (`app.run(debug=True)`).

---

## 7. Frontend (React) — frontend/src

- **App.js**:
  - State: `code` (editor content; default sample: `val x=2; val y=x+8; prt(y);`), `output`, `isError`, `loading`.
  - **Run Code**: POST `http://localhost:5000/compile` with `JSON.stringify({ code })`. On response: set `output` to `data.output`, `isError` to `data.type === "error"`. On network failure: show “Could not reach compiler” and set error state.
  - UI: Title “Marvels Compiler”, split pane (code editor left, output right), Run button with loading spinner. Output box is styled green for success, red for errors; uses `pre-wrap` so compiler output is readable.
- **App.css**: Theming (gradients, colors, borders), split pane, editor and output box, button and spinner, scrollbars. Responsive for smaller screens.
- **Dependencies**: `react`, `react-dom`, `react-scripts`, `@rexxars/react-split-pane` for the resizable split. No backend URL configuration (hardcoded localhost:5000).

The frontend does **not** parse the compiler output (e.g. no separate parse-tree or AST viewer in the current code; some components are commented out).

---

## 8. Project File Structure

```text
Compiler_Code/
├── main.cpp              # Full compiler: lexer, parser, AST, semantic analysis,
│                         # intermediate code, assembly, interpreter
├── server.py             # Flask app: / and POST /compile, runs compiler.exe
├── requirements.txt     # flask, flask-cors
├── README.md             # Short guide + steps to run
├── PROJECT_SUMMARY.md    # This file
├── compiler.exe          # Built from main.cpp (Windows; name may be compiler on Unix)
├── AJ.anshu              # Sample Mukku program (optional; backend uses .mukku temp files)
└── frontend/
    ├── package.json      # React app deps and scripts
    ├── public/
    │   └── index.html
    └── src/
        ├── index.js      # React root
        ├── App.js        # Main UI and compile API call
        ├── App.css       # Styles
        └── ...           # Other CRA files (tests, etc.)
```

---

## 9. How to Run the Project (Short)

1. **Prerequisites**: Python 3, Node/npm, g++ (or compatible C++ compiler).
2. **Build compiler**: From project root run `g++ main.cpp -o compiler` (produces `compiler.exe` on Windows).
3. **Python**: `pip install -r requirements.txt`.
4. **Frontend**: `cd frontend`, `npm install`, then `npm start` (runs on port 3000).
5. **Backend**: From project root run `python server.py` (port 5000). Keep it running.
6. **Use**: Open http://localhost:3000, type or paste Mukku code, click **Run Code**. Full compiler output (all phases + program result) appears in the output panel.

---

## 10. Error Handling Summary

- **Lexer**: Illegal character → message with line/column; stop.
- **Parser**: Missing `;`, `)`, `}`, wrong keyword, or invalid expression → collected and printed; stop.
- **Semantic**: Duplicate variable declaration or use of undeclared variable → printed; stop.
- **Backend**: Empty code → 400. Compiler non-zero exit → 400 with stderr. Timeout or exception → 400/500 with message.
- **Frontend**: Network error → “Could not reach compiler”; response `type === "error"` → red box and show server output.

---

## 11. Quick Reference: Keywords and Grammar

- **Keywords**: `val`, `prt`, `agar`, `nhi-to`, `bhejo`.
- **Grammar sketch**:
  - Program = Statement*
  - Statement = `val` id [`=` Expr] `;` | `prt` `(` Expr | string `)` `;` | `agar` `(` Expr `)` `{` Statement* `}` [`nhi-to` `{` Statement* `}`] | `bhejo` Expr `;`
  - Expr = Primary (OP | COMPARE Expr)* with precedence.
- **Operators**: `+ - * /`; `== != < > <= >=`; `=` (in declarations only).

---

## 12. Who Is This For?

- **Students**: See a full pipeline (lexer → parser → semantics → IR → assembly) and a tree-walk interpreter in one codebase.
- **Developers**: Understand how the frontend, backend, and compiler interact; extend the language or add features (e.g. loops, functions, parse-tree viewer).
- **Instructors**: Use as a single project that ties together languages, compilers, and web APIs.

This document, together with the code and README, should be enough for anyone to understand and run the **Marvels Compiler** project end to end.
