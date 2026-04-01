#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cctype>
#include <regex>
#include <set>

using namespace std;

// Token types
enum class TokenType {
    VAL, PRT, AGAR, NHI_TO, BHEJO,
    ID, NUMBER, STRING, 
    OP, COMPARE, ASSIGN,
    LPAREN, RPAREN, LBRACE, RBRACE, SEMI,
    END
};

// Token structure
struct Token {
    TokenType type;
    string value;
    int line;
    int column;

    Token(TokenType t, string v, int l, int c) 
        : type(t), value(v), line(l), column(c) {}
};

// AST Node structure
struct ASTNode {
    string nodeType;
    vector<shared_ptr<ASTNode>> children;
    string value;
    int indentLevel = 0;

    ASTNode(string type, string val = "") : nodeType(type), value(val) {}

    void addChild(shared_ptr<ASTNode> child) {
        child->indentLevel = indentLevel + 1;
        children.push_back(child);
    }

    void print() const {
        cout << string(indentLevel * 2, ' ') << "└─ " << nodeType;
        if (!value.empty()) cout << " (" << value << ")";
        cout << endl;
        for (const auto& child : children) {
            child->print();
        }
    }

    // JSON output for visualization
    void printJSON(ostream& out, int indent = 0) const {
        string ind(indent, ' ');
        out << ind << "{\n";
        out << ind << "  \"type\": \"" << nodeType << "\"";
        if (!value.empty()) out << ",\n" << ind << "  \"value\": \"" << value << "\"";
        if (!children.empty()) {
            out << ",\n" << ind << "  \"children\": [\n";
            for (size_t i = 0; i < children.size(); ++i) {
                children[i]->printJSON(out, indent + 4);
                if (i + 1 < children.size()) out << ",\n";
            }
            out << "\n" << ind << "  ]";
        }
        out << "\n" << ind << "}";
    }

    string generateIntermediateCode(vector<string>& code, int& tempCount) {
        if (nodeType == "Program") {
            for (const auto& child : children) {
                child->generateIntermediateCode(code, tempCount);
            }
        }
        else if (nodeType == "Declaration") {
            if (!children.empty()) {
                string temp = children[0]->generateIntermediateCode(code, tempCount);
                code.push_back(value + " = " + temp);
            }
        }
        else if (nodeType == "BinaryExpr") {
            string leftTemp = children[0]->generateIntermediateCode(code, tempCount);
            string rightTemp = children[1]->generateIntermediateCode(code, tempCount);
            string resultTemp = "T" + to_string(++tempCount);
            code.push_back(resultTemp + " = " + leftTemp + " " + value + " " + rightTemp);
            return resultTemp;
        }
        else if (nodeType == "Identifier") {
            return value;
        }
        else if (nodeType == "NumberLiteral") {
            return value;
        }
        else if (nodeType == "Return") {
            if (!children.empty()) {
                string retVal = children[0]->generateIntermediateCode(code, tempCount);
                code.push_back("return " + retVal);
            }
        }
        else if (nodeType == "Print") {
            if (!children.empty()) {
                string val = children[0]->generateIntermediateCode(code, tempCount);
                code.push_back("print " + val);
            }
        }
        else if (nodeType == "IfElse") {
            string cond = children[0]->generateIntermediateCode(code, tempCount);
            string labelElse = "L" + to_string(++tempCount);
            string labelEnd = "L" + to_string(++tempCount);
        
            code.push_back("ifnot " + cond + " goto " + labelElse);
            children[1]->generateIntermediateCode(code, tempCount);
            code.push_back("goto " + labelEnd);
            code.push_back(labelElse + ":");
            if (children.size() > 2) {
                children[2]->generateIntermediateCode(code, tempCount);
            }
            code.push_back(labelEnd + ":");
        }
        
        return "";
    }

    // Assembly code generation using registers
    string getRegister(int idx) const {
        static vector<string> regs = {"eax", "ebx", "ecx", "edx"};
        return regs[idx % regs.size()];
    }

