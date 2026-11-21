#include "Parser.h"
#include <string_view>
#include <cctype>
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <functional>

Compiler::Compiler(std::string_view input) : src(input) {}

bool Compiler::atEnd() const { return pos >= src.size(); }
char Compiler::peek()  const { return atEnd() ? '\0' : src[pos]; }
char Compiler::peek2() const { return (pos + 1 < src.size()) ? src[pos + 1] : '\0'; }
void Compiler::advance() { ++pos; }

void Compiler::skipWs() {
    while (!atEnd()) {
        if (std::isspace(static_cast<unsigned char>(peek()))) { advance(); continue; }
        if (peek() == '/' && peek2() == '/') {
            advance(); advance();
            while (!atEnd() && peek() != '\n') advance();
            continue;
        }
        if (peek() == '/' && peek2() == '*') {
            advance(); advance();
            bool closed = false;
            while (!atEnd()) {
                if (peek() == '*' && peek2() == '/') { advance(); advance(); closed = true; break; }
                advance();
            }
            if (!closed) throw std::runtime_error("Unclosed multi-line comment");
            continue;
        }
        break;
    }
}

bool Compiler::eatAnd() {
    skipWs();
    if (peek() == '&' && peek2() == '&') { pos += 2; return true; }
    return false;
}

void Compiler::expect(char c) {
    skipWs();
    if (peek() != c) throw std::runtime_error(std::string("Expected '") + c + "'");
    advance();
}

bool Compiler::checkKeyword(const std::string& kw) {
    skipWs();
    if (pos + kw.size() > src.size()) return false;
    if (src.compare(pos, kw.size(), kw) != 0) return false;
    size_t end = pos + kw.size();
    if (end < src.size()) {
        char c = src[end];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') return false;
    }
    return true;
}

void Compiler::consumeKeyword(const std::string& kw) {
    if (!checkKeyword(kw)) throw std::runtime_error("Expected keyword: " + kw);
    pos += kw.size();
}

void Compiler::emit(Command c) { program.push_back(std::move(c)); }
void Compiler::emit(OpCode op) { program.push_back(Command{ op }); }

std::vector<Command> Compiler::compile() {
    parseProgram();
    emit(OpCode::Halt);
    return std::move(program);
}

void Compiler::parseProgram() {
    while (true) {
        skipWs();
        if (atEnd()) break;
        if (checkKeyword("func")) { consumeKeyword("func"); parseFunctionDef(false); continue; }
        if (checkKeyword("proc")) { consumeKeyword("proc"); parseFunctionDef(true); continue; }
        parseStatement();
    }
}

void Compiler::parseFunctionDef(bool isProc) {
    skipWs();
    if (checkKeyword("int")) { consumeKeyword("int"); skipWs(); }
    else if (checkKeyword("float")) { consumeKeyword("float"); skipWs(); }
    else if (checkKeyword("double")) { consumeKeyword("double"); skipWs(); }

    std::string name = parseIdentifier();
    expect('(');
    std::vector<std::string> params;
    skipWs();
    if (peek() != ')') {
        params.push_back(parseIdentifier());
        skipWs();
        while (peek() == ',') {
            advance(); skipWs(); params.push_back(parseIdentifier()); skipWs();
        }
    }
    expect(')');

    Command def;
    def.op = isProc ? OpCode::ProcDef : OpCode::FuncDef;
    def.name = name;
    def.params = params;
    emit(def);
    size_t defPos = program.size() - 1;

    emit(OpCode::Jmp);
    size_t jmpPos = program.size() - 1;

    program[defPos].target = (int)program.size();
    parseBlock();

    Command ret;
    ret.op = OpCode::Return;
    ret.num = 0.0;
    emit(ret);

    program[jmpPos].target = (int)program.size();
}

