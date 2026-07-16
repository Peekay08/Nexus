#include "ir.hpp"
#include "../ast/ast.hpp"
#include <sstream>


std::string irOpToString(IROp op) {
    switch (op) {
        case IROp::ALLOCA: return "alloca";
        case IROp::STORE: return "store";
        case IROp::LOAD: return "load";
        case IROp::CONST: return "const";
        case IROp::ADD: return "add";
        case IROp::SUB: return "sub";
        case IROp::MUL: return "mul";
        case IROp::DIV: return "div";
        case IROp::MOD: return "mod";
        case IROp::EQ: return "eq";
        case IROp::NE: return "ne";
        case IROp::LT: return "lt";
        case IROp::LE: return "le";
        case IROp::GT: return "gt";
        case IROp::GE: return "ge";
        case IROp::LABEL: return "label";
        case IROp::BR: return "br";
        case IROp::CBR: return "cbr";
        case IROp::CALL: return "call";
        case IROp::RET: return "ret";
        case IROp::PRINT: return "print";
        default: return "unknown";
    }
}

std::string IRInstruction::toString() const {
    std::stringstream ss;
    switch (op) {
        case IROp::ALLOCA:
            ss << "  " << dest << " = alloca " << arg1;
            break;
        case IROp::STORE:
            ss << "  store " << dest << ", " << arg1;
            break;
        case IROp::LOAD:
            ss << "  " << dest << " = load " << arg1;
            break;
        case IROp::CONST:
            ss << "  " << dest << " = const " << arg1;
            break;
        case IROp::ADD:
        case IROp::SUB:
        case IROp::MUL:
        case IROp::DIV:
        case IROp::MOD:
        case IROp::EQ:
        case IROp::NE:
        case IROp::LT:
        case IROp::LE:
        case IROp::GT:
        case IROp::GE:
            ss << "  " << dest << " = " << irOpToString(op) << " " << arg1 << ", " << arg2;
            break;
        case IROp::LABEL:
            ss << dest << ":";
            break;
        case IROp::BR:
            ss << "  br label %" << dest;
            break;
        case IROp::CBR:
            ss << "  cbr " << dest << ", label %" << arg1 << ", label %" << arg2;
            break;
        case IROp::CALL:
            ss << "  " << dest << " = call @" << arg1 << "(";
            for (size_t i = 0; i < callArgs.size(); ++i) {
                ss << callArgs[i];
                if (i + 1 < callArgs.size()) ss << ", ";
            }
            ss << ")";
            break;
        case IROp::RET:
            ss << "  ret " << dest;
            break;
        case IROp::PRINT:
            ss << "  print " << arg1;
            break;
    }
    return ss.str();
}

std::string IRFunction::toString() const {
    std::stringstream ss;
    ss << "define " << returnType << " @" << name << "(";
    for (size_t i = 0; i < paramNames.size(); ++i) {
        ss << "%" << paramNames[i];
        if (i + 1 < paramNames.size()) ss << ", ";
    }
    ss << ") {\n";
    for (const auto& inst : instructions) {
        ss << inst.toString() << "\n";
    }
    ss << "}\n";
    return ss.str();
}

std::string IRProgram::toString() const {
    std::stringstream ss;
    ss << "; NEXUS IR v1.0\n\n";
    for (const auto& func : functions) {
        ss << func.toString() << "\n";
    }
    return ss.str();
}

// --- IRGenerator Implementation ---

IRGenerator::IRGenerator() : regCounter(0), labelCounter(0) {
    enterScope(); // global scope
}

IRProgram IRGenerator::generate(const ASTNode& root) {
    program.functions.clear();
    root.codegen(*this);
    return program;
}

std::string IRGenerator::nextRegister() {
    return "%" + std::to_string(regCounter++);
}

std::string IRGenerator::nextLabel(const std::string& prefix) {
    return prefix + "_" + std::to_string(labelCounter++);
}

void IRGenerator::emit(IROp op, const std::string& dest, const std::string& arg1, const std::string& arg2, const std::vector<std::string>& callArgs) {
    currentFunc.instructions.push_back(IRInstruction{op, dest, arg1, arg2, callArgs});
}

void IRGenerator::enterScope() {
    symbolTables.push_back(std::map<std::string, std::string>());
}

void IRGenerator::exitScope() {
    if (!symbolTables.empty()) {
        symbolTables.pop_back();
    }
}

void IRGenerator::declareVariable(const std::string& name, const std::string& type) {
    if (!symbolTables.empty()) {
        symbolTables.back()[name] = type;
    }
}