    string generateAssembly(vector<string>& asmCode, int& regCount) const {
        if (nodeType == "NumberLiteral" || nodeType == "Identifier") {
            string reg = getRegister(regCount++);
            asmCode.push_back("mov " + reg + ", " + value);
            return reg;
        }
        if (nodeType == "BinaryExpr") {
            int leftRegIdx = regCount;
            string leftReg = children[0]->generateAssembly(asmCode, regCount);
            int rightRegIdx = regCount;
            string rightReg = children[1]->generateAssembly(asmCode, regCount);

            if (value == "+") {
                asmCode.push_back("add " + leftReg + ", " + rightReg);
                return leftReg;
            } else if (value == "-") {
                asmCode.push_back("sub " + leftReg + ", " + rightReg);
                return leftReg;
            } else if (value == "*") {
                asmCode.push_back("imul " + leftReg + ", " + rightReg);
                return leftReg;
            } else if (value == "/") {
                asmCode.push_back("cdq");
                asmCode.push_back("idiv " + rightReg);
                return leftReg;
            }
        }
        if (nodeType == "Declaration") {
            if (!children.empty()) {
                int regCountLocal = 0;
                string resultReg = children[0]->generateAssembly(asmCode, regCountLocal);
                asmCode.push_back("mov " + value + ", " + resultReg);
            } else {
                asmCode.push_back("mov " + value + ", 0");
            }
            return "";
        }
        if (nodeType == "Program") {
            for (const auto& child : children) {
                child->generateAssembly(asmCode, regCount);
            }
            return "";
        }
        if (nodeType == "Return") {
            if (!children.empty()) {
                int regCountLocal = 0;
                string retReg = children[0]->generateAssembly(asmCode, regCountLocal);
                asmCode.push_back("mov eax, " + retReg); // Conventionally, return value in eax
                asmCode.push_back("ret");
            }
            return "";
        }        
        return "";
    }
};

// Interpreter for executing the AST and showing output
class Interpreter {
    map<string, int> variables;
public:
    void execute(shared_ptr<ASTNode> node) {
        if (!node) return;
        if (node->nodeType == "Program") {
            for (auto& child : node->children)
                execute(child);
        } else if (node->nodeType == "Declaration") {
            string var = node->value;
            int val = evaluate(node->children[0]);
            variables[var] = val;
        } else if (node->nodeType == "Print") {
            if (!node->children.empty()) {
                if (node->children[0]->nodeType == "StringLiteral") {
                    // Remove quotes from string literal
                    string s = node->children[0]->value;
                    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
                        s = s.substr(1, s.size()-2);
                    cout << s << endl;
                } else {
                    int val = evaluate(node->children[0]);
                    cout << val << endl;
                }
            }
        } else if (node->nodeType == "IfElse") {
            int cond = evaluate(node->children[0]);
            if (cond) {
                execute(node->children[1]);
            } else if (node->children.size() > 2) {
                execute(node->children[2]);
            }
        } else if (node->nodeType == "Block") {
            for (auto& child : node->children)
                execute(child);
        } else if (node->nodeType == "Return") {
            // For now, just print the return value
            int val = evaluate(node->children[0]);
            cout << "Return: " << val << endl;
        }
    }

    int evaluate(shared_ptr<ASTNode> node) {
        if (node->nodeType == "NumberLiteral") {
            return stoi(node->value);
        } else if (node->nodeType == "Identifier") {
            return variables[node->value];
        } else if (node->nodeType == "BinaryExpr") {
            int left = evaluate(node->children[0]);
            int right = evaluate(node->children[1]);
            if (node->value == "+") return left + right;
            if (node->value == "-") return left - right;
            if (node->value == "*") return left * right;
            if (node->value == "/") return left / right;
            if (node->value == "==") return left == right;
            if (node->value == "!=") return left != right;
            if (node->value == "<") return left < right;
            if (node->value == "<=") return left <= right;
            if (node->value == ">") return left > right;
            if (node->value == ">=") return left >= right;
        }
        return 0;
    }
};