void Compiler::parseStatement() {
    skipWs();
    if (atEnd()) return;

    if (checkKeyword("return")) {
        consumeKeyword("return"); skipWs();
        if (peek() == ';') { advance(); Command c; c.op = OpCode::Return; c.num = 0.0; emit(c); }
        else { parseExpr(); skipWs(); if (peek() == ';') advance(); emit(OpCode::Return); }
        return;
    }

    if (checkKeyword("go")) {
        consumeKeyword("go"); skipWs();
        std::string name = parseIdentifier();
        expect('(');
        int argc = 0; skipWs();
        if (peek() != ')') {
            parseExpr(); argc++; skipWs();
            while (peek() == ',') { advance(); parseExpr(); argc++; skipWs(); }
        }
        expect(')'); skipWs();
        if (peek() == ';') advance();
        Command c; c.op = OpCode::GoCall; c.name = name; c.argc = argc;
        emit(c); return;
    }

    if (checkKeyword("sync")) {
        consumeKeyword("sync"); skipWs();
        if (peek() == ';') advance();
        emit(OpCode::Sync); return;
    }

    if (checkKeyword("lock")) {
        consumeKeyword("lock"); expect('(');
        std::string var = parseIdentifier(); expect(')'); skipWs();
        if (peek() == ';') advance();
        Command c; c.op = OpCode::Lock; c.name = var; emit(c); return;
    }

    if (checkKeyword("unlock")) {
        consumeKeyword("unlock"); expect('(');
        std::string var = parseIdentifier(); expect(')'); skipWs();
        if (peek() == ';') advance();
        Command c; c.op = OpCode::Unlock; c.name = var; emit(c); return;
    }

    if (checkKeyword("int")) { consumeKeyword("int"); skipWs(); }
    else if (checkKeyword("float")) { consumeKeyword("float"); skipWs(); }
    else if (checkKeyword("double")) { consumeKeyword("double"); skipWs(); }

    if (checkKeyword("if")) { consumeKeyword("if"); parseIf(); return; }
    if (checkKeyword("while")) { consumeKeyword("while"); parseWhile(); return; }
    if (peek() == '{') { parseBlock(); return; }

    if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_') {
        size_t save = pos;
        std::string name = parseIdentifier();
        skipWs();

        if (peek() == '[') {
            advance();
            size_t progSizeBeforeIndex = program.size();
            parseExpr();
            expect(']');
            skipWs();

            if (peek() == ';') {
                advance(); emit(OpCode::MakeArraySized);
                Command c; c.op = OpCode::Store; c.name = name; emit(c); return;
            }

            if (peek() == '=' && peek2() != '=') {
                advance();
                std::vector<Command> indexCmds(program.begin() + progSizeBeforeIndex, program.end());
                program.resize(progSizeBeforeIndex);
                Command c; c.op = OpCode::PushVar; c.name = name; emit(c);
                program.insert(program.end(), indexCmds.begin(), indexCmds.end());
                parseExpr(); skipWs();
                if (peek() == ';') advance();
                emit(OpCode::StoreIndex); return;
            }

            program.resize(progSizeBeforeIndex);
            pos = save;
        }
        else if (peek() == '=' && peek2() != '=') {
            advance(); skipWs();

            if (peek() == '[') {
                advance();
                int count = 0; skipWs();
                if (peek() != ']') {
                    parseExpr(); count++; skipWs();
                    while (peek() == ',') { advance(); parseExpr(); count++; skipWs(); }
                }
                expect(']'); expect(';');
                Command c; c.op = OpCode::MakeArray; c.argc = count; emit(c);
                Command s; s.op = OpCode::Store; s.name = name; emit(s); return;
            }

            parseExpr(); skipWs();
            Command c; c.op = OpCode::Store; c.name = name; emit(c);

            while (peek() == ',') {
                advance(); skipWs();
                std::string nextName = parseIdentifier(); skipWs();
                expect('='); skipWs();
                parseExpr(); skipWs();
                Command c2; c2.op = OpCode::Store; c2.name = nextName; emit(c2);
            }

            if (peek() == ';') advance();
            return;
        }
        else {
            pos = save;
        }
    }

    parseExpr(); skipWs();
    if (peek() == ';') { advance(); emit(OpCode::Pop); }
}

