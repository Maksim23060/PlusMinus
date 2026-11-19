#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

enum class OpCode {
    PushConst, PushVar, Store,
    MakeArray, MakeArraySized, LoadIndex, StoreIndex,
    Add, Sub, Mul, Div, Mod, Pow,
    Less, Greater, And,
    Call,
    FuncDef, ProcDef, Return,
    GoCall, Sync,
    Lock, Unlock,
    Jmp, Jz, Pop, Halt
};

struct Value {
    double num = 0.0;
    std::shared_ptr<std::vector<double>> arr;
    std::string str;
    bool isStr = false;

    bool isArray() const { return arr != nullptr; }
    bool isString() const { return isStr; }

    Value() : num(0.0) {}
    Value(double v) : num(v) {}
    Value(std::shared_ptr<std::vector<double>> a) : arr(a), num(0.0) {}
    Value(const std::string& s) : str(s), isStr(true) {}

    double getNum() const { return num; }
    std::shared_ptr<std::vector<double>> getArr() const { return arr; }
};

struct Command {
    OpCode op;
    double num = 0.0;
    std::string name;
    std::string str;
    bool isStr = false;
    int argc = 0;
    int target = -1;
    std::vector<std::string> params;
};

class Compiler {
public:
    Compiler(std::string_view input);
    std::vector<Command> compile();

private:
    std::string_view src;
    size_t pos = 0;
    std::vector<Command> program;

    bool atEnd() const;
    char peek() const;
    char peek2() const;
    void advance();
    void skipWs();
    bool eatAnd();
    void expect(char c);
    bool checkKeyword(const std::string& kw);
    void consumeKeyword(const std::string& kw);
    void emit(Command c);
    void emit(OpCode op);

    void parseProgram();
    void parseStatement();
    void parseBlock();
    void parseIf();
    void parseWhile();
    void parseFunctionDef(bool isProc);
    void parseExpr();
    void parseCompare();
    void parseSum();
    void parseTerm();
    void parseFactor();
    void parseOperand();
    double parseNumber();
    std::string parseIdentifier();
};

class VM {
public:
    std::unordered_map<std::string, Value> vars;
    std::string output;

    struct FuncInfo {
        std::vector<std::string> params;
        size_t bodyStart;
    };
    struct Frame {
        std::unordered_map<std::string, Value> locals;
        size_t returnIp;
    };
    struct GoTask {
        std::string name;
        std::vector<Value> args;
    };

    std::unordered_map<std::string, FuncInfo> functions;
    std::vector<Frame> localStack;
    std::vector<GoTask> goTasks;
    std::unordered_map<std::string, std::unique_ptr<std::mutex>> namedLocks;

    double run(const std::vector<Command>& program);
};

bool isExpression(const std::string& input);
std::vector<Command> compileToCommands(const std::string& input);
double evaluate(const std::string& input);
std::unordered_map<std::string, double> runAndGetVars(const std::string& input);
std::vector<double> getArrayVar(const std::string& input, const std::string& varName);
std::string getOutput(const std::string& input);