// Compiler class
class MukkuCompiler {
private:
    vector<Token> tokens;
    size_t currentTokenIndex = 0;
    map<string, string> symbolTable;
    vector<string> errors;
    vector<string> intermediateCode;
    shared_ptr<ASTNode> ast;
    // --- Reserved keywords set for identifier check ---
    const std::set<std::string> reservedKeywords = {"val", "prt", "agar", "nhi-to", "bhejo"};

public:
    void compile(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Could not open file '" << filename << "'" << endl;
            return;
        }

        string sourceCode((istreambuf_iterator<char>(file)), 
                         istreambuf_iterator<char>());
        file.close();

        cout << "=== Source Code ===" << endl;
        cout << sourceCode << endl << endl;

        // Phase 1: Lexical Analysis
        cout << "=== Lexical Analysis (Tokenization) ===" << endl;
        tokenize(sourceCode);
        printTokens();

        if (!errors.empty()) {
            printErrors();
            return;
        }

        // Phase 2: Syntax Analysis
        cout << "\n=== Syntax Analysis (Parsing) ===" << endl;
        ast = parseProgram();

        if (!errors.empty()) {
            printErrors();
            return;
        } 

        // === JSON Parse Tree Output ===
        cout << "\nParse Tree (JSON):" << endl;
        if (ast) ast->printJSON(cout, 0);
        cout << endl;

        // Phase 3: Semantic Analysis
        cout << "\n=== Semantic Analysis ===" << endl;
        semanticAnalysis(ast.get());

        if (!errors.empty()) {
            printErrors();
            return;
        }

        cout << "\nSymbol Table:" << endl;
        for (const auto& entry : symbolTable) {
            cout << entry.first << ": " << entry.second << endl;
        }

        // Phase 4: Intermediate Code Generation
        cout << "\n=== Intermediate Code Generation ===" << endl;
        int tempCount = 0;
        ast->generateIntermediateCode(intermediateCode, tempCount);

        cout << "\nIntermediate Code (Three-Address Code):" << endl;
        for (size_t i = 0; i < intermediateCode.size(); ++i) {
            cout << i << ": " << intermediateCode[i] << endl;
        }

        // Phase 5: Assembly Code Generation
        cout << "\n=== Assembly Code Generation ===" << endl;
        vector<string> asmCode;
        int regCount = 0;
        if (ast) ast->generateAssembly(asmCode, regCount);

        cout << "\nAssembly Code:" << endl;
        for (size_t i = 0; i < asmCode.size(); ++i) {
            cout << i << ": " << asmCode[i] << endl;
        }

        cout << "\nCompilation successful!" << endl;
        cout << "\n=== Output of Input Code ===" << endl;
        Interpreter interpreter;
        interpreter.execute(ast);

    }