void Compiler::parseBlock() {
    expect('{');
    while (true) {
        skipWs();
        if (peek() == '}') { advance(); return; }
        if (atEnd()) throw std::runtime_error("Expected '}'");
        parseStatement();
    }
}

void Compiler::parseIf() {
    skipWs();
    if (peek() == '(') { advance(); parseExpr(); expect(')'); }
    else { parseExpr(); }

    emit(OpCode::Jz);
    size_t jzPos = program.size() - 1;
    parseStatement();
    if (checkKeyword("else")) {
        consumeKeyword("else");
        emit(OpCode::Jmp);
        size_t jmpPos = program.size() - 1;
        program[jzPos].target = (int)program.size();
        parseStatement();
        program[jmpPos].target = (int)program.size();
    }
    else { program[jzPos].target = (int)program.size(); }
}

void Compiler::parseWhile() {
    size_t loopStart = (size_t)program.size();
    skipWs();
    if (peek() == '(') { advance(); parseExpr(); expect(')'); }
    else { parseExpr(); }

    emit(OpCode::Jz);
    size_t jzPos = program.size() - 1;
    parseStatement();
    emit(OpCode::Jmp);
    program.back().target = (int)loopStart;
    program[jzPos].target = (int)program.size();
}

void Compiler::parseExpr() {
    parseCompare(); skipWs();
    while (eatAnd()) { parseCompare(); emit(OpCode::And); skipWs(); }
}

void Compiler::parseCompare() {
    parseSum(); skipWs();
    while (peek() == '<' || peek() == '>') {
        char op = peek(); advance(); parseSum();
        emit(op == '<' ? OpCode::Less : OpCode::Greater); skipWs();
    }
}

void Compiler::parseSum() {
    parseTerm(); skipWs();
    while (peek() == '+' || peek() == '-') {
        char op = peek(); advance(); parseTerm();
        emit(op == '+' ? OpCode::Add : OpCode::Sub); skipWs();
    }
}

void Compiler::parseTerm() {
    parseFactor(); skipWs();
    while (peek() == '*' || peek() == '/' || peek() == '%') {
        char op = peek(); advance(); parseFactor();
        if (op == '*') emit(OpCode::Mul);
        else if (op == '/') emit(OpCode::Div);
        else emit(OpCode::Mod);
        skipWs();
    }
}

void Compiler::parseFactor() {
    skipWs();
    if (peek() == '-' || peek() == '+') {
        char op = peek(); advance();
        if (op == '-') { Command c; c.op = OpCode::PushConst; c.num = 0.0; emit(c); parseFactor(); emit(OpCode::Sub); }
        else { parseFactor(); }
        return;
    }

    parseOperand(); skipWs();
    int bangs = 0;
    while (peek() == '!') { advance(); bangs++; skipWs(); }

    if (bangs == 1) { Command c; c.op = OpCode::Call; c.name = "fact"; c.argc = 1; emit(c); }
    else if (bangs > 1) {
        Command k; k.op = OpCode::PushConst; k.num = bangs; emit(k);
        Command c; c.op = OpCode::Call; c.name = "multifact"; c.argc = 2; emit(c);
    }

    if (peek() == '^') { advance(); parseFactor(); emit(OpCode::Pow); }
}

void Compiler::parseOperand() {
    skipWs();

    // --- ДОБАВЛЕНО: Поддержка строковых литералов ---
    if (peek() == '"') {
        advance();
        std::string s;
        while (!atEnd() && peek() != '"') {
            s += peek();
            advance();
        }
        expect('"');
        Command c; c.op = OpCode::PushConst; c.str = s; c.isStr = true; emit(c); return;
    }
    // ----------------------------------------------

    if (std::isdigit(static_cast<unsigned char>(peek()))) {
        Command c; c.op = OpCode::PushConst; c.num = parseNumber(); emit(c); return;
    }
    if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_') {
        std::string name = parseIdentifier(); skipWs();
        if (peek() == '[') {
            advance(); Command c; c.op = OpCode::PushVar; c.name = name; emit(c);
            parseExpr(); expect(']'); emit(OpCode::LoadIndex); return;
        }
        if (peek() == '(') {
            advance(); int argc = 0; skipWs();
            if (peek() != ')') {
                parseExpr(); argc++; skipWs();
                while (peek() == ',') { advance(); parseExpr(); argc++; skipWs(); }
            }
            expect(')');
            Command c; c.op = OpCode::Call; c.name = name; c.argc = argc; emit(c); return;
        }
        Command c; c.op = OpCode::PushVar; c.name = name; emit(c); return;
    }
    if (peek() == '(') { advance(); parseExpr(); expect(')'); return; }
    throw std::runtime_error("Expected operand");
}