std::string IRGenerator::getVariableType(const std::string& name) {
    for (auto it = symbolTables.rbegin(); it != symbolTables.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return "";
}

// --- Node codegen ---

std::string IRGenerator::codegenLiteralInt(int val) {
    std::string reg = nextRegister();
    emit(IROp::CONST, reg, std::to_string(val));
    return reg;
}

std::string IRGenerator::codegenLiteralFloat(float val) {
    std::string reg = nextRegister();
    emit(IROp::CONST, reg, std::to_string(val));
    return reg;
}

std::string IRGenerator::codegenLiteralString(const std::string& val) {
    std::string reg = nextRegister();
    std::string escaped = "";
    for (char c : val) {
        if (c == '\n') escaped += "\\n";
        else if (c == '\t') escaped += "\\t";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '"') escaped += "\\\"";
        else escaped += c;
    }
    emit(IROp::CONST, reg, "\"" + escaped + "\"");
    return reg;
}

std::string IRGenerator::codegenIdentifier(const std::string& name) {
    // Treat standard symbols specially
    if (name == "std::cout" || name == "std::endl") {
        return name;
    }
    
    // Read local variable
    std::string type = getVariableType(name);
    if (!type.empty()) {
        std::string reg = nextRegister();
        // Load the variable address %var_name into register
        emit(IROp::LOAD, reg, "%addr_" + name);
        return reg;
    }
    
    return "%" + name; // fallback
}

std::string IRGenerator::codegenBinaryOp(const std::string& op, const ExpressionNode& left, const ExpressionNode& right) {
    // Special handling for std::cout << printing
    if (op == "<<") {
        std::string leftVal = left.codegen(*this);
        if (leftVal == "std::cout") {
            std::string rightVal = right.codegen(*this);
            if (rightVal == "std::endl") {
                emit(IROp::PRINT, "", "\"\\n\"");
            } else {
                emit(IROp::PRINT, "", rightVal);
            }
            return "std::cout";
        }
    }

    std::string lhs = left.codegen(*this);
    std::string rhs = right.codegen(*this);
    std::string dest = nextRegister();

    IROp operation;
    if (op == "+") operation = IROp::ADD;
    else if (op == "-") operation = IROp::SUB;
    else if (op == "*") operation = IROp::MUL;
    else if (op == "/") operation = IROp::DIV;
    else if (op == "%") operation = IROp::MOD;
    else if (op == "==") operation = IROp::EQ;
    else if (op == "!=") operation = IROp::NE;
    else if (op == "<") operation = IROp::LT;
    else if (op == "<=") operation = IROp::LE;
    else if (op == ">") operation = IROp::GT;
    else if (op == ">=") operation = IROp::GE;
    else operation = IROp::ADD; // fallback

    emit(operation, dest, lhs, rhs);

    return dest;
}

std::string IRGenerator::codegenFuncCall(const std::string& name, const std::vector<std::unique_ptr<ExpressionNode>>& args) {
    std::vector<std::string> compiledArgs;
    for (const auto& arg : args) {
        compiledArgs.push_back(arg->codegen(*this));
    }
    std::string dest = nextRegister();
    emit(IROp::CALL, dest, name, "", compiledArgs);
    return dest;
}

std::string IRGenerator::codegenVarDecl(const std::string& type, const std::string& name, const ExpressionNode* init) {
    declareVariable(name, type);
    emit(IROp::ALLOCA, "%addr_" + name, type);
    if (init) {
        std::string initReg = init->codegen(*this);
        emit(IROp::STORE, initReg, "%addr_" + name);
    }
    return "";
}

std::string IRGenerator::codegenAssign(const std::string& name, const ExpressionNode& val) {
    std::string valReg = val.codegen(*this);
    emit(IROp::STORE, valReg, "%addr_" + name);
    return "";
}

std::string IRGenerator::codegenBlock(const std::vector<std::unique_ptr<StatementNode>>& statements) {
    enterScope();
    for (const auto& stmt : statements) {
        stmt->codegen(*this);
    }
    exitScope();
    return "";
}

std::string IRGenerator::codegenIf(const ExpressionNode& cond, const StatementNode& thenBr, const StatementNode* elseBr) {
    std::string condReg = cond.codegen(*this);
    
    std::string thenLabel = nextLabel("then");
    std::string elseLabel = elseBr ? nextLabel("else") : "";
    std::string mergeLabel = nextLabel("merge");

    if (elseBr) {
        emit(IROp::CBR, condReg, thenLabel, elseLabel);
    } else {
        emit(IROp::CBR, condReg, thenLabel, mergeLabel);
    }

    // Then branch
    emit(IROp::LABEL, thenLabel);
    thenBr.codegen(*this);
    emit(IROp::BR, mergeLabel);

    // Else branch
    if (elseBr) {
        emit(IROp::LABEL, elseLabel);
        elseBr->codegen(*this);
        emit(IROp::BR, mergeLabel);
    }

    emit(IROp::LABEL, mergeLabel);
    return "";
}

