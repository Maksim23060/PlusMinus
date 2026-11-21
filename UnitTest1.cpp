#include "CppUnitTest.h"
#include "Parser.h"
#include <chrono>
#include <cmath>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ParserTests
{
    // ============================================================
    //  АРИФМЕТИКА: каждый оператор отдельно
    // ============================================================
    TEST_CLASS(Arithmetic)
    {
    public:
        TEST_METHOD(Add) { Assert::AreEqual(7.0, evaluate("3 + 4")); }
        TEST_METHOD(Sub) { Assert::AreEqual(-1.0, evaluate("3 - 4")); }
        TEST_METHOD(Mul) { Assert::AreEqual(12.0, evaluate("3 * 4")); }
        TEST_METHOD(Div) { Assert::AreEqual(2.5, evaluate("10 / 4")); }
        TEST_METHOD(Mod) { Assert::AreEqual(2.0, evaluate("10 % 4")); }
        TEST_METHOD(Pow) { Assert::AreEqual(81.0, evaluate("3 ^ 4")); }

        TEST_METHOD(Zero) { Assert::AreEqual(0.0, evaluate("0")); }
        TEST_METHOD(Neg) { Assert::AreEqual(-5.0, evaluate("0 - 5")); }
        TEST_METHOD(Float) { Assert::AreEqual(3.5, evaluate("1.5 + 2.0")); }
    };

    // ============================================================
    //  ПРИОРИТЕТЫ ОПЕРАЦИЙ
    // ============================================================
    TEST_CLASS(Precedence)
    {
    public:
        TEST_METHOD(MulOverAdd) { Assert::AreEqual(7.0, evaluate("1 + 2 * 3")); }
        TEST_METHOD(MulOverSub) { Assert::AreEqual(-5.0, evaluate("1 - 2 * 3")); }
        TEST_METHOD(DivOverAdd) { Assert::AreEqual(4.0, evaluate("1 + 6 / 2")); }
        TEST_METHOD(PowOverMul) { Assert::AreEqual(18.0, evaluate("2 * 3 ^ 2")); }
        TEST_METHOD(PowOverAdd) { Assert::AreEqual(10.0, evaluate("1 + 3 ^ 2")); }
        TEST_METHOD(PowOverDiv) { Assert::AreEqual(4.0, evaluate("8 / 2 ^ 1")); }

        TEST_METHOD(Mixed) { Assert::AreEqual(50.0, evaluate("2 + 3 * 4 ^ 2")); }
        TEST_METHOD(Mixed2) { Assert::AreEqual(26.0, evaluate("2 * 3 + 4 * 5")); }
        TEST_METHOD(Mixed3) { Assert::AreEqual(12.0, evaluate("2 + 4 * 10 / 4")); }

        TEST_METHOD(ParensChangeOrder) { Assert::AreEqual(9.0, evaluate("(1 + 2) * 3")); }
        TEST_METHOD(ParensChangeOrder2) { Assert::AreEqual(1.0, evaluate("(1 + 2) % 2")); }
        TEST_METHOD(DeepParens) { Assert::AreEqual(14.0, evaluate("((1 + 2) * (3 + 1) + 2)")); }
    };

    // ============================================================
    //  АССОЦИАТИВНОСТЬ
    // ============================================================
    TEST_CLASS(Associativity)
    {
    public:
        // Левая ассоциативность: (a-b)-c
        TEST_METHOD(SubLeft) { Assert::AreEqual(5.0, evaluate("10 - 3 - 2")); }
        TEST_METHOD(DivLeft) { Assert::AreEqual(5.0, evaluate("100 / 10 / 2")); }
        TEST_METHOD(ModLeft) { Assert::AreEqual(0.0, evaluate("100 % 7 % 2")); }
        TEST_METHOD(AddLeft) { Assert::AreEqual(6.0, evaluate("1 + 2 + 3")); }
        TEST_METHOD(MulLeft) { Assert::AreEqual(24.0, evaluate("2 * 3 * 4")); }
        TEST_METHOD(MixedLeft) { Assert::AreEqual(6.0, evaluate("10 - 2 - 2")); }

        // Правая ассоциативность: a^(b^c)
        TEST_METHOD(PowRight) { Assert::AreEqual(512.0, evaluate("2 ^ 3 ^ 2")); }
        TEST_METHOD(PowRight2) { Assert::AreEqual(81.0, evaluate("3 ^ 2 ^ 2")); } // 3^4
        TEST_METHOD(PowExplicit) { Assert::AreEqual(64.0, evaluate("(2 ^ 3) ^ 2")); }
        TEST_METHOD(PowRightBig) { Assert::AreEqual(65536.0, evaluate("2 ^ 2 ^ 2 ^ 2")); } // 2^(2^(2^2)) = 2^16

        // Скобки меняют ассоциативность
        TEST_METHOD(ParensSub) { Assert::AreEqual(9.0, evaluate("10 - (3 - 2)")); }
        TEST_METHOD(ParensDiv) { Assert::AreEqual(20.0, evaluate("100 / (10 / 2)")); }
    };

    // ============================================================
    //  ОПЕРАТОРЫ СРАВНЕНИЯ
    // ============================================================
    TEST_CLASS(Comparison)
    {
    public:
        TEST_METHOD(LessTrue) { Assert::AreEqual(1.0, evaluate("1 < 2")); }
        TEST_METHOD(LessFalse) { Assert::AreEqual(0.0, evaluate("2 < 1")); }
        TEST_METHOD(LessEqual) { Assert::AreEqual(0.0, evaluate("2 < 2")); }
        TEST_METHOD(GreaterTrue) { Assert::AreEqual(1.0, evaluate("3 > 2")); }
        TEST_METHOD(GreaterFalse) { Assert::AreEqual(0.0, evaluate("1 > 2")); }
        TEST_METHOD(GreaterEq) { Assert::AreEqual(0.0, evaluate("2 > 2")); }

        TEST_METHOD(CompareExpr) { Assert::AreEqual(1.0, evaluate("1 + 1 < 1 + 2")); }
        TEST_METHOD(CompareExpr2) { Assert::AreEqual(0.0, evaluate("5 * 2 > 3 * 5")); }
        TEST_METHOD(CompareChain) { Assert::AreEqual(1.0, evaluate("1 < 2 < 3")); } // (1<2)<3 = 1<3 = 1
        TEST_METHOD(CompareChain2) { Assert::AreEqual(0.0, evaluate("3 > 2 > 1")); } // (3>2)>1 = 1>1 = 0
    };

    // ============================================================
    //  ЛОГИЧЕСКОЕ И (&&)
    // ============================================================
    TEST_CLASS(LogicalAnd)
    {
    public:
        TEST_METHOD(TrueTrue) { Assert::AreEqual(1.0, evaluate("1 && 1")); }
        TEST_METHOD(TrueFalse) { Assert::AreEqual(0.0, evaluate("1 && 0")); }
        TEST_METHOD(FalseTrue) { Assert::AreEqual(0.0, evaluate("0 && 1")); }
        TEST_METHOD(FalseFalse) { Assert::AreEqual(0.0, evaluate("0 && 0")); }

        TEST_METHOD(NonZero) { Assert::AreEqual(1.0, evaluate("5 && 3")); }
        TEST_METHOD(Chain) { Assert::AreEqual(1.0, evaluate("1 && 1 && 1")); }
        TEST_METHOD(ChainFalse) { Assert::AreEqual(0.0, evaluate("1 && 0 && 1")); }

        TEST_METHOD(WithComparisons) { Assert::AreEqual(1.0, evaluate("1 < 2 && 3 > 2")); }
        TEST_METHOD(WithComparisons2) { Assert::AreEqual(0.0, evaluate("1 < 2 && 2 > 3")); }

        TEST_METHOD(AndLowestPrecedence)
        {
            // 1 + 1 && 1 + 1  =>  (1+1) && (1+1)  =>  1
            Assert::AreEqual(1.0, evaluate("1 + 1 && 1 + 1"));
        }
        TEST_METHOD(AndLowerThanCompare)
        {
            // 1 < 2 && 2 < 3  =>  (1<2) && (2<3)  =>  1
            Assert::AreEqual(1.0, evaluate("1 < 2 && 2 < 3"));
        }
    };

    // ============================================================
    //  IF — базовые
    // ============================================================
    TEST_CLASS(IfBasic)
    {
    public:
        TEST_METHOD(TrueBlock) { auto v = runAndGetVars("x = 0; if (1) { x = 1; }"); Assert::AreEqual(1.0, v["x"]); }
        TEST_METHOD(FalseBlock) { auto v = runAndGetVars("x = 0; if (0) { x = 1; }"); Assert::AreEqual(0.0, v["x"]); }
        TEST_METHOD(EmptyTrue) { auto v = runAndGetVars("x = 5; if (1) {}");          Assert::AreEqual(5.0, v["x"]); }
        TEST_METHOD(EmptyFalse) { auto v = runAndGetVars("x = 5; if (0) {}");          Assert::AreEqual(5.0, v["x"]); }

        TEST_METHOD(VarCondition) { auto v = runAndGetVars("c = 1; x = 0; if (c) { x = 7; }"); Assert::AreEqual(7.0, v["x"]); }
        TEST_METHOD(VarCondition0) { auto v = runAndGetVars("c = 0; x = 0; if (c) { x = 7; }"); Assert::AreEqual(0.0, v["x"]); }

        TEST_METHOD(ComplexCondition) { auto v = runAndGetVars("x = 0; if (2 + 3 > 4) { x = 1; }"); Assert::AreEqual(1.0, v["x"]); }
        TEST_METHOD(AndCondition) { auto v = runAndGetVars("x = 0; if (1 < 2 && 3 > 2) { x = 1; }"); Assert::AreEqual(1.0, v["x"]); }
        TEST_METHOD(AndCondition2) { auto v = runAndGetVars("x = 0; if (1 < 2 && 2 > 3) { x = 1; }"); Assert::AreEqual(0.0, v["x"]); }
    };

    // ============================================================
    //  IF — с else
    // ============================================================
    TEST_CLASS(IfElse)
    {
    public:
        TEST_METHOD(TrueBranch)
        {
            auto v = runAndGetVars("x = 0; if (1) { x = 1; } else { x = 2; }");
            Assert::AreEqual(1.0, v["x"]);
        }
        TEST_METHOD(FalseBranch)
        {
            auto v = runAndGetVars("x = 0; if (0) { x = 1; } else { x = 2; }");
            Assert::AreEqual(2.0, v["x"]);
        }
        TEST_METHOD(ElseNotTaken_True)
        {
            auto v = runAndGetVars("a = 0; b = 0; if (1) { a = 1; } else { b = 1; }");
            Assert::AreEqual(1.0, v["a"]);
            Assert::AreEqual(0.0, v["b"]);
        }
        TEST_METHOD(ElseNotTaken_False)
        {
            auto v = runAndGetVars("a = 0; b = 0; if (0) { a = 1; } else { b = 1; }");
            Assert::AreEqual(0.0, v["a"]);
            Assert::AreEqual(1.0, v["b"]);
        }
    };

    // ============================================================
    //  IF — вложенные и else-if
    // ============================================================
    TEST_CLASS(IfNested)
    {
    public:
        TEST_METHOD(IfInIf_TrueTrue)
        {
            auto v = runAndGetVars("x = 0; if (1) { if (1) { x = 5; } }");
            Assert::AreEqual(5.0, v["x"]);
        }
        TEST_METHOD(IfInIf_TrueFalse)
        {
            auto v = runAndGetVars("x = 0; if (1) { if (0) { x = 5; } }");
            Assert::AreEqual(0.0, v["x"]);
        }
        TEST_METHOD(IfInIf_FalseTrue)
        {
            auto v = runAndGetVars("x = 0; if (0) { if (1) { x = 5; } }");
            Assert::AreEqual(0.0, v["x"]);
        }

        TEST_METHOD(ElseIf_First)
        {
            auto v = runAndGetVars(
                "x = 0;"
                "if (1) { x = 1; }"
                "else { if (1) { x = 2; } }");
            Assert::AreEqual(1.0, v["x"]);
        }
        TEST_METHOD(ElseIf_Second)
        {
            auto v = runAndGetVars(
                "x = 0;"
                "if (0) { x = 1; }"
                "else { if (1) { x = 2; } }");
            Assert::AreEqual(2.0, v["x"]);
        }

        TEST_METHOD(TripleNested)
        {
            auto v = runAndGetVars(
                "x = 0;"
                "if (1) {"
                "  if (1) {"
                "    if (1) { x = 9; }"
                "  }"
                "}");
            Assert::AreEqual(9.0, v["x"]);
        }
    };

    // ============================================================
    //  WHILE — базовые
    // ============================================================
    TEST_CLASS(WhileBasic)
    {
    public:
        TEST_METHOD(ZeroIterations)
        {
            auto v = runAndGetVars("x = 42; while (0) { x = 1; }");
            Assert::AreEqual(42.0, v["x"]);
        }
        TEST_METHOD(SingleIteration)
        {
            auto v = runAndGetVars("i = 1; x = 0; while (i) { x = 1; i = 0; }");
            Assert::AreEqual(1.0, v["x"]);
        }
        TEST_METHOD(CountUp)
        {
            auto v = runAndGetVars("i = 0; while (i < 5) { i = i + 1; }");
            Assert::AreEqual(5.0, v["i"]);
        }
        TEST_METHOD(CountDown)
        {
            auto v = runAndGetVars("i = 10; while (i > 0) { i = i - 1; }");
            Assert::AreEqual(0.0, v["i"]);
        }
        TEST_METHOD(Sum)
        {
            auto v = runAndGetVars(
                "i = 1; sum = 0;"
                "while (i < 6) {"
                "  sum = sum + i;"
                "  i = i + 1;"
                "}");
            Assert::AreEqual(15.0, v["sum"]);   // 1+2+3+4+5
        }
        TEST_METHOD(Product)
        {
            auto v = runAndGetVars(
                "i = 1; p = 1;"
                "while (i < 5) {"
                "  p = p * i;"
                "  i = i + 1;"
                "}");
            Assert::AreEqual(24.0, v["p"]);     // 1*2*3*4
        }
    };

    // ============================================================
    //  WHILE — сложные условия
    // ============================================================
    TEST_CLASS(WhileConditions)
    {
    public:
        TEST_METHOD(WithAnd)
        {
            auto v = runAndGetVars(
                "i = 0; s = 0;"
                "while (i < 10 && s < 6) {"
                "  s = s + i;"
                "  i = i + 1;"
                "}");
            // s: 0,1,3,6 — стоп, s=6
            Assert::AreEqual(6.0, v["s"]);
        }
        TEST_METHOD(WithComparisonInBody)
        {
            auto v = runAndGetVars(
                "i = 0; c = 0;"
                "while (i < 10) {"
                "  if (i > 5) { c = c + 1; }"
                "  i = i + 1;"
                "}");
            Assert::AreEqual(4.0, v["c"]);      // 6,7,8,9
        }
        TEST_METHOD(Factorial)
        {
            auto v = runAndGetVars(
                "n = 5; f = 1;"
                "while (n > 1) {"
                "  f = f * n;"
                "  n = n - 1;"
                "}");
            Assert::AreEqual(120.0, v["f"]);
        }
        TEST_METHOD(CountEven)
        {
            auto v = runAndGetVars(
                "i = 0; c = 0;"
                "while (i < 20) {"
                "  if (i % 2 < 1) { c = c + 1; }"
                "  i = i + 1;"
                "}");
            Assert::AreEqual(10.0, v["c"]);
        }
    };

    // ============================================================
    //  WHILE — вложенные
    // ============================================================
    TEST_CLASS(WhileNested)
    {
    public:
        TEST_METHOD(TwoLoops)
        {
            auto v = runAndGetVars(
                "i = 0; t = 0;"
                "while (i < 3) {"
                "  j = 0;"
                "  while (j < 3) {"
                "    t = t + 1;"
                "    j = j + 1;"
                "  }"
                "  i = i + 1;"
                "}");
            Assert::AreEqual(9.0, v["t"]);
        }
        TEST_METHOD(Triangle)
        {
            auto v = runAndGetVars(
                "i = 0; t = 0;"
                "while (i < 4) {"
                "  j = 0;"
                "  while (j < i) {"
                "    t = t + 1;"
                "    j = j + 1;"
                "  }"
                "  i = i + 1;"
                "}");
            // 0+1+2+3 = 6
            Assert::AreEqual(6.0, v["t"]);
        }
    };

    // ============================================================
    //  WHILE + IF комбинации
    // ============================================================
    TEST_CLASS(WhileWithIf)
    {
    public:
        TEST_METHOD(SumOdd)
        {
            auto v = runAndGetVars(
                "i = 0; s = 0;"
                "while (i < 10) {"
                "  if (i % 2 > 0) { s = s + i; }"
                "  i = i + 1;"
                "}");
            Assert::AreEqual(25.0, v["s"]);     // 1+3+5+7+9
        }
        TEST_METHOD(SumEven)
        {
            auto v = runAndGetVars(
                "i = 0; s = 0;"
                "while (i < 10) {"
                "  if (i % 2 < 1) { s = s + i; }"
                "  i = i + 1;"
                "}");
            Assert::AreEqual(20.0, v["s"]);     // 0+2+4+6+8
        }
        TEST_METHOD(FirstGreater)
        {
            auto v = runAndGetVars(
                "i = 0; found = 0;"
                "while (i < 20 && found < 1) {"
                "  if (i * i > 50) { found = i; }"
                "  i = i + 1;"
                "}");
            // 7^2 = 49 < 50, 8^2 = 64 > 50 — первое i = 8
            Assert::AreEqual(8.0, v["found"]);
        }
        TEST_METHOD(IfElseInsideWhile)
        {
            auto v = runAndGetVars(
                "i = 0; a = 0; b = 0;"
                "while (i < 10) {"
                "  if (i < 5) { a = a + 1; }"
                "  else { b = b + 1; }"
                "  i = i + 1;"
                "}");
            Assert::AreEqual(5.0, v["a"]);
            Assert::AreEqual(5.0, v["b"]);
        }
    };

    // ============================================================
    //  Полноценные программы
    // ============================================================
    TEST_CLASS(Programs)
    {
    public:
        TEST_METHOD(Fibonacci10)
        {
            auto v = runAndGetVars(
                "a = 0; b = 1; n = 0;"
                "while (n < 10) {"
                "  t = a + b;"
                "  a = b;"
                "  b = t;"
                "  n = n + 1;"
                "}");
            Assert::AreEqual(55.0, v["a"]);
            Assert::AreEqual(89.0, v["b"]);
        }

        TEST_METHOD(Gcd)
        {
            // НОД(48, 18) = 6  (алгоритм Евклида)
            auto v = runAndGetVars(
                "a = 48; b = 18;"
                "while (b > 0) {"
                "  t = a % b;"
                "  a = b;"
                "  b = t;"
                "}");
            Assert::AreEqual(6.0, v["a"]);
        }

        TEST_METHOD(PowerOfTwo)
        {
            // 2^10 = 1024
            auto v = runAndGetVars(
                "p = 1; n = 0;"
                "while (n < 10) {"
                "  p = p * 2;"
                "  n = n + 1;"
                "}");
            Assert::AreEqual(1024.0, v["p"]);
        }

        TEST_METHOD(CollatzSteps)
        {
            // Число шагов Коллатца для 27 — длинная история, но работает
            // Возьмём 6: 6 → 3 → 10 → 5 → 16 → 8 → 4 → 2 → 1  (8 шагов)
            auto v = runAndGetVars(
                "n = 6; steps = 0;"
                "while (n > 1) {"
                "  if (n % 2 < 1) { n = n / 2; }"
                "  else { n = 3 * n + 1; }"
                "  steps = steps + 1;"
                "}");
            Assert::AreEqual(1.0, v["n"]);
            Assert::AreEqual(8.0, v["steps"]);
        }

        TEST_METHOD(MaxInSequence)
        {
            auto v = runAndGetVars(
                "i = 0; max = 0;"
                "while (i < 10) {"
                "  v = (i * 7) % 10;"     // псевдослучайные 0..9
                "  if (v > max) { max = v; }"
                "  i = i + 1;"
                "}");
            Assert::IsTrue(v["max"] > 0.0);
        }
    };

    // ============================================================
    //  Генерация байт-кода для операторов
    // ============================================================
    TEST_CLASS(CodeGenOps)
    {
    public:
        TEST_METHOD(Add) { auto p = compileToCommands("1 + 2"); Assert::IsTrue(p[2].op == OpCode::Add); }
        TEST_METHOD(Sub) { auto p = compileToCommands("1 - 2"); Assert::IsTrue(p[2].op == OpCode::Sub); }
        TEST_METHOD(Mul) { auto p = compileToCommands("1 * 2"); Assert::IsTrue(p[2].op == OpCode::Mul); }
        TEST_METHOD(Div) { auto p = compileToCommands("1 / 2"); Assert::IsTrue(p[2].op == OpCode::Div); }
        TEST_METHOD(Mod) { auto p = compileToCommands("1 % 2"); Assert::IsTrue(p[2].op == OpCode::Mod); }
        TEST_METHOD(Pow) { auto p = compileToCommands("1 ^ 2"); Assert::IsTrue(p[2].op == OpCode::Pow); }
        TEST_METHOD(Less) { auto p = compileToCommands("1 < 2"); Assert::IsTrue(p[2].op == OpCode::Less); }
        TEST_METHOD(Greater) { auto p = compileToCommands("1 > 2"); Assert::IsTrue(p[2].op == OpCode::Greater); }
        TEST_METHOD(And) { auto p = compileToCommands("1 && 2"); Assert::IsTrue(p[2].op == OpCode::And); }
    };

    // ============================================================
    //  Генерация байт-кода для if / while
    // ============================================================
    TEST_CLASS(CodeGenControlFlow)
    {
    public:
        TEST_METHOD(IfEmitsJz)
        {
            auto p = compileToCommands("if (1) { x = 5; }");
            bool hasJz = false;
            for (auto& c : p) if (c.op == OpCode::Jz) hasJz = true;
            Assert::IsTrue(hasJz);
        }

        TEST_METHOD(IfNoElse_HasNoJmp)
        {
            auto p = compileToCommands("if (1) { x = 5; }");
            bool hasJmp = false;
            for (auto& c : p) if (c.op == OpCode::Jmp) hasJmp = true;
            Assert::IsFalse(hasJmp);
        }

        TEST_METHOD(IfElse_HasJzAndJmp)
        {
            auto p = compileToCommands("if (1) { x = 1; } else { x = 2; }");
            bool hasJz = false, hasJmp = false;
            for (auto& c : p) {
                if (c.op == OpCode::Jz)  hasJz = true;
                if (c.op == OpCode::Jmp) hasJmp = true;
            }
            Assert::IsTrue(hasJz);
            Assert::IsTrue(hasJmp);
        }

        TEST_METHOD(While_HasJzAndJmp)
        {
            auto p = compileToCommands("while (i < 5) { i = i + 1; }");
            bool hasJz = false, hasJmp = false;
            for (auto& c : p) {
                if (c.op == OpCode::Jz)  hasJz = true;
                if (c.op == OpCode::Jmp) hasJmp = true;
            }
            Assert::IsTrue(hasJz);
            Assert::IsTrue(hasJmp);
        }

        TEST_METHOD(While_JmpTargetsStart)
        {
            auto p = compileToCommands("while (i < 5) { i = i + 1; }");
            int jmpTarget = -1;
            for (auto& c : p) if (c.op == OpCode::Jmp) jmpTarget = c.target;
            Assert::AreEqual(0, jmpTarget);  // цикл начинается с нулевой команды
        }

        TEST_METHOD(AssignmentEmitsStore)
        {
            auto p = compileToCommands("x = 5;");
            bool hasStore = false;
            for (auto& c : p) if (c.op == OpCode::Store) { hasStore = true; Assert::AreEqual(std::string("x"), c.name); }
            Assert::IsTrue(hasStore);
        }

        TEST_METHOD(HaltIsLast)
        {
            auto p = compileToCommands("x = 5;");
            Assert::IsTrue(p.back().op == OpCode::Halt);
        }
    };

    // ============================================================
    //  Массивы: объявление и инициализация
    // ============================================================
    TEST_CLASS(ArraysInit)
    {
    public:
        TEST_METHOD(ArrayWithSize)
        {
            auto arr = getArrayVar("int a[5];", "a");
            Assert::AreEqual<size_t>(5, arr.size());
            for (double v : arr) Assert::AreEqual(0.0, v);
        }

        TEST_METHOD(ArrayWithValues)
        {
            auto arr = getArrayVar("int b = [7, 5, 2];", "b");
            Assert::AreEqual<size_t>(3, arr.size());
            Assert::AreEqual(7.0, arr[0]);
            Assert::AreEqual(5.0, arr[1]);
            Assert::AreEqual(2.0, arr[2]);
        }

        TEST_METHOD(SingleElementArray)
        {
            auto arr = getArrayVar("int d = [5];", "d");
            Assert::AreEqual<size_t>(1, arr.size());
            Assert::AreEqual(5.0, arr[0]);
        }

        TEST_METHOD(ResizeArray)
        {
            auto arr = getArrayVar("int b = [1, 2]; resize(b, 5);", "b");
            Assert::AreEqual<size_t>(5, arr.size());
            Assert::AreEqual(1.0, arr[0]);
            Assert::AreEqual(0.0, arr[2]); // новые элементы заполняются нулями
        }
    };

    // ============================================================
    //  Массивы: чтение и запись
    // ============================================================
    TEST_CLASS(ArraysAccess)
    {
    public:
        TEST_METHOD(ReadElement)
        {
            auto v = runAndGetVars("int b = [7, 5, 2]; int x = b[1];");
            Assert::AreEqual(5.0, v["x"]);
        }

        TEST_METHOD(WriteElement)
        {
            auto arr = getArrayVar("int b = [7, 5, 2]; b[1] = 99;", "b");
            Assert::AreEqual(7.0, arr[0]);
            Assert::AreEqual(99.0, arr[1]);
            Assert::AreEqual(2.0, arr[2]);
        }

        TEST_METHOD(WriteWithExpressionIndex)
        {
            auto arr = getArrayVar("int a = [0, 0, 0]; int i = 2; a[i - 1] = 42;", "a");
            Assert::AreEqual(0.0, arr[0]);
            Assert::AreEqual(42.0, arr[1]);
            Assert::AreEqual(0.0, arr[2]);
        }

        TEST_METHOD(ReadOutOfBounds_Throws)
        {
            bool threw = false;
            try {
                // evaluate запускает и компилятор, и виртуальную машину
                evaluate("int a = [1]; int x = a[5];");
            }
            catch (...) {
                threw = true;
            }
            Assert::IsTrue(threw, L"Expected an out-of-bounds exception");
        }
    };

    // ============================================================
    //  Массивы: вложенные индексы (пример с доски)
    // ============================================================
    TEST_CLASS(ComplexArrays)
    {
    public:
        TEST_METHOD(WhiteboardExample)
        {
            // b = [10, 20, 30, 40, 50, 60]
            // i = 0
            // x = b[b[i+1]+4] => i+1 = 1 => b[1] = 20 => 20+4 = 24 => b[24] - ошибка!
            // Изменим данные, чтобы не выходить за пределы:
            // b = [4, 2, 7, 5, 9] (размер 6)
            // i = 0. b[i+1] => b[1] = 2. 2+4 = 6 => b[6] - опять выход.
            // Сделаем b подлиннее: b = [1, 1, 1, 1, 1, 10]
            // i = 0. b[i+1] => b[1] = 1. 1+4 = 5 => b[5] = 10.
            std::string prog =
                "int b = [1, 1, 1, 1, 1, 10];"
                "int i = 0;"
                "int x = b[b[i+1]+4];";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(10.0, v["x"]);
        }

        TEST_METHOD(ArrayInLoop)
        {
            // Заполняем массив квадратами чисел
            std::string prog =
                "int arr[5];"
                "int i = 0;"
                "while (i < 5) {"
                "  arr[i] = i * i;"
                "  i = i + 1;"
                "}";
            auto arr = getArrayVar(prog, "arr");
            Assert::AreEqual(0.0, arr[0]);
            Assert::AreEqual(1.0, arr[1]);
            Assert::AreEqual(4.0, arr[2]);
            Assert::AreEqual(9.0, arr[3]);
            Assert::AreEqual(16.0, arr[4]);
        }
    };

    // ============================================================
    //  Массивы: встроенная функция len()
    // ============================================================
    TEST_CLASS(ArraysLen)
    {
    public:
        TEST_METHOD(LenOnInitializedArray)
        {
            auto v = runAndGetVars("int a = [10, 20, 30, 40]; int x = len(a);");
            Assert::AreEqual(4.0, v["x"]);
        }

        TEST_METHOD(LenOnSizedArray)
        {
            auto v = runAndGetVars("int a[100]; int x = len(a);");
            Assert::AreEqual(100.0, v["x"]);
        }

        TEST_METHOD(LenAfterResize)
        {
            auto v = runAndGetVars("int a = [1, 2]; resize(a, 50); int x = len(a);");
            Assert::AreEqual(50.0, v["x"]);
        }

        TEST_METHOD(LenInExpression)
        {
            auto v = runAndGetVars("int a = [1, 2, 3]; int x = len(a) * 2 + 1;");
            Assert::AreEqual(7.0, v["x"]);
        }

        TEST_METHOD(LenInWhileCondition)
        {
            std::string prog =
                "int a[5];"
                "int i = 0;"
                "while (i < len(a)) {"
                "  i = i + 1;"
                "}";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(5.0, v["i"]);
        }

        TEST_METHOD(LenOnPassedArray)
        {
            auto v = runAndGetVars(
                "int myArr = [7, 8, 9];"
                "int size = len(myArr);"
                "int last = myArr[size - 1];");
            Assert::AreEqual(3.0, v["size"]);
            Assert::AreEqual(9.0, v["last"]);
        }

        TEST_METHOD(LenOnNonArray_Throws)
        {
            bool threw = false;
            try {
                evaluate("int num = 42; int x = len(num);");
            }
            catch (...) {
                threw = true;
            }
            Assert::IsTrue(threw, L"Expected exception when calling len() on a number");
        }
    };

    // ============================================================
//  Динамическое изменение размера массивов
// ============================================================
    TEST_CLASS(ArrayDynamicResize)
    {
    public:
        // ----- УВЕЛИЧЕНИЕ (Рост) -----

        TEST_METHOD(GrowDoubleSize)
        {
            // Увеличиваем размер в 2 раза
            auto arr = getArrayVar("int a = [1, 2, 3]; resize(a, len(a) * 2);", "a");
            Assert::AreEqual<size_t>(6, arr.size());
            Assert::AreEqual(1.0, arr[0]);
            Assert::AreEqual(0.0, arr[3]); // новые элементы заполнены нулями
        }

        TEST_METHOD(GrowByConstant)
        {
            // Прибавляем к размеру константу
            auto arr = getArrayVar("int a[2]; resize(a, len(a) + 8);", "a");
            Assert::AreEqual<size_t>(10, arr.size());
        }

        TEST_METHOD(GrowLoop)
        {
            // Удваиваем размер в цикле 3 раза: 2 -> 4 -> 8 -> 16
            auto arr = getArrayVar(
                "int a[2];"
                "int i = 0;"
                "while (i < 3) {"
                "  resize(a, len(a) * 2);"
                "  i = i + 1;"
                "}",
                "a");
            Assert::AreEqual<size_t>(16, arr.size());
        }

        TEST_METHOD(GrowByFormula)
        {
            // Формула: len(a) * len(a) + 1 (3*3+1 = 10)
            auto arr = getArrayVar("int a = [1, 2, 3]; resize(a, len(a) * len(a) + 1);", "a");
            Assert::AreEqual<size_t>(10, arr.size());
        }


        // ----- УМЕНЬШЕНИЕ (Сжатие) -----

        TEST_METHOD(ShrinkHalfSize)
        {
            // Уменьшаем размер в 2 раза
            auto arr = getArrayVar("int a = [10, 20, 30, 40, 50, 60]; resize(a, len(a) / 2);", "a");
            Assert::AreEqual<size_t>(3, arr.size());
            Assert::AreEqual(10.0, arr[0]); // первые элементы сохраняются
            Assert::AreEqual(30.0, arr[2]);
        }

        TEST_METHOD(ShrinkByConstant)
        {
            // Отнимаем от размера константу
            auto arr = getArrayVar("int a[10]; resize(a, len(a) - 7);", "a");
            Assert::AreEqual<size_t>(3, arr.size());
        }

        TEST_METHOD(ShrinkToZero)
        {
            // Сжимаем в ноль
            auto arr = getArrayVar("int a[100]; resize(a, len(a) - len(a));", "a");
            Assert::AreEqual<size_t>(0, arr.size());
        }

        TEST_METHOD(ShrinkLoop)
        {
            // Уменьшаем размер в цикле 3 раза: 16 -> 8 -> 4 -> 2
            auto arr = getArrayVar(
                "int a[16];"
                "int i = 0;"
                "while (i < 3) {"
                "  resize(a, len(a) / 2);"
                "  i = i + 1;"
                "}",
                "a");
            Assert::AreEqual<size_t>(2, arr.size());
        }


        // ----- СМЕШАННЫЕ СЦЕНАРИИ (Рост и Сжатие) -----

        TEST_METHOD(GrowThenShrink)
        {
            // 5 -> +5 -> 10 -> /2 -> 5
            auto arr = getArrayVar(
                "int a = [1, 2, 3, 4, 5];"
                "resize(a, len(a) + 5);"
                "resize(a, len(a) / 2);",
                "a");
            Assert::AreEqual<size_t>(5, arr.size());
            Assert::AreEqual(5.0, arr[4]); // данные не потерялись
        }

        TEST_METHOD(ShrinkThenGrow)
        {
            // 5 -> -2 -> 3 -> *3 -> 9
            auto arr = getArrayVar(
                "int a = [10, 20, 30, 40, 50];"
                "resize(a, len(a) - 2);" // стало 3
                "resize(a, len(a) * 3);", // стало 9
                "a");
            Assert::AreEqual<size_t>(9, arr.size());
            Assert::AreEqual(10.0, arr[0]);
            Assert::AreEqual(30.0, arr[2]);
            Assert::AreEqual(0.0, arr[5]); // новые элементы — нули
        }

        TEST_METHOD(ChainResize)
        {
            // Сложная цепочка: 1 -> *10 -> 10 -> +10 -> 20 -> /4 -> 5
            auto arr = getArrayVar(
                "int a = [1];"
                "resize(a, len(a) * 10);"
                "resize(a, len(a) + 10);"
                "resize(a, len(a) / 4);",
                "a");
            Assert::AreEqual<size_t>(5, arr.size());
            Assert::AreEqual(1.0, arr[0]);
            Assert::AreEqual(0.0, arr[1]);
        }


        // ----- ПРОВЕРКА ОШИБОК И ГРАНИЦ -----

        TEST_METHOD(ShrinkOutOfBoundsAccess_Throws)
        {
            // После сжатия доступ к старым индексам должен вызывать ошибку
            bool threw = false;
            try {
                // Размер был 5, стал 2. Пытаемся прочитать a[4]
                evaluate(
                    "int a[5];"
                    "resize(a, len(a) - 3);"
                    "int x = a[4];"
                );
            }
            catch (...) {
                threw = true;
            }
            Assert::IsTrue(threw, L"Expected out-of-bounds exception after shrinking");
        }

        TEST_METHOD(NegativeResize_Throws)
        {
            // Попытка сделать размер отрицательным
            bool threw = false;
            try {
                evaluate("int a[2]; resize(a, len(a) - 5);");
            }
            catch (...) {
                threw = true;
            }
            Assert::IsTrue(threw, L"Expected exception on negative size");
        }
    };

// ============================================================
//  Комментарии (как в C++)
// ============================================================
    TEST_CLASS(Comments)
    {
    public:
        TEST_METHOD(SingleLineAtStart)
        {
            auto v = runAndGetVars("// This is a comment\nx = 5;");
            Assert::AreEqual(5.0, v["x"]);
        }

        TEST_METHOD(SingleLineAtEnd)
        {
            auto v = runAndGetVars("x = 5; // Comment at the end");
            Assert::AreEqual(5.0, v["x"]);
        }

        TEST_METHOD(SingleLineInsideExpression)
        {
            auto v = runAndGetVars("x = 5 + // plus\n 3;");
            Assert::AreEqual(8.0, v["x"]);
        }

        TEST_METHOD(MultiLineSimple)
        {
            auto v = runAndGetVars("/* comment */ x = 10;");
            Assert::AreEqual(10.0, v["x"]);
        }

        TEST_METHOD(MultiLineInsideExpression)
        {
            auto v = runAndGetVars("x = 5 /* mid */ + 3;");
            Assert::AreEqual(8.0, v["x"]);
        }

        TEST_METHOD(MultiLineMultiline)
        {
            std::string prog =
                "x = 1;\n"
                "/*\n"
                "   Multi-line\n"
                "   comment\n"
                "*/\n"
                "x = x + 1;";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(2.0, v["x"]);
        }

        TEST_METHOD(MixedComments)
        {
            std::string prog =
                "// start\n"
                "x = /* inline */ 5;\n"
                "/* block\n"
                "   comment */\n"
                "y = 10; // end\n";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(5.0, v["x"]);
            Assert::AreEqual(10.0, v["y"]);
        }

        TEST_METHOD(DivisionNotComment)
        {
            // Проверяем, что / всё ещё работает как деление
            auto v = runAndGetVars("x = 10 / 2;");
            Assert::AreEqual(5.0, v["x"]);
        }

        TEST_METHOD(UnclosedMultiLineComment_Throws)
        {
            bool threw = false;
            try {
                evaluate("x = 5; /* unclosed comment");
            }
            catch (...) {
                threw = true;
            }
            Assert::IsTrue(threw);
        }
    };
    // ============================================================
//  Встроенные математические функции (sin, cos)
// ============================================================
    TEST_CLASS(MathFunctions)
    {
    public:
        // ----- Синус (sin) -----
        TEST_METHOD(SinZero)
        {
            // sin(0) = 0
            Assert::AreEqual(0.0, evaluate("sin(0)"), 0.00001);
        }

        TEST_METHOD(SinPiHalf)
        {
            // sin(pi/2) = 1
            Assert::AreEqual(1.0, evaluate("sin(3.141592653589793 / 2)"), 0.00001);
        }

        TEST_METHOD(SinPi)
        {
            // sin(pi) = 0
            Assert::AreEqual(0.0, evaluate("sin(3.141592653589793)"), 0.00001);
        }

        TEST_METHOD(SinWithExpression)
        {
            // sin(pi/6) = 0.5
            Assert::AreEqual(0.5, evaluate("sin(3.141592653589793 / 6)"), 0.00001);
        }

        TEST_METHOD(SinWithVariable)
        {
            // Использование переменной в качестве аргумента
            auto v = runAndGetVars("pi = 3.141592653589793; x = sin(pi / 2);");
            Assert::AreEqual(1.0, v["x"], 0.00001);
        }

        // ----- Косинус (cos) -----
        TEST_METHOD(CosZero)
        {
            // cos(0) = 1
            Assert::AreEqual(1.0, evaluate("cos(0)"), 0.00001);
        }

        TEST_METHOD(CosPiHalf)
        {
            // cos(pi/2) = 0
            Assert::AreEqual(0.0, evaluate("cos(3.141592653589793 / 2)"), 0.00001);
        }

        TEST_METHOD(CosPi)
        {
            // cos(pi) = -1
            Assert::AreEqual(-1.0, evaluate("cos(3.141592653589793)"), 0.00001);
        }

        TEST_METHOD(CosWithExpression)
        {
            // cos(pi/3) = 0.5
            Assert::AreEqual(0.5, evaluate("cos(3.141592653589793 / 3)"), 0.00001);
        }

        // ----- Комбинации и вложенность -----
        TEST_METHOD(SinCosCombo)
        {
            // sin(pi/2) + cos(0) = 1 + 1 = 2
            Assert::AreEqual(2.0, evaluate("sin(3.141592653589793 / 2) + cos(0)"), 0.00001);
        }

        TEST_METHOD(NestedMath)
        {
            // sin(cos(0)) = sin(1) = 0.84147...
            // Используем std::sin напрямую в C++, чтобы получить точное значение
            Assert::AreEqual(std::sin(1.0), evaluate("sin(cos(0))"), 0.00001);
        }

        TEST_METHOD(MathInLoop)
        {
            // Вычисление синусов углов и сохранение в массив
            std::string prog =
                "int a[3];"
                "pi = 3.141592653589793;"
                "a[0] = sin(0);"
                "a[1] = sin(pi / 2);"
                "a[2] = sin(pi);";
            auto arr = getArrayVar(prog, "a");
            Assert::AreEqual(0.0, arr[0], 0.00001);
            Assert::AreEqual(1.0, arr[1], 0.00001);
            Assert::AreEqual(0.0, arr[2], 0.00001);
        }

        // ----- Обработка ошибок -----
        TEST_METHOD(SinWrongArgCount_Throws)
        {
            bool threw = false;
            try { evaluate("sin()"); }
            catch (...) { threw = true; }
            Assert::IsTrue(threw, L"sin() without arguments should throw");

            threw = false;
            try { evaluate("sin(1, 2)"); }
            catch (...) { threw = true; }
            Assert::IsTrue(threw, L"sin(1, 2) with 2 arguments should throw");
        }

        TEST_METHOD(SinOnArray_Throws)
        {
            bool threw = false;
            try {
                evaluate("int a = [1, 2]; x = sin(a);");
            }
            catch (...) {
                threw = true;
            }
            Assert::IsTrue(threw, L"sin() on an array should throw");
        }
    };

    //////////////////////////////////
    // Print
    //////////////////////////////////
    
    TEST_CLASS(PrintFunctionMultiArgs)
    {
    public:
        TEST_METHOD(PrintTwoNumbers)
        {
            std::string out = getOutput("print(10, 20);");
            Assert::AreEqual(std::string("10 20\n"), out);
        }

        TEST_METHOD(PrintThreeNumbers)
        {
            std::string out = getOutput("print(1, 2, 3);");
            Assert::AreEqual(std::string("1 2 3\n"), out);
        }

        TEST_METHOD(PrintMixedTypes)
        {
            std::string out = getOutput("int a = [1, 2222, 4, 11]; print(42, a, 99);");
            Assert::AreEqual(std::string("42 [1, 2222, 4, 11] 99\n"), out);
        }

        TEST_METHOD(PrintExpressions)
        {
            std::string out = getOutput("print(2 + 2, 3 * 3, 10 / 2);");
            Assert::AreEqual(std::string("4 9 5\n"), out);
        }

        TEST_METHOD(PrintNoArgs)
        {
            // Печать без аргументов просто выведет пустую строку
            std::string out = getOutput("print();");
            Assert::AreEqual(std::string("\n"), out);
        }
    };
    // ============================================================
//  Глубоко вложенные массивы и математика (матрешки)
// ============================================================
    TEST_CLASS(DeepNestedArraysAndMath)
    {
    public:
        TEST_METHOD(SimpleNestedArrayIndex)
        {
            // a[b[0]] — используем массив как индекс для другого массива
            // b[0] = 2, значит берем a[2]
            std::string prog =
                "int b = [2, 1, 0];"
                "int a = [10, 20, 30];"
                "x = a[b[0]];";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(30.0, v["x"]);
        }

        TEST_METHOD(MathFunctionAsIndex)
        {
            // a[sin(pi/2) + 1]
            // sin(pi/2) = 1, 1 + 1 = 2, берем a[2]
            std::string prog =
                "pi = 3.141592653589793;"
                "int a = [10, 20, 30];"
                "x = a[sin(pi/2) + 1];";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(30.0, v["x"]);
        }

        TEST_METHOD(MathInsideArrayInsideMath)
        {
            // sin(a[cos(0) + 1])
            // cos(0) = 1, 1 + 1 = 2. a[2] = pi/2. sin(pi/2) = 1.
            std::string prog =
                "pi = 3.141592653589793;"
                "int a = [0, 0, 1.5707963267948966];" // a[2] = pi/2
                "x = sin(a[cos(0) + 1]);";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(1.0, v["x"], 0.00001);
        }

        TEST_METHOD(UserExampleExact)
        {
            // Точная копия вашего примера: a[sin(a[1+cos(a[sin(pi/2)])])]
            std::string prog =
                "pi = 3.141592653589793;"
                "int a = [42, 0.8414709848078965, 1.5707963267948966];"
                "x = sin(a[1+cos(a[sin(pi/2)])]);";

            // Как это вычисляется:
            // 1. sin(pi/2) = 1.0
            // 2. a[1] = 0.84147
            // 3. cos(0.84147) = 0.66636
            // 4. 1 + 0.66636 = 1.66636
            // 5. a[1.66636] -> приведение к int = a[1] = 0.84147
            // 6. sin(0.84147) = 0.74562
            // 7. a[0.74562] -> приведение к int = a[0] = 42.0

            auto v = runAndGetVars(prog);
            Assert::AreEqual(0.74562, v["x"], 0.00001);
        }

        TEST_METHOD(ExtremeNesting)
        {
            // a[b[a[0]]]
            // a[0] = 1 -> b[1] = 2 -> a[2] = 30
            std::string prog =
                "int a = [1, 10, 30];"
                "int b = [0, 2, 40];"
                "x = a[b[a[0]]];";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(30.0, v["x"]);
        }

        TEST_METHOD(NestedFunctionCalls)
        {
            // sin(cos(sin(pi/2)))
            // sin(pi/2) = 1 -> cos(1) = 0.5403 -> sin(0.5403) = 0.5143
            std::string prog =
                "pi = 3.141592653589793;"
                "x = sin(cos(sin(pi/2)));";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(0.514395, v["x"], 0.00001);
        }
        TEST_METHOD(ArrayInsideArrayInsideCos)
        {
            // cos( arr1[ arr2[0] ] )
            // arr2[0] = 1 -> arr1[1] = 0 -> cos(0) = 1.0
            std::string prog =
                "int arr2 = [1, 0];"
                "int arr1 = [1.5707963267948966, 0];"
                "x = cos( arr1[ arr2[0] ] );";

            auto v = runAndGetVars(prog);
            Assert::AreEqual(1.0, v["x"], 0.00001);
        }

        TEST_METHOD(DeepNestedMathWrappedInArray)
        {
            // sin( cos( sin( arr[0] ) ) )
            // arr[0] = pi -> sin(pi) = 0 -> cos(0) = 1 -> sin(1) = 0.84147
            std::string prog =
                "pi = 3.141592653589793;"
                "int arr = [3.141592653589793];"
                "x = sin( cos( sin( arr[0] ) ) );";

            auto v = runAndGetVars(prog);
            Assert::AreEqual(0.8414709848, v["x"], 0.00001);
        }
    };
    // ============================================================
//  Встроенная функция pow
// ============================================================
    TEST_CLASS(PowFunction)
    {
    public:
        TEST_METHOD(PowBasic)
        {
            Assert::AreEqual(8.0, evaluate("pow(2, 3)"));
            Assert::AreEqual(81.0, evaluate("pow(3, 4)"));
        }

        TEST_METHOD(PowZeroAndOne)
        {
            Assert::AreEqual(1.0, evaluate("pow(5, 0)"));  // Любое число в 0 степени = 1
            Assert::AreEqual(1.0, evaluate("pow(1, 100)"));
            Assert::AreEqual(0.0, evaluate("pow(0, 5)"));
        }

        TEST_METHOD(PowNegativeExp)
        {
            // 2^(-2) = 1 / 2^2 = 0.25
            Assert::AreEqual(0.25, evaluate("pow(2, -2)"), 0.000001);
        }

        TEST_METHOD(PowWithExpressions)
        {
            // (1+1)^(2*2) = 2^4 = 16
            Assert::AreEqual(16.0, evaluate("pow(1 + 1, 2 * 2)"));
        }

        TEST_METHOD(PowWithVariables)
        {
            auto v = runAndGetVars("base = 3; exp = 2; x = pow(base, exp);");
            Assert::AreEqual(9.0, v["x"]);
        }

        TEST_METHOD(PowWithArrays)
        {
            // arr[0]^arr[1] = 2^4 = 16
            std::string prog =
                "int arr = [2, 4];"
                "x = pow(arr[0], arr[1]);";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(16.0, v["x"]);
        }

        TEST_METHOD(PowNestedMath)
        {
            // pow(sin(pi/2), 2) + pow(cos(0), 2) = 1^2 + 1^2 = 2
            std::string prog =
                "pi = 3.141592653589793;"
                "x = pow(sin(pi/2), 2) + pow(cos(0), 2);";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(2.0, v["x"], 0.00001);
        }

        TEST_METHOD(PowInsideArrayIndex)
        {
            // arr[ pow(2, 1) ] -> arr[2] = 30
            std::string prog =
                "int arr = [10, 20, 30, 40];"
                "x = arr[ pow(2, 1) ];";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(30.0, v["x"]);
        }

        TEST_METHOD(PowCombinedWithOtherFunctions)
        {
            // pow( cos( sin(pi/2) ), 2 ) -> sin(pi/2)=1, cos(1)=0.5403, 0.5403^2 = 0.2919
            std::string prog =
                "pi = 3.141592653589793;"
                "x = pow( cos( sin(pi/2) ), 2 );";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(0.29192658, v["x"], 0.00001);
        }

        TEST_METHOD(PowWrongArgCount_Throws)
        {
            bool threw = false;
            try { evaluate("pow(2);"); }
            catch (...) { threw = true; }
            Assert::IsTrue(threw, L"pow(2) with 1 argument should throw");

            threw = false;
            try { evaluate("pow(2, 3, 4);"); }
            catch (...) { threw = true; }
            Assert::IsTrue(threw, L"pow(2, 3, 4) with 3 arguments should throw");
        }
    };
    // ============================================================
//  Факториал (оператор ! и функция fact)
// ============================================================
    TEST_CLASS(Factorial)
    {
    public:
        TEST_METHOD(BasicFactorialOperator)
        {
            Assert::AreEqual(1.0, evaluate("0!"));
            Assert::AreEqual(1.0, evaluate("1!"));
            Assert::AreEqual(2.0, evaluate("2!"));
            Assert::AreEqual(6.0, evaluate("3!"));
            Assert::AreEqual(24.0, evaluate("4!"));
            Assert::AreEqual(120.0, evaluate("5!"));
        }

        TEST_METHOD(BasicFactorialFunction)
        {
            Assert::AreEqual(1.0, evaluate("fact(0)"));
            Assert::AreEqual(120.0, evaluate("fact(5)"));
        }

        TEST_METHOD(FactorialPrecedence)
        {
            // ! имеет более высокий приоритет, чем * и ^
            // 3! * 2 = 6 * 2 = 12
            Assert::AreEqual(12.0, evaluate("3! * 2"));

            // 2 ^ 3! = 2 ^ 6 = 64
            Assert::AreEqual(64.0, evaluate("2 ^ 3!"));

            // 3! ^ 2 = 6 ^ 2 = 36
            Assert::AreEqual(36.0, evaluate("3! ^ 2"));
        }

        TEST_METHOD(FactorialWithUnaryMinus)
        {
            // -3! = -(3!) = -6
            Assert::AreEqual(-6.0, evaluate("-3!"));
            // -(3!) = -6
            Assert::AreEqual(-6.0, evaluate("-(3!)"));
        }

        TEST_METHOD(FactorialInExpressions)
        {
            // (2 + 2)! = 4! = 24
            Assert::AreEqual(24.0, evaluate("(2 + 2)!"));

            // 3! + 4! = 6 + 24 = 30
            Assert::AreEqual(30.0, evaluate("3! + 4!"));
        }

        TEST_METHOD(FactorialWithArrays)
        {
            // arr[3!] -> arr[6] = 60
            std::string prog =
                "int arr = [10, 20, 30, 40, 50, 60, 70];"
                "x = arr[3!];";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(70.0, v["x"]); // Индексы с 0: arr[6] это 7-й элемент (70)
        }

        TEST_METHOD(FactorialWrongInput_Throws)
        {
            bool threw = false;
            try {
                evaluate("(-5)!"); // Отрицательный факториал
            }
            catch (...) {
                threw = true;
            }
            Assert::IsTrue(threw, L"Factorial of negative should throw");

            threw = false;
            try {
                evaluate("2.5!"); // Дробный факториал
            }
            catch (...) {
                threw = true;
            }
            Assert::IsTrue(threw, L"Factorial of float should throw");
        }
    };
    // ============================================================
//  Мультифакториалы (!!, !!! и т.д.)
// ============================================================
    TEST_CLASS(MultiFactorial)
    {
    public:
        TEST_METHOD(DoubleFactorial)
        {
            // 5!! = 5 * 3 * 1 = 15
            Assert::AreEqual(15.0, evaluate("5!!"));
            // 6!! = 6 * 4 * 2 = 48
            Assert::AreEqual(48.0, evaluate("6!!"));
            // 7!! = 7 * 5 * 3 * 1 = 105
            Assert::AreEqual(105.0, evaluate("7!!"));
        }

        TEST_METHOD(TripleFactorial)
        {
            // 5!!! = 5 * 2 = 10
            Assert::AreEqual(10.0, evaluate("5!!!"));
            // 7!!! = 7 * 4 * 1 = 28
            Assert::AreEqual(28.0, evaluate("7!!!"));
            // 8!!! = 8 * 5 * 2 = 80
            Assert::AreEqual(80.0, evaluate("8!!!"));
        }

        TEST_METHOD(DoubleFactorialWithSpaces)
        {
            // Пробелы не должны ломать логику
            Assert::AreEqual(15.0, evaluate("5 ! !"));
        }

        TEST_METHOD(RepeatedFactorialInBrackets)
        {
            // Если нужен именно (3!)! = 6! = 720, используем скобки
            Assert::AreEqual(720.0, evaluate("(3!)!"));

            // (2!)! = 2! = 2
            Assert::AreEqual(2.0, evaluate("(2!)!"));
        }

        TEST_METHOD(MultiFactorialInExpression)
        {
            // 3!! + 4!! = 3 + 8 = 11
            Assert::AreEqual(11.0, evaluate("3!! + 4!!"));

            // 2 ^ 3!! = 2 ^ 3 = 8
            Assert::AreEqual(8.0, evaluate("2 ^ 3!!"));
        }

        TEST_METHOD(MultiFactorialWithArray)
        {
            // arr[3!!] -> arr[3] = 40
            std::string prog =
                "int arr = [10, 20, 30, 40, 50];"
                "x = arr[3!!];";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(40.0, v["x"]);
        }
        TEST_METHOD(SinTeilor)
        {
            std::string prog =
                "float pi = 3.141592653589793;"
                "float x = pi/2, sum = 0;"
                "while i < 10 {"
                    "sum = sum + pow(-1, i) * x^(2 * i + 1)/((2 * i + 1)!);"
                    "i = i+1;"
                "}";

            auto v = runAndGetVars(prog);
            Assert::AreEqual(std::sin(3.141592653589793 / 2), v["sum"], 0.01);

        }
    };
    TEST_CLASS(ConcurrencyAndFunctions)
    {
    public:
        TEST_METHOD(TestFunctionAndProc)
        {
            std::string prog =
                "proc printHello() { print(\"Hello\"); }\n"
                "func int add(a, b) { return a + b; }\n"
                "printHello();\n"
                "x = add(5, 10);\n"
                "print(x);";
            std::string out = getOutput(prog);
            Assert::AreEqual(std::string("Hello\n15\n"), out);
        }

        TEST_METHOD(TestRaceConditionWithLock)
        {
            // Воспроизводим доску: c=0, p1 и p2 инкрементируют c.
            std::string prog =
                "int c = 0;\n"
                "proc inc() {\n"
                "  i = 0;\n"
                "  while (i < 100) {\n"
                "    lock(c);\n"
                "    c = c + 1;\n"
                "    unlock(c);\n"
                "    i = i + 1;\n"
                "  }\n"
                "}\n"
                "go inc();\n"
                "go inc();\n"
                "sync;\n"
                "print(c);\n";

            std::string out = getOutput(prog);
            Assert::AreEqual(std::string("200\n"), out); // Ожидаем 200, так как есть lock
        }

        TEST_METHOD(TestAsyncExecution)
        {
            // Тест на то, что go запускает поток, а sync ждет
            std::string prog =
                "proc f(x) { print(\"start \", x); }\n"
                "go f(1);\n"
                "go f(2);\n"
                "sync;\n"
                "print(\"done\");\n";
            std::string out = getOutput(prog);
            // Порядок вывода start 1 и start 2 может меняться, но done всегда в конце
            Assert::IsTrue(out.find("start 1") != std::string::npos);
            Assert::IsTrue(out.find("start 2") != std::string::npos);
            Assert::IsTrue(out.rfind("done") > out.find("start"));
        }
    };
    // ============================================================
//  УГЛУБЛЕННЫЕ ТЕСТЫ ФУНКЦИЙ (Рекурсия, область видимости)
// ============================================================
    TEST_CLASS(AdvancedFunctions)
    {
    public:
        // 1. Рекурсивный расчет чисел Фибоначчи
        TEST_METHOD(RecursiveFibonacci)
        {
            std::string prog =
                "func fib(n) {\n"
                "  if (n < 2) { return n; }\n"
                "  return fib(n - 1) + fib(n - 2);\n"
                "}\n"
                "x = fib(10);";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(55.0, (double)v["x"]);
        }

        // 2. Передача массива в функцию по ссылке (shared_ptr)
        TEST_METHOD(ArrayPassByReference)
        {
            std::string prog =
                "proc modifyArray(arr) {\n"
                "  arr[0] = 99;\n"
                "  arr[1] = arr[0] + 1;\n"
                "}\n"
                "int a = [1, 2, 3];\n"
                "modifyArray(a);\n";
            auto arr = getArrayVar(prog, "a");
            Assert::AreEqual((size_t)3, arr.size());
            Assert::AreEqual(99.0, (double)arr[0]);
            Assert::AreEqual(100.0, (double)arr[1]);
            Assert::AreEqual(3.0, (double)arr[2]); // Третий элемент не тронут
        }

        // 3. Изменение глобальной переменной изнутри функции
        TEST_METHOD(FunctionModifiesGlobal)
        {
            std::string prog =
                "c = 10;\n"
                "proc incGlobal() {\n"
                "  c = c + 5;\n" // Store найдет 'c' в глобальной области
                "}\n"
                "incGlobal();\n"
                "incGlobal();\n";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(20.0, (double)v["c"]);
        }

        // 4. Функция возвращает массив (динамическое создание)
        TEST_METHOD(FunctionReturnsArray)
        {
            std::string prog =
                "func createArray(n) {\n"
                "  int a[3];\n"
                "  a[0] = n;\n"
                "  a[1] = n * 2;\n"
                "  a[2] = n * 3;\n"
                "  return a;\n"
                "}\n"
                "b = createArray(5);\n";
            auto arr = getArrayVar(prog, "b");
            Assert::AreEqual((size_t)3, arr.size());
            Assert::AreEqual(5.0, (double)arr[0]);
            Assert::AreEqual(10.0, (double)arr[1]);
            Assert::AreEqual(15.0, (double)arr[2]);
        }

        // 5. Функция с несколькими параметрами и отсутствием return (возвращает 0)
        TEST_METHOD(DefaultReturnValue)
        {
            std::string prog =
                "func doNothing(a, b) {\n"
                "  x = a + b;\n" // Просто вычисляем, но не возвращаем
                "}\n"
                "y = doNothing(10, 20);\n";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(0.0, (double)v["y"]); // Неявный return 0.0
        }
    };

    // ============================================================
    //  УГЛУБЛЕННЫЕ ТЕСТЫ АСИНХРОННОСТИ (go / sync)
    // ============================================================
    TEST_CLASS(AsyncExecutionExtended)
    {
    public:
        // 1. Аргументы для go фиксируются (снапшот) в момент вызова
        TEST_METHOD(AsyncArgumentSnapshot)
        {
            std::string prog =
                "func printVal(val) { print(val); }\n"
                "x = 100;\n"
                "go printVal(x);\n" // Захватывает 100
                "x = 200;\n"
                "go printVal(x);\n" // Захватывает 200
                "sync;\n";
            std::string out = getOutput(prog);
            Assert::AreEqual(std::string("100\n200\n"), out);
        }

        // 2. Асинхронный цикл накапливает результат
        TEST_METHOD(AsyncLoopAccumulator)
        {
            std::string prog =
                "func addToC() { c = c + 1; }\n"
                "c = 0;\n"
                "i = 0;\n"
                "while (i < 5) {\n"
                "  go addToC();\n"
                "  i = i + 1;\n"
                "}\n"
                "sync;\n";
            auto v = runAndGetVars(prog);
            Assert::AreEqual(5.0, (double)v["c"]);
        }

        // 3. Вложенный асинхронный вызов (go внутри функции, которую вызвали через go)
        TEST_METHOD(NestedAsync)
        {
            std::string prog =
                "func inner() { print(\"inner\"); }\n"
                "func outer() {\n"
                "  print(\"outer start\");\n"
                "  go inner();\n"
                "  print(\"outer end\");\n"
                "}\n"
                "go outer();\n"
                "print(\"before sync\");\n"
                "sync;\n"
                "print(\"after sync\");\n";

            std::string out = getOutput(prog);

            // Порядок между "outer start/end" и "inner" теперь недетерминирован,
            // так как они бегут параллельно на разных ядрах.
            // Проверяем только то, что точно гарантировано.
            Assert::IsTrue(out.find("before sync") != std::string::npos);
            Assert::IsTrue(out.find("after sync") != std::string::npos);
            Assert::IsTrue(out.find("outer start") != std::string::npos);
            Assert::IsTrue(out.find("outer end") != std::string::npos);
            Assert::IsTrue(out.find("inner") != std::string::npos);

            // before всегда до after
            Assert::IsTrue(out.find("before sync") < out.find("after sync"));
            // outer start всегда до outer end (в одном потоке)
            Assert::IsTrue(out.find("outer start") < out.find("outer end"));
            // before всегда до outer start (поток стартует после print)
            Assert::IsTrue(out.find("before sync") < out.find("outer start"));
            // after всегда после всего
            Assert::IsTrue(out.find("after sync") > out.find("outer start"));
            Assert::IsTrue(out.find("after sync") > out.find("outer end"));
            Assert::IsTrue(out.find("after sync") > out.find("inner"));
        }

        // 4. Несколько sync подряд очищают очередь по частям
        TEST_METHOD(MultipleSyncs)
        {
            std::string prog =
                "func hello(n) { print(\"Task \", n); }\n"
                "go hello(1);\n"
                "go hello(2);\n"
                "sync;\n" // Выполняет 1 и 2
                "print(\"---\");\n"
                "go hello(3);\n"
                "sync;\n" // Выполняет 3
                "print(\"End\");\n";

            std::string out = getOutput(prog);
            Assert::AreEqual(std::string("Task 1\nTask 2\n---\nTask 3\nEnd\n"), out);
        }

        // 5. Использование lock/unlock внутри асинхронных задач (имитация доски)
        TEST_METHOD(AsyncWithLocks)
        {
            std::string prog =
                "proc worker(id) {\n"
                "  i = 0;\n"
                "  while (i < 10) {\n"
                "    lock(res);\n"
                "    res = res + 1;\n"
                "    unlock(res);\n"
                "    i = i + 1;\n"
                "  }\n"
                "}\n"
                "res = 0;\n"
                "go worker(1);\n"
                "go worker(2);\n"
                "go worker(3);\n"
                "sync;\n";
            auto v = runAndGetVars(prog);
            // 3 воркера по 10 инкрементов = 30
            Assert::AreEqual(30.0, (double)v["res"]);
        }
    };
    // ============================================================
//  СРАВНЕНИЕ ВРЕМЕНИ ВЫПОЛНЕНИЯ (Sync vs Async)
// ============================================================
    TEST_CLASS(AsyncPerformance)
    {
    public:
        TEST_METHOD(CompareSyncVsAsyncTime)
        {
            // Тяжёлая функция: ~100k итераций
            std::string heavyFunc =
                "proc heavy() {\n"
                "  i = 0; sum = 0;\n"
                "  while (i < 50000) { sum = sum + i; i = i + 1; }\n"
                "}\n";

            // Синхронно
            auto startSync = std::chrono::high_resolution_clock::now();
            evaluate(heavyFunc + "heavy(); heavy();");
            auto endSync = std::chrono::high_resolution_clock::now();
            auto timeSync = std::chrono::duration_cast<std::chrono::milliseconds>(endSync - startSync).count();

            // Асинхронно (реальные потоки)
            auto startAsync = std::chrono::high_resolution_clock::now();
            evaluate(heavyFunc + "go heavy(); go heavy(); sync;");
            auto endAsync = std::chrono::high_resolution_clock::now();
            auto timeAsync = std::chrono::duration_cast<std::chrono::milliseconds>(endAsync - startAsync).count();

            // Логируем в Test Output
            std::string msg = "\n--- Performance ---\n";
            msg += "Sync:  " + std::to_string(timeSync) + " ms\n";
            msg += "Async: " + std::to_string(timeAsync) + " ms\n";
            msg += "Speedup: " + std::to_string((double)timeSync / std::max(1.0, (double)timeAsync)) + "x\n";
            Logger::WriteMessage(msg.c_str());

            // На многоядерном ПК async должен быть быстрее (или хотя бы не сильно медленнее).
            // На одноядерном/CI - возможно равенство.
            Assert::IsTrue(timeSync > 0 && timeAsync > 0, L"Both timings should be positive");
        }
    };
    // ============================================================
    //  МАСШТАБИРОВАНИЕ ПО ЧИСЛУ ПОТОКОВ (3..12)
    // ============================================================
    TEST_CLASS(MultiCoreScaling)
    {
        // Генерирует программу: N потоков, каждый крутит цикл и инкрементит res под lock.
        // После sync res должен быть ровно N.
        static std::string makeWorkProgram(int n, int iters = 500000)
        {
            std::string prog =
                "proc work(id) {\n"
                "  i = 0; s = 0;\n"
                "  while (i < " + std::to_string(iters) + ") { s = s + 1; i = i + 1; }\n"
                "  lock(res);\n"
                "  res = res + 1;\n"
                "  unlock(res);\n"
                "}\n"
                "res = 0;\n";
            for (int k = 1; k <= n; ++k)
                prog += "go work(" + std::to_string(k) + ");\n";
            prog += "sync;\n";
            return prog;
        }

        // Запуск и логирование
        static void runScaling(int n)
        {
            auto prog = makeWorkProgram(n);

            auto start = std::chrono::high_resolution_clock::now();
            auto v = runAndGetVars(prog);
            auto end = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            std::wstring msg = L"[N=" + std::to_wstring(n) + L"] time=" +
                std::to_wstring(ms) + L" ms, res=" +
                std::to_wstring((int)v["res"]) + L"\n";
            Logger::WriteMessage(msg.c_str());

            // Корректность: все N потоков отработали и инкрементнули res.
            Assert::AreEqual((double)n, (double)v["res"]);
        }

    public:
        TEST_METHOD(ThreeThreads) { runScaling(3); }
        TEST_METHOD(FourThreads) { runScaling(4); }
        TEST_METHOD(FiveThreads) { runScaling(5); }
        TEST_METHOD(SixThreads) { runScaling(6); }
        TEST_METHOD(SevenThreads) { runScaling(7); }
        TEST_METHOD(EightThreads) { runScaling(8); }
        TEST_METHOD(NineThreads) { runScaling(9); }
        TEST_METHOD(TenThreads) { runScaling(10); }
        TEST_METHOD(ElevenThreads) { runScaling(11); }
        TEST_METHOD(TwelveThreads) { runScaling(12); }

        // Сводный отчёт: 1..12 потоков, синхронный baseline + масштабирование.
        TEST_METHOD(ScalingReport)
        {
            const int ITERS = 500000;
            Logger::WriteMessage(L"\n--- Multi-core scaling report ---\n");

            // Baseline: 38 последовательных вызовов × ITERS итераций
            std::string syncProg =
                "proc work(id) {\n"
                "  i = 0; s = 0;\n"
                "  while (i < " + std::to_string(ITERS) + ") { s = s + 1; i = i + 1; }\n"
                "}\n";
            for (int k = 1; k <= 138; ++k)
                syncProg += "work(" + std::to_string(k) + ");\n";

            auto s0 = std::chrono::high_resolution_clock::now();
            evaluate(syncProg);
            auto s1 = std::chrono::high_resolution_clock::now();
            long long syncMs = std::chrono::duration_cast<std::chrono::milliseconds>(s1 - s0).count();
            Logger::WriteMessage((L"Baseline (38 sequential × " + std::to_wstring(ITERS) +
                L" iters): " + std::to_wstring(syncMs) + L" ms\n").c_str());

            Logger::WriteMessage(L"N   | Time (ms) | Speedup vs baseline | res\n");
            Logger::WriteMessage(L"----+-----------+---------------------+----\n");

            for (int n = 1; n <= 38; ++n) {
                auto prog = makeWorkProgram(n, ITERS);
                auto start = std::chrono::high_resolution_clock::now();
                auto v = runAndGetVars(prog);
                auto end = std::chrono::high_resolution_clock::now();
                long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                double speedup = (double)syncMs / std::max(1LL, ms);
                std::wstring line =
                    (n < 10 ? L" " : L"") + std::to_wstring(n) + L"  | " +
                    std::to_wstring(ms) + L"\t| " +
                    std::to_wstring(speedup) + L"x\t\t| " +
                    std::to_wstring((int)v["res"]) + L"\n";
                Logger::WriteMessage(line.c_str());

                Assert::AreEqual((double)n, (double)v["res"]);
            }
        }
    };
}