double Compiler::parseNumber() {
    size_t start = pos;
    while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
    if (!atEnd() && peek() == '.') {
        advance();
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
    }
    return std::stod(std::string(src.substr(start, pos - start)));
}

std::string Compiler::parseIdentifier() {
    size_t start = pos;
    while (!atEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) advance();
    return std::string(src.substr(start, pos - start));
}

// ============================================================
//  VM — реальная многопоточность
// ============================================================
double VM::run(const std::vector<Command>& program) {
    vars.clear();
    output.clear();
    functions.clear();
    namedLocks.clear();
    threads.clear();

    // PRE-SCAN: регистрируем все функции ДО запуска потоков.
    // Это делает functions read-only во время выполнения -> безопасно без мьютекса.
    for (size_t i = 0; i < program.size(); ++i) {
        if (program[i].op == OpCode::FuncDef || program[i].op == OpCode::ProcDef) {
            FuncInfo fi;
            fi.params = program[i].params;
            fi.bodyStart = program[i].target;
            functions[program[i].name] = fi;
        }
    }

    try {
        // Главный поток: startIp=0, нет кадров, не выходим при Return
        return executeThread(program, 0, {}, false);
    }
    catch (...) {
        // Прибираем за собой потоки, чтобы не было std::terminate
        std::vector<std::thread> toJoin;
        {
            std::lock_guard<std::mutex> lock(threadsMutex);
            toJoin = std::move(threads);
        }
        for (auto& t : toJoin) if (t.joinable()) t.join();
        throw;
    }
}

void VM::syncAllThreads() {
    auto self = std::this_thread::get_id();
    while (true) {
        std::vector<std::thread> toJoin;
        {
            std::lock_guard<std::mutex> lock(threadsMutex);
            std::vector<std::thread> remaining;
            for (auto& t : threads) {
                if (t.get_id() == self) {
                    remaining.push_back(std::move(t)); // не join сами себя
                }
                else {
                    toJoin.push_back(std::move(t));
                }
            }
            threads = std::move(remaining);
        }
        if (toJoin.empty()) break;
        for (auto& t : toJoin) if (t.joinable()) t.join();
    }
}