private:
    void tokenize(const string& source) {
        vector<pair<string, TokenType>> tokenSpecs = {
            {"val", TokenType::VAL},
            {"prt", TokenType::PRT},
            {"agar", TokenType::AGAR},
            {"nhi-to", TokenType::NHI_TO},
            {"bhejo", TokenType::BHEJO},
            {"==", TokenType::COMPARE},
    {"!=", TokenType::COMPARE},
    {"<=", TokenType::COMPARE},
    {">=", TokenType::COMPARE},

    // Single-character comparison operators
    {"<", TokenType::COMPARE},
    {">", TokenType::COMPARE},

    // Assignment operator
    {"=", TokenType::ASSIGN},

    {"[+*/\\-]", TokenType::OP},

    // Parentheses and braces
    {"\\(", TokenType::LPAREN},
    {"\\)", TokenType::RPAREN},
    {"\\{", TokenType::LBRACE},
    {"\\}", TokenType::RBRACE},
    {";", TokenType::SEMI},

    // Literals
    {"\"[^\"]*\"", TokenType::STRING}, // String literals
    {"[0-9]+", TokenType::NUMBER},     // Numbers

    // Identifiers
    {"[a-zA-Z_][a-zA-Z0-9_]*", TokenType::ID},
        };
        
        

        size_t pos = 0;
        int line = 1;
        int lineStart = 0;

        while (pos < source.length()) {
            while (pos < source.length() && isspace(source[pos])) {
                if (source[pos] == '\n') {
                    line++;
                    lineStart = pos + 1;
                }
                pos++;
            }
            if (pos >= source.length()) break;

            bool matched = false;
            for (size_t i = 0; i < tokenSpecs.size(); ++i) {
                const string& pattern = tokenSpecs[i].first;
                TokenType type = tokenSpecs[i].second;

                regex re("^" + pattern);
                smatch match;
                string remaining = source.substr(pos);
                
                if (regex_search(remaining, match, re)) {
                    string value = match.str();
                    int column = pos - lineStart;

                    tokens.emplace_back(type, value, line, column);
                    pos += value.length();
                    matched = true;
                    break;
                }
            }

            if (!matched) {
                int column = pos - lineStart;
                errors.push_back("Illegal character '" + string(1, source[pos]) + 
                               "' at line " + to_string(line) + 
                               ", column " + to_string(column));
                pos++;
            }
        }
        tokens.emplace_back(TokenType::END, "", line, 0);
    }

    // --- Parser for declarations and expressions ---
    shared_ptr<ASTNode> parseStatement() {
        if (currentToken().type == TokenType::VAL) {
            return parseDeclaration();
        } else if (currentToken().type == TokenType::PRT) {
            return parsePrint();
        } else if (currentToken().type == TokenType::AGAR) {
            return parseIfElse();
        } else if (currentToken().type == TokenType::BHEJO) {
            return parseReturn();
        } else {
            errors.push_back("Unexpected statement or keyword '" + currentToken().value + "' at line " +
                             to_string(currentToken().line) + ", column " + to_string(currentToken().column));
            advance();
            return nullptr;
        }
    }
    
    shared_ptr<ASTNode> parseProgram() {
        auto program = make_shared<ASTNode>("Program");
        while (currentToken().type != TokenType::END) {
            auto stmt = parseStatement();
            if (stmt) program->addChild(stmt);
        }
        return program;
    }
    

    shared_ptr<ASTNode> parseReturn() {
        advance(); // skip 'bhejo'
        auto expr = parseExpression();
        if (!expr) {
            errors.push_back("Invalid expression in bhejo statement");
            return nullptr;
        }
        if (currentToken().type != TokenType::SEMI) {
            errors.push_back("Expected ';' after bhejo statement");
            return nullptr;
        }
        advance(); // skip ';'
        auto retNode = make_shared<ASTNode>("Return");
        retNode->addChild(expr);
        return retNode;
    }
    
    shared_ptr<ASTNode> parsePrint() {
        advance(); // skip 'prt'
        if (currentToken().type != TokenType::LPAREN) {
            errors.push_back("Expected '(' after 'prt'");
            return nullptr;
        }
        advance(); // skip '('
    
        shared_ptr<ASTNode> expr = nullptr;
        if (currentToken().type == TokenType::STRING) {
            expr = make_shared<ASTNode>("StringLiteral", currentToken().value);
            advance();
        } else {
            expr = parseExpression();
        }
    
        if (currentToken().type != TokenType::RPAREN) {
            errors.push_back("Expected ')' after prt argument");
            return nullptr;
        }
        advance(); // skip ')'
    
        if (currentToken().type != TokenType::SEMI) {
            errors.push_back("Expected ';' after prt statement");
            return nullptr;
        }
        advance(); // skip ';'
    
        auto prtNode = make_shared<ASTNode>("Print");
        if (expr) prtNode->addChild(expr);
        return prtNode;
    }
    
    shared_ptr<ASTNode> parseIfElse() {
        advance(); // skip 'agar'
        if (currentToken().type != TokenType::LPAREN) {
            errors.push_back("Expected '(' after 'agar'");
            return nullptr;
        }
        advance(); // skip '('
    
        auto condition = parseExpression();
        if (!condition) {
            errors.push_back("Invalid condition in agar statement");
            return nullptr;
        }
    
        if (currentToken().type != TokenType::RPAREN) {
            errors.push_back("Expected ')' after agar condition");
            return nullptr;
        }
        advance(); // skip ')'
    
        if (currentToken().type != TokenType::LBRACE) {
            errors.push_back("Expected '{' after agar condition");
            return nullptr;
        }
        advance(); // skip '{'
    
        auto ifBlock = make_shared<ASTNode>("Block");
        while (currentToken().type != TokenType::RBRACE && currentToken().type != TokenType::END) {
            auto stmt = parseStatement();
            if (stmt) ifBlock->addChild(stmt);
        }
        if (currentToken().type != TokenType::RBRACE) {
            errors.push_back("Expected '}' at end of agar block");
            return nullptr;
        }
        advance(); // skip '}'
    
        // Optional nhi-to
        shared_ptr<ASTNode> elseBlock = nullptr;
        if (currentToken().type == TokenType::NHI_TO) {
            advance(); // skip 'nhi-to'
            if (currentToken().type != TokenType::LBRACE) {
                errors.push_back("Expected '{' after nhi-to");
                return nullptr;
            }
            advance(); // skip '{'
            elseBlock = make_shared<ASTNode>("Block");
            while (currentToken().type != TokenType::RBRACE && currentToken().type != TokenType::END) {
                auto stmt = parseStatement();
                if (stmt) elseBlock->addChild(stmt);
            }
            if (currentToken().type != TokenType::RBRACE) {
                errors.push_back("Expected '}' at end of nhi-to block");
                return nullptr;
            }
            advance(); // skip '}'
        }
    
        auto ifElseNode = make_shared<ASTNode>("IfElse");
        ifElseNode->addChild(condition);
        ifElseNode->addChild(ifBlock);
        if (elseBlock) ifElseNode->addChild(elseBlock);
    
        return ifElseNode;
    }
    

    shared_ptr<ASTNode> parseDeclaration() {
        advance(); // skip 'val'
        // Check if the next token is a keyword
    if (currentToken().type == TokenType::VAL ||
    currentToken().type == TokenType::PRT ||
    currentToken().type == TokenType::AGAR ||
    currentToken().type == TokenType::NHI_TO ||
    currentToken().type == TokenType::BHEJO) {
    errors.push_back("Cannot use reserved keyword '" + currentToken().value + "' as an identifier after 'val' at line " + to_string(currentToken().line));
    return nullptr;
}
        if (currentToken().type != TokenType::ID) {
            errors.push_back("Expected identifier after 'val' at line " + to_string(currentToken().line));
            return nullptr;
        }
        string varName = currentToken().value;
        if (reservedKeywords.count(varName)) {
            errors.push_back("Cannot use reserved keyword '" + varName + "' as an identifier after 'val' at line " + to_string(currentToken().line));
            return nullptr;
        }
        advance(); // skip ID

        shared_ptr<ASTNode> expr = nullptr;
        if (currentToken().type == TokenType::ASSIGN) {
            advance(); // skip '='
            expr = parseExpression();
            if (!expr) {
                errors.push_back("Invalid expression in declaration at line " + to_string(currentToken().line));
                return nullptr;
            }
        }
        if (currentToken().type != TokenType::SEMI) {
            errors.push_back("Expected ';' at end of declaration at line " + to_string(currentToken().line));
            return nullptr;
        }
        advance(); // skip ';'

        auto decl = make_shared<ASTNode>("Declaration", varName);
        if (expr) decl->addChild(expr);
        return decl;
    }

    shared_ptr<ASTNode> parseExpression(int minPrec = 0) {
        auto left = parsePrimary();
        while (true) {
            string op = currentToken().value;
            int prec = getPrecedence(op);
            if ((currentToken().type == TokenType::OP || currentToken().type == TokenType::COMPARE) && prec >= minPrec) {
                advance();
                auto right = parseExpression(prec + 1);
                if (!right) return nullptr;
                auto bin = make_shared<ASTNode>("BinaryExpr", op);
                bin->addChild(left);
                bin->addChild(right);
                left = bin;
            } else {
                break;
            }
        }
        return left;
    }

    shared_ptr<ASTNode> parsePrimary() {
        if (currentToken().type == TokenType::ID) {
            auto node = make_shared<ASTNode>("Identifier", currentToken().value);
            advance();
            return node;
        }
        else if (currentToken().type == TokenType::NUMBER) {
            auto node = make_shared<ASTNode>("NumberLiteral", currentToken().value);
            advance();
            return node;
        }
        else {
            errors.push_back("Expected identifier or number in expression");
            return nullptr;
        }
    }

    int getPrecedence(const string& op) {
        if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") return 0;
        if (op == "+" || op == "-") return 1;
        if (op == "*" || op == "/") return 2;
        return -1;
    }
    

    Token& currentToken() { return tokens[currentTokenIndex]; }
    void advance() { if (currentTokenIndex < tokens.size() - 1) ++currentTokenIndex; }

    void printTokens() const {
        for (const auto& token : tokens) {
            cout << "Line " << token.line << ", Column " << token.column << ": ";
            switch (token.type) {
                case TokenType::VAL:
                case TokenType::PRT:
                case TokenType::AGAR:
                case TokenType::NHI_TO:
                case TokenType::BHEJO:
                    cout << "Keyword"; break;
                case TokenType::ID: cout << "ID"; break;
                case TokenType::NUMBER: cout << "NUMBER"; break;
                case TokenType::STRING: cout << "STRING"; break;
                case TokenType::OP: cout << "OP"; break;
                case TokenType::COMPARE: cout << "COMPARE"; break;
                case TokenType::ASSIGN: cout << "ASSIGN"; break;
                case TokenType::LPAREN: cout << "LPAREN"; break;
                case TokenType::RPAREN: cout << "RPAREN"; break;
                case TokenType::LBRACE: cout << "LBRACE"; break;
                case TokenType::RBRACE: cout << "RBRACE"; break;
                case TokenType::SEMI: cout << "SEMI"; break;
                case TokenType::END: cout << "END"; break;
            }
            cout << " = " << token.value << endl;
        }
    }

    void semanticAnalysis(ASTNode* node) {
        if (!node) return;
        if (node->nodeType == "Program") {
            for (const auto& child : node->children) {
                semanticAnalysis(child.get());
            }
        }
        else if (node->nodeType == "Declaration") {
            if (symbolTable.find(node->value) != symbolTable.end()) {
                errors.push_back("Variable '" + node->value + "' already declared.");
            } else {
                symbolTable[node->value] = "variable";
            }
            if (!node->children.empty()) {
                semanticAnalysis(node->children[0].get());
            }
        }
        else if (node->nodeType == "Identifier") {
            if (symbolTable.find(node->value) == symbolTable.end()) {
                errors.push_back("Undeclared variable '" + node->value + "'");
            }
        }
        else if (node->nodeType == "BinaryExpr") {
            semanticAnalysis(node->children[0].get());
            semanticAnalysis(node->children[1].get());
        }
        else if (node->nodeType == "Return") {
            if (!node->children.empty())
                semanticAnalysis(node->children[0].get());
        }
         // === FIX: Add this else branch to handle all other node types ===
    else {
        for (const auto& child : node->children) {
            semanticAnalysis(child.get());
        }
    }
        
    }

    void printErrors() const {
        cout << "\nCompilation errors:" << endl;
        for (const auto& error : errors) {
            cout << error << endl;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <filename.anshu>" << endl;
        return 1;
    }

    MukkuCompiler compiler;
    compiler.compile(argv[1]);

    return 0;
}