std::string IRGenerator::codegenWhile(const ExpressionNode& cond, const StatementNode& body) {
    std::string condLabel = nextLabel("cond");
    std::string bodyLabel = nextLabel("body");
    std::string endLabel = nextLabel("end");

    emit(IROp::LABEL, condLabel);
    std::string condReg = cond.codegen(*this);
    emit(IROp::CBR, condReg, bodyLabel, endLabel);

    emit(IROp::LABEL, bodyLabel);
    body.codegen(*this);
    emit(IROp::BR, condLabel);

    emit(IROp::LABEL, endLabel);
    return "";
}

std::string IRGenerator::codegenReturn(const ExpressionNode* expr) {
    if (expr) {
        std::string valReg = expr->codegen(*this);
        emit(IROp::RET, valReg);
    } else {
        emit(IROp::RET);
    }
    return "";
}

std::string IRGenerator::codegenExprStmt(const ExpressionNode& expr) {
    expr.codegen(*this);
    return "";
}

// --- Virtual Methods implementations for AST Nodes ---

std::string LiteralIntNode::codegen(IRGenerator& gen) const { return gen.codegenLiteralInt(value); }
std::string LiteralFloatNode::codegen(IRGenerator& gen) const { return gen.codegenLiteralFloat(value); }
std::string LiteralStringNode::codegen(IRGenerator& gen) const { return gen.codegenLiteralString(value); }
std::string IdentifierNode::codegen(IRGenerator& gen) const { return gen.codegenIdentifier(name); }
std::string BinaryOpNode::codegen(IRGenerator& gen) const { return gen.codegenBinaryOp(op, *left, *right); }
std::string FuncCallNode::codegen(IRGenerator& gen) const { return gen.codegenFuncCall(funcName, args); }
std::string VarDeclNode::codegen(IRGenerator& gen) const { return gen.codegenVarDecl(typeName, name, initVal.get()); }
std::string AssignNode::codegen(IRGenerator& gen) const { return gen.codegenAssign(name, *val); }
std::string BlockNode::codegen(IRGenerator& gen) const { return gen.codegenBlock(statements); }
std::string IfStmtNode::codegen(IRGenerator& gen) const { return gen.codegenIf(*cond, *thenBranch, elseBranch.get()); }
std::string WhileStmtNode::codegen(IRGenerator& gen) const { return gen.codegenWhile(*cond, *body); }
std::string ReturnStmtNode::codegen(IRGenerator& gen) const { return gen.codegenReturn(expr.get()); }
std::string ExprStmtNode::codegen(IRGenerator& gen) const { return gen.codegenExprStmt(*expr); }

std::string FunctionDeclNode::codegen(IRGenerator& gen) const {
    gen.enterScope();
    gen.currentFunc = IRFunction{};
    gen.currentFunc.name = name;
    gen.currentFunc.returnType = returnType;
    
    // Add parameters
    for (const auto& arg : args) {
        gen.currentFunc.paramNames.push_back(arg.name);
        gen.declareVariable(arg.name, arg.type);
        // Map parameter to allocated space in function frame
        gen.emit(IROp::ALLOCA, "%addr_" + arg.name, arg.type);
        gen.emit(IROp::STORE, "%" + arg.name, "%addr_" + arg.name);
    }

    body->codegen(gen);
    
    // Append a default return if none exists to guarantee correct stack unwinding
    if (gen.currentFunc.instructions.empty() || gen.currentFunc.instructions.back().op != IROp::RET) {
        if (returnType == "void") {
            gen.emit(IROp::RET);
        } else if (returnType == "float") {
            std::string r = gen.nextRegister();
            gen.emit(IROp::CONST, r, "0.0");
            gen.emit(IROp::RET, r);
        } else {
            std::string r = gen.nextRegister();
            gen.emit(IROp::CONST, r, "0");
            gen.emit(IROp::RET, r);
        }
    }

    gen.exitScope();
    gen.program.functions.push_back(gen.currentFunc);
    return "";
}

std::string ProgramNode::codegen(IRGenerator& gen) const {
    for (const auto& func : functions) {
        func->codegen(gen);
    }
    return "";
}