double VM::executeThread(const std::vector<Command>& program, size_t startIp,
    std::vector<Frame> initialFrames, bool stopOnEmptyReturn)
{
    std::vector<Value> stack;
    std::vector<Frame> localStack = std::move(initialFrames);
    size_t ip = startIp;

    auto popNum = [&]() {
        if (stack.empty()) throw std::runtime_error("Stack underflow");
        Value v = stack.back(); stack.pop_back();
        if (v.isArray()) throw std::runtime_error("Expected number, got array");
        return v.getNum();
        };

    auto binOp = [&](std::vector<Value>& st, std::function<double(double, double)> f) {
        if (st.size() < 2) throw std::runtime_error("Stack underflow");
        if (st.back().isArray() || st[st.size() - 2].isArray())
            throw std::runtime_error("Cannot perform arithmetic on an array");
        double b = st.back().getNum(); st.pop_back();
        double a = st.back().getNum(); st.pop_back();
        st.push_back(Value(f(a, b)));
        };

    while (ip < program.size()) {
        const Command& c = program[ip];
        switch (c.op) {
        case OpCode::PushConst:
            if (c.isStr) stack.push_back(Value(c.str));
            else stack.push_back(Value(c.num));
            break;

        case OpCode::PushVar: {
            bool found = false;
            for (auto it = localStack.rbegin(); it != localStack.rend(); ++it) {
                auto f = it->locals.find(c.name);
                if (f != it->locals.end()) { stack.push_back(f->second); found = true; break; }
            }
            if (!found) {
                std::lock_guard<std::mutex> lock(varsMutex);
                auto it = vars.find(c.name);
                if (it != vars.end()) stack.push_back(it->second);
                else stack.push_back(Value(0.0));
            }
            break;
        }

        case OpCode::Store: {
            if (stack.empty()) throw std::runtime_error("Stack underflow (Store)");
            Value val = stack.back();
            stack.pop_back();

            bool found = false;
            for (auto it = localStack.rbegin(); it != localStack.rend(); ++it) {
                if (it->locals.count(c.name)) {
                    it->locals[c.name] = val;
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::lock_guard<std::mutex> lock(varsMutex);
                if (vars.count(c.name)) {
                    vars[c.name] = val;
                }
                else if (!localStack.empty()) {
                    localStack.back().locals[c.name] = val;
                }
                else {
                    vars[c.name] = val;
                }
            }
            break;
        }

        case OpCode::MakeArray: {
            auto arr = std::make_shared<std::vector<double>>(c.argc);
            for (int i = c.argc - 1; i >= 0; --i) arr->at(i) = popNum();
            stack.push_back(Value(arr)); break;
        }
        case OpCode::MakeArraySized: {
            int size = (int)popNum();
            if (size < 0) throw std::runtime_error("Negative array size");
            stack.push_back(Value(std::make_shared<std::vector<double>>(size, 0.0))); break;
        }
        case OpCode::LoadIndex: {
            double idx = popNum();
            Value arrVal = stack.back(); stack.pop_back();
            if (!arrVal.isArray()) throw std::runtime_error("Not an array");
            auto arr = arrVal.getArr();
            if (idx < 0 || idx >= (double)arr->size()) throw std::runtime_error("Index out of bounds");
            stack.push_back(Value(arr->at((size_t)idx))); break;
        }
        case OpCode::StoreIndex: {
            double val = popNum();
            double idx = popNum();
            Value arrVal = stack.back(); stack.pop_back();
            if (!arrVal.isArray()) throw std::runtime_error("Not an array");
            auto arr = arrVal.getArr();
            if (idx < 0 || idx >= (double)arr->size()) throw std::runtime_error("Index out of bounds");
            arr->at((size_t)idx) = val; break;
        }

        case OpCode::Add:  binOp(stack, [](double a, double b) { return a + b; }); break;
        case OpCode::Sub:  binOp(stack, [](double a, double b) { return a - b; }); break;
        case OpCode::Mul:  binOp(stack, [](double a, double b) { return a * b; }); break;
        case OpCode::Div:  binOp(stack, [](double a, double b) { return a / b; }); break;
        case OpCode::Mod:  binOp(stack, [](double a, double b) { return std::fmod(a, b); }); break;
        case OpCode::Pow:  binOp(stack, [](double a, double b) { return std::pow(a, b); }); break;
        case OpCode::Less: binOp(stack, [](double a, double b) { return a < b ? 1.0 : 0.0; }); break;
        case OpCode::Greater: binOp(stack, [](double a, double b) { return a > b ? 1.0 : 0.0; }); break;
        case OpCode::And:  binOp(stack, [](double a, double b) { return (a != 0.0 && b != 0.0) ? 1.0 : 0.0; }); break;

        case OpCode::Call: {
            if (c.name == "print") {
                std::vector<Value> args(c.argc);
                for (int i = c.argc - 1; i >= 0; --i) {
                    if (stack.empty()) throw std::runtime_error("Stack underflow in print");
                    args[i] = stack.back(); stack.pop_back();
                }
                std::ostringstream oss;
                for (size_t i = 0; i < args.size(); ++i) {
                    const Value& arg = args[i];
                    if (arg.isString()) oss << arg.str;
                    else if (arg.isArray()) {
                        oss << "[";
                        for (size_t j = 0; j < arg.getArr()->size(); ++j) {
                            oss << arg.getArr()->at(j);
                            if (j + 1 < arg.getArr()->size()) oss << ", ";
                        }
                        oss << "]";
                    }
                    else oss << arg.getNum();
                    if (i + 1 < args.size()) {
                        if (!arg.isString() || arg.str.empty() || arg.str.back() != ' ') oss << " ";
                    }
                }
                {
                    std::lock_guard<std::mutex> lock(outputMutex);
                    output += oss.str() + "\n";
                }
                stack.push_back(Value(0.0));
            }
            else if (c.name == "resize") {
                if (c.argc != 2) throw std::runtime_error("resize expects 2 args");
                double newSize = popNum();
                Value arrVal = stack.back(); stack.pop_back();
                if (!arrVal.isArray()) throw std::runtime_error("resize expects an array");
                arrVal.getArr()->resize((size_t)newSize, 0.0);
                stack.push_back(Value(0.0));
            }
            else if (c.name == "len") {
                if (c.argc != 1) throw std::runtime_error("len expects 1 arg");
                if (stack.empty()) throw std::runtime_error("Stack underflow in len");
                Value arg = stack.back(); stack.pop_back();
                if (!arg.isArray()) throw std::runtime_error("len expects an array");
                stack.push_back(Value(static_cast<double>(arg.getArr()->size())));
            }
            else if (c.name == "sin") {
                if (c.argc != 1) throw std::runtime_error("sin expects 1 arg");
                stack.push_back(Value(std::sin(popNum())));
            }
            else if (c.name == "cos") {
                if (c.argc != 1) throw std::runtime_error("cos expects 1 arg");
                stack.push_back(Value(std::cos(popNum())));
            }
            else if (c.name == "pow") {
                if (c.argc != 2) throw std::runtime_error("pow expects 2 args");
                double exp = popNum();
                double base = popNum();
                stack.push_back(Value(std::pow(base, exp)));
            }
            else if (c.name == "fact") {
                if (c.argc != 1) throw std::runtime_error("fact expects 1 arg");
                double val = popNum();
                if (val < 0 || val != std::floor(val)) throw std::runtime_error("Factorial expects non-negative integer");
                double res = 1.0;
                for (int i = 2; i <= (int)val; ++i) res *= i;
                stack.push_back(Value(res));
            }
            else if (c.name == "multifact") {
                if (c.argc != 2) throw std::runtime_error("multifact expects 2 args");
                double step = popNum();
                double val = popNum();
                if (val < 0 || val != std::floor(val) || step < 1) throw std::runtime_error("Invalid args for multifact");
                double res = 1.0;
                for (double i = val; i > 0; i -= step) res *= i;
                stack.push_back(Value(res));
            }
            else if (functions.count(c.name)) {
                std::vector<Value> args(c.argc);
                for (int i = c.argc - 1; i >= 0; --i) { args[i] = stack.back(); stack.pop_back(); }
                const FuncInfo& fi = functions[c.name];
                Frame frame;
                frame.returnIp = ip + 1;
                for (size_t i = 0; i < fi.params.size(); ++i)
                    frame.locals[fi.params[i]] = (i < args.size() ? args[i] : Value(0.0));
                localStack.push_back(std::move(frame));
                ip = fi.bodyStart;
                continue;
            }
            else {
                for (int i = 0; i < c.argc; ++i) popNum();
                stack.push_back(Value(0.0));
            }
            break;
        }

        case OpCode::FuncDef:
        case OpCode::ProcDef:
            // Уже зарегистрировано в pre-scan. Просто пропускаем.
            break;

        case OpCode::Return: {
            Value ret = stack.empty() ? Value(0.0) : stack.back();
            if (!stack.empty()) stack.pop_back();

            if (localStack.empty()) {
                if (stopOnEmptyReturn) return ret.getNum();
                ip = program.size();
                continue;
            }

            size_t returnIp = localStack.back().returnIp;
            localStack.pop_back();

            if (localStack.empty() && stopOnEmptyReturn) {
                return ret.getNum();
            }

            ip = returnIp;
            stack.push_back(ret);
            continue;
        }

        case OpCode::GoCall: {
            std::vector<Value> args(c.argc);
            for (int i = c.argc - 1; i >= 0; --i) {
                if (stack.empty()) throw std::runtime_error("Stack underflow in go");
                args[i] = stack.back(); stack.pop_back();
            }
            auto it = functions.find(c.name);
            if (it == functions.end()) {
                for (auto& a : args) (void)a;
                break;
            }
            const FuncInfo& fi = it->second;
            Frame frame;
            frame.returnIp = program.size(); // фиктивный: не используется в потоке
            for (size_t i = 0; i < fi.params.size(); ++i)
                frame.locals[fi.params[i]] = (i < args.size() ? args[i] : Value(0.0));

            std::vector<Frame> initialStack;
            initialStack.push_back(std::move(frame));
            size_t startBody = fi.bodyStart;

            // Реальный поток!
            std::lock_guard<std::mutex> lock(threadsMutex);
            threads.emplace_back(
                [this, prog = &program, startBody, initialStack = std::move(initialStack)]() mutable {
                    try {
                        this->executeThread(*prog, startBody, std::move(initialStack), true);
                    }
                    catch (const std::exception& e) {
                        std::lock_guard<std::mutex> lk(this->outputMutex);
                        this->output += std::string("[thread error] ") + e.what() + "\n";
                    }
                });
            break;
        }

        case OpCode::Sync:
            syncAllThreads();
            break;

        case OpCode::Lock: {
            std::mutex* m = nullptr;
            {
                std::lock_guard<std::mutex> guard(locksMutex);
                if (!namedLocks[c.name]) namedLocks[c.name] = std::make_unique<std::mutex>();
                m = namedLocks[c.name].get();
            }
            m->lock();
            break;
        }
        case OpCode::Unlock: {
            std::mutex* m = nullptr;
            {
                std::lock_guard<std::mutex> guard(locksMutex);
                auto it = namedLocks.find(c.name);
                if (it == namedLocks.end()) break;
                m = it->second.get();
            }
            m->unlock();
            break;
        }

        case OpCode::Jmp: ip = (size_t)c.target; continue;
        case OpCode::Jz:  if (popNum() == 0.0) { ip = (size_t)c.target; continue; } break;
        case OpCode::Pop: if (!stack.empty()) stack.pop_back(); break;
        case OpCode::Halt:
            syncAllThreads(); // ждём все потоки перед выходом
            ip = program.size();
            continue;
        }
        ++ip;
    }
    return stack.empty() ? 0.0 : stack.back().getNum();
}

// Остальные глобальные функции (isExpression, compileToCommands, evaluate,
// runAndGetVars, getArrayVar, getOutput) — без изменений.
// ============================================================
//  ГЛОБАЛЬНЫЕ ФУНКЦИИ
// ============================================================
bool isExpression(const std::string& input) {
    try { Compiler c(input); (void)c.compile(); return true; }
    catch (...) { return false; }
}

std::vector<Command> compileToCommands(const std::string& input) {
    Compiler c(input); return c.compile();
}

double evaluate(const std::string& input) {
    Compiler c(input); auto p = c.compile(); VM vm; return vm.run(p);
}

std::unordered_map<std::string, double> runAndGetVars(const std::string& input) {
    Compiler c(input); auto p = c.compile(); VM vm; vm.run(p);
    std::unordered_map<std::string, double> result;
    for (auto& [k, v] : vm.vars) {
        if (!v.isArray()) result[k] = v.getNum();
    }
    return result;
}

std::vector<double> getArrayVar(const std::string& input, const std::string& varName) {
    Compiler c(input); auto p = c.compile(); VM vm; vm.run(p);
    auto it = vm.vars.find(varName);
    if (it != vm.vars.end() && it->second.isArray()) return *(it->second.getArr());
    return {};
}

std::string getOutput(const std::string& input) {
    Compiler c(input);
    auto p = c.compile();
    VM vm;
    vm.run(p);
    return vm.output;
}