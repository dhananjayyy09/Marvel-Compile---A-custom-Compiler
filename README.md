# Marvels Compiler (Mukku Language)

A full-stack compiler project: React frontend, Flask backend, and a C++ compiler for the custom **Mukku** language (keywords: `val`, `prt`, `agar`, `nhi-to`, `bhejo`).

---

## Prerequisites

- **Python 3** (with `pip`)
- **Node.js** and **npm**
- **C++ compiler** (e.g. **g++** – MinGW on Windows, or GCC/clang on macOS/Linux)

---

## Steps to Run the Project

### 1. Clone or open the project

```bash
cd "c:\Users\ALG\Documents\vs code\pro\compilor\Compiler_Code"
```

(Or your actual project path.)

### 2. Build the C++ compiler

From the **project root** (where `main.cpp` is):

**Windows (PowerShell):**
```powershell
g++ main.cpp -o compiler
```
This creates `compiler.exe` on Windows.

**Linux / macOS:**
```bash
g++ main.cpp -o compiler
```
This creates `compiler` (no extension).

### 3. Install Python dependencies

From the **project root**:

```bash
pip install -r requirements.txt
```

Or manually:
```bash
pip install flask flask-cors
```

### 4. Install frontend dependencies

```bash
cd frontend
npm install
cd ..
```

### 5. Start the backend (Flask)

From the **project root** (so `compiler`/`compiler.exe` is in the same folder):

```bash
python server.py
```

You should see:
- `Running on http://127.0.0.1:5000`

Leave this terminal open.

### 6. Start the frontend (React)

In a **new terminal**, from the project root:

```bash
cd frontend
npm start
```

The app will open in the browser at **http://localhost:3000**.

---

## Quick reference

| Step | Command | Where |
|------|---------|--------|
| 1 | `g++ main.cpp -o compiler` | Project root |
| 2 | `pip install -r requirements.txt` | Project root |
| 3 | `cd frontend` then `npm install` | Project root → frontend |
| 4 | `python server.py` | Project root (keep running) |
| 5 | `cd frontend` then `npm start` | New terminal → frontend |

---

## Using the app

1. Open **http://localhost:3000** in your browser.
2. Write Mukku code in the left panel (e.g. `val x=2; val y=x+8; prt(y);`).
3. Click **Run Code**.
4. Output (and any errors) appear in the right panel.

---

## Project structure

```
Compiler_Code/
├── main.cpp          # C++ compiler (lexer, parser, AST, interpreter)
├── server.py         # Flask API that runs the compiler
├── requirements.txt  # Python dependencies
├── README.md         # This file
└── frontend/         # React app (editor + output)
    ├── package.json
    └── src/
        └── App.js    # Calls http://localhost:5000/compile
```

Backend runs on **port 5000**, frontend on **port 3000**. Both must be running for Run Code to work.
