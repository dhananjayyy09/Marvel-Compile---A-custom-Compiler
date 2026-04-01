from flask import Flask, request, jsonify
from flask_cors import CORS
import subprocess
import os
import tempfile
import uuid

app = Flask(__name__)
CORS(app)

# Precompile the compiler at startup (Windows uses .exe)
COMPILER_EXE = "compiler.exe" if os.name == "nt" else "compiler"
if not os.path.exists(COMPILER_EXE):
    compile_result = subprocess.run(
        ["g++", "main.cpp", "-o", COMPILER_EXE],
        capture_output=True,
        text=True
    )
    if compile_result.returncode != 0:
        print(f"⚠ COMPILER COMPILATION FAILED:\n{compile_result.stderr}")

@app.route('/')
def home():
    return "Mukku Compiler Backend is running."

@app.route('/compile', methods=['POST'])
def compile_code():
    try:
        data = request.get_json()
        code = data.get("code", "").strip()
        
        if not code:
            return jsonify({"output": "❌ Error: Empty code submitted"}), 400
            
        # Create temp file
        temp_dir = tempfile.mkdtemp()
        input_path = os.path.join(temp_dir, f"input_{uuid.uuid4().hex}.mukku")
        
        with open(input_path, "w") as f:
            f.write(code)
        
        # Run compiler (Windows: no leading ./)
        run_cmd = [COMPILER_EXE, input_path] if os.name == "nt" else [f"./{COMPILER_EXE}", input_path]
        process = subprocess.run(
            run_cmd,
            capture_output=True,
            text=True,
            timeout=10  # Prevent infinite runs
        )
        
        # Cleanup temp directory
        os.remove(input_path)
        os.rmdir(temp_dir)
        
        if process.returncode != 0:
            return jsonify({
                "output": f"❌ Runtime Error (Code {process.returncode}):\n{process.stderr}",
                "type": "error"
            }), 400
            
        return jsonify({
            "output": process.stdout,
            "type": "success"
        })

    except subprocess.TimeoutExpired:
        return jsonify({"output": "❌ Timeout: Code took too long to execute", "type": "error"}), 400
    except Exception as e:
        return jsonify({"output": f"❌ Server Error: {str(e)}", "type": "error"}), 500

if __name__ == '__main__':
    app.run(debug=True)