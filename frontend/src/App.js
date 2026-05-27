import React, { useEffect, useState, useRef } from "react";
import { SplitPane } from "@rexxars/react-split-pane";
import Editor from "@monaco-editor/react";
import "./App.css";

//import ParseTree from "./ParseTree"; //  Parse tree visual component
//import { extractParseTreeFromOutput } from "./extractParseTree"; //  Extract JSON tree

function App() {
  const STORAGE_KEY = "mukku_editor_code_v1";
  const THEME_KEY = "mukku_theme_v1";
  const starterCode = `val x = 2;
val y = x + 8;
prt(y);`;
  const demoCode = `val a = 10;
val b = 20;
prt("Hello, Johny!");
prt(a + b);
agar (a < b) {
  prt("Hello buddy");
} nhi-to {
  prt("Cool man");
}`;
  const snippetTemplates = [
    {
      label: "Variable",
      code: "val total = 100;\nprt(total);",
    },
    {
      label: "If/Else",
      code: 'agar (x > 10) {\n  prt("x is greater");\n} nhi-to {\n  prt("x is smaller");\n}',
    },
    {
      label: "Return",
      code: "val score = 42;\nbhejo score;",
    },
  ];

  const [code, setCode] = useState(() => {
    const cached = localStorage.getItem(STORAGE_KEY);
    return cached ?? starterCode;
  });
  const [output, setOutput] = useState("");
  const [isError, setIsError] = useState(false);
  const [loading, setLoading] = useState(false);
  const [theme, setTheme] = useState(() => localStorage.getItem(THEME_KEY) || "dark");
  const [history, setHistory] = useState([]);
  const [showHistory, setShowHistory] = useState(false);
  const [errorLine, setErrorLine] = useState(null);
  //const [parseTree, setParseTree] = useState(null); //  Parse Tree state
  const outputRef = useRef(null);
  const monacoRef = useRef(null);
  const editorRef = useRef(null);
  const decorationsRef = useRef([]);

  const lineCount = code ? code.split("\n").length : 0;
  const charCount = code.length;

  useEffect(() => {
    localStorage.setItem(STORAGE_KEY, code);
  }, [code]);

  useEffect(() => {
    localStorage.setItem(THEME_KEY, theme);
  }, [theme]);

  useEffect(() => {
    if (!editorRef.current || !monacoRef.current) return;

    const decorations = [];
    if (errorLine && Number.isFinite(errorLine)) {
      decorations.push({
        range: new monacoRef.current.Range(errorLine, 1, errorLine, 1),
        options: {
          isWholeLine: true,
          className: "error-line-highlight",
          glyphMarginClassName: "error-line-glyph",
        },
      });
    }

    decorationsRef.current = editorRef.current.deltaDecorations(
      decorationsRef.current,
      decorations
    );
  }, [errorLine]);

  useEffect(() => {
    const onKeyDown = (event) => {
      if ((event.ctrlKey || event.metaKey) && event.key === "Enter") {
        event.preventDefault();
        if (!loading) {
          handleRun();
        }
      }
      if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "l") {
        event.preventDefault();
        handleClearOutput();
      }
    };

    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  }, [loading, code]);

  const handleRun = async () => {
    setLoading(true);
    setOutput("");
    setIsError(false);
    // setParseTree(null); //  Reset parse tree before each run

    try {
      const response = await fetch("http://localhost:5000/compile", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ code }),
      });
      const data = await response.json();
      setIsError(data.type === "error");
      setOutput(data.output || "No output");
      const parsedErrorLine = data.type === "error" ? extractErrorLine(data.output || "") : null;
      setErrorLine(parsedErrorLine);
      setHistory((prev) => [
        {
          id: Date.now(),
          status: data.type === "error" ? "error" : "success",
          at: new Date().toLocaleTimeString(),
          codePreview: code.split("\n")[0]?.slice(0, 40) || "Empty code",
          output: data.output || "No output",
        },
        ...prev,
      ].slice(0, 8));

      // const tree = extractParseTreeFromOutput(data.output); //  Get tree from output
      //setParseTree(tree);

      if (outputRef.current) {
        outputRef.current.scrollTop = outputRef.current.scrollHeight;
      }
    } catch (error) {
      setOutput("❌ Network Error: Could not reach compiler");
      setIsError(true);
      setErrorLine(null);
      setHistory((prev) => [
        {
          id: Date.now(),
          status: "error",
          at: new Date().toLocaleTimeString(),
          codePreview: code.split("\n")[0]?.slice(0, 40) || "Empty code",
          output: "❌ Network Error: Could not reach compiler",
        },
        ...prev,
      ].slice(0, 8));
    } finally {
      setLoading(false);
    }
  };

  const handleClearEditor = () => {
    setCode("");
  };

  const handleResetEditor = () => {
    setCode(starterCode);
  };

  const handleLoadSample = () => {
    setCode(demoCode);
  };

  const handleClearOutput = () => {
    setOutput("");
    setIsError(false);
    setErrorLine(null);
  };

  const handleCopyOutput = async () => {
    if (!output) return;
    try {
      await navigator.clipboard.writeText(output);
    } catch {
      // Clipboard can fail silently in some environments.
    }
  };

  const handleDownloadOutput = () => {
    if (!output) return;
    const blob = new Blob([output], { type: "text/plain;charset=utf-8" });
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement("a");
    anchor.href = url;
    anchor.download = "mukku-output.txt";
    anchor.click();
    URL.revokeObjectURL(url);
  };

  const handleInsertSnippet = (snippetCode) => {
    setCode((prev) => (prev.trim() ? `${prev}\n\n${snippetCode}` : snippetCode));
  };

  const handleRestoreHistory = (item) => {
    setOutput(item.output);
    setIsError(item.status === "error");
    setErrorLine(item.status === "error" ? extractErrorLine(item.output || "") : null);
  };

  const extractErrorLine = (text) => {
    const match = text.match(/line\s+(\d+)/i);
    if (!match) return null;
    const value = Number(match[1]);
    return Number.isFinite(value) ? value : null;
  };

  const handleEditorMount = (editor, monaco) => {
    editorRef.current = editor;
    monacoRef.current = monaco;
  };

  return (
    <div className="app" data-theme={theme}>
      <header className="topbar">
        <div>
          <h1 className="split-title">Marvels Compiler</h1>
          <p className="subtitle">
            Write Mukku code, compile instantly, and inspect full compiler output.
          </p>
        </div>
        <div className="stats">
          <button
            className="tool-btn history-toggle-btn"
            onClick={() => setShowHistory((prev) => !prev)}
          >
            History ({history.length})
          </button>
          <select
            className="theme-select"
            value={theme}
            onChange={(event) => setTheme(event.target.value)}
            title="Theme mode"
          >
            <option value="dark">Dark</option>
            <option value="light">Light</option>
            <option value="neon">Neon</option>
          </select>
          <span>{lineCount} lines</span>
          <span>{charCount} chars</span>
          <span className={isError ? "status error" : "status"}>
            {loading ? "Compiling..." : output ? (isError ? "Error" : "Success") : "Ready"}
          </span>
        </div>
      </header>

      {showHistory && (
        <section className="history-card top-history">
          <div className="history-header">
            <h3>Run History</h3>
            <div className="toolbar">
              <button
                className="tool-btn"
                onClick={() => setHistory([])}
                disabled={!history.length}
              >
                Clear History
              </button>
              <button
                className="tool-btn"
                onClick={() => setShowHistory(false)}
              >
                Close
              </button>
            </div>
          </div>
          {!history.length ? (
            <p className="history-empty">No runs yet. Compile once to create history.</p>
          ) : (
            <div className="history-list">
              {history.map((item) => (
                <button
                  key={item.id}
                  className={`history-item ${item.status}`}
                  onClick={() => handleRestoreHistory(item)}
                >
                  <span>{item.at}</span>
                  <span>{item.codePreview}</span>
                </button>
              ))}
            </div>
          )}
        </section>
      )}

      {/* Top section: Code and Output in split pane */}
      <div className="workspace">
        <SplitPane split="vertical" minSize={300} defaultSize="50%">
          {/* Code Editor */}
          <div className="pane editor-pane">
            <div className="pane-header-row">
              <div className="pane-header">Write your code</div>
              <div className="toolbar">
                {snippetTemplates.map((snippet) => (
                  <button
                    key={snippet.label}
                    className="tool-btn snippet-btn"
                    onClick={() => handleInsertSnippet(snippet.code)}
                  >
                    + {snippet.label}
                  </button>
                ))}
                <button className="tool-btn" onClick={handleLoadSample}>Sample</button>
                <button className="tool-btn" onClick={handleResetEditor}>Reset</button>
                <button className="tool-btn" onClick={handleClearEditor}>Clear</button>
              </div>
            </div>
            <div className="editor">
              <Editor
                height="100%"
                defaultLanguage="plaintext"
                value={code}
                onChange={(value) => setCode(value ?? "")}
                onMount={handleEditorMount}
                theme={theme === "light" ? "vs" : "vs-dark"}
                options={{
                  minimap: { enabled: false },
                  fontSize: 15,
                  lineNumbers: "on",
                  roundedSelection: true,
                  scrollBeyondLastLine: false,
                  automaticLayout: true,
                  tabSize: 2,
                  glyphMargin: true,
                }}
              />
            </div>
            <button
              className={`run-btn ${loading ? "loading" : ""}`}
              onClick={handleRun}
              disabled={loading}
            >
              {loading ? (
                <>
                  <span className="spinner"></span> Compiling...
                </>
              ) : (
                "Run Code (Ctrl+Enter)"
              )}
            </button>
            <div className="hint-text">
              Auto-saves your editor content locally.
              {errorLine ? ` Compiler error highlighted on line ${errorLine}.` : ""}
            </div>
          </div>

          {/* Output */}
          <div className="pane output-pane">
            <div className="pane-header-row">
              <div className="pane-header">Output</div>
              <div className="toolbar">
                <button className="tool-btn" onClick={handleDownloadOutput} disabled={!output}>Download</button>
                <button className="tool-btn" onClick={handleCopyOutput} disabled={!output}>Copy</button>
                <button className="tool-btn" onClick={handleClearOutput} disabled={!output}>Clear</button>
              </div>
            </div>
            <div
              ref={outputRef}
              className={`output-box ${isError ? "error" : "success"}`}
            >
              {output || (
                <span className="placeholder">Output will appear here...</span>
              )}
            </div>
            <div className="hint-text">Shortcut: Ctrl+L to clear output.</div>
          </div>
        </SplitPane>
      </div>

      {/* Bottom section: Parse Tree */}
      {/* {parseTree && (
      <div style={{ marginTop: "20px", padding: "0 20px 30px" }}>
     
      </div>
    )}*/}
    </div>
  );

}

export default App;