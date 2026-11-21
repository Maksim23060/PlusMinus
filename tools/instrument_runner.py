"""Idempotent source patch for older PlusMinus runner.cpp checkouts.

The fork's branch already contains the patched runner. This patch is bundled
only for users applying the standalone source overlay to an older checkout.
"""
from pathlib import Path
import sys


def instrument(source: str) -> str:
    if '"program_instructions"' in source or '\\"program_instructions\\"' in source:
        return source
    old = '#include <algorithm>\n'
    if old not in source:
        raise ValueError('Cannot locate runner.cpp includes')
    source = source.replace(old,old+'#include <chrono>\n',1)
    old = '    std::string phase = "compile";\n    try {\n        auto program = compileToCommands(source);\n        code = listing(program);\n'
    new = '''    std::string phase = "compile";
    using Clock = std::chrono::steady_clock;
    double compileMs = 0.0, executeMs = 0.0;
    size_t instructionCount = 0;
    try {
        auto compilationStart = Clock::now();
        auto program = compileToCommands(source);
        compileMs = std::chrono::duration<double, std::milli>(Clock::now() - compilationStart).count();
        instructionCount = program.size();
        code = listing(program);
'''
    if old not in source:
        raise ValueError('Cannot find expected compile block in runner.cpp')
    source = source.replace(old,new,1)
    old = '''            try { result = number(vm.run(program)); }
            catch (...) { output = vm.output; throw; }
'''
    new = '''            auto executionStart = Clock::now();
            try { result = number(vm.run(program)); }
            catch (...) {
                executeMs = std::chrono::duration<double, std::milli>(Clock::now() - executionStart).count();
                output = vm.output;
                throw;
            }
            executeMs = std::chrono::duration<double, std::milli>(Clock::now() - executionStart).count();
'''
    if old not in source:
        raise ValueError('Cannot find expected execute block in runner.cpp')
    source = source.replace(old,new,1)
    old = '''        << ",\\\"error\\\":" << quote(error) << "}\\n";
'''
    new = '''        << ",\\\"error\\\":" << quote(error)
        << ",\\\"metrics\\\":{\\\"compile_ms\\\":" << compileMs
        << ",\\\"execute_ms\\\":" << executeMs
        << ",\\\"program_instructions\\\":" << instructionCount << "}}\\n";
'''
    if old not in source:
        raise ValueError('Cannot find expected JSON tail in runner.cpp')
    return source.replace(old,new,1)


if __name__ == '__main__':
    path = Path(sys.argv[1] if len(sys.argv)>1 else 'ide/runner.cpp')
    original = path.read_text(encoding='utf-8-sig')
    patched = instrument(original)
    if patched != original:
        path.write_text(patched,encoding='utf-8')
        print(f'Instrumented {path}')
    else:
        print(f'Already instrumented {path}')
