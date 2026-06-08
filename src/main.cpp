#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include "Sema/Sema.h"
#include "CodeGen/CodeGen.h"
#include "Basic/Diagnostic.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string readFile(const std::string &Path) {
  std::ifstream File(Path);
  if (!File) {
    std::fprintf(stderr, "error: could not open '%s'\n", Path.c_str());
    return "";
  }
  std::stringstream SS;
  SS << File.rdbuf();
  return SS.str();
}

static void printUsage(const char *ProgName) {
  std::fprintf(stderr, "Citius Programming Language Compiler\n");
  std::fprintf(stderr, "Usage: %s [options] <input.ct>\n", ProgName);
  std::fprintf(stderr, "Options:\n");
  std::fprintf(stderr, "  -o <file>     Output filename\n");
  std::fprintf(stderr, "  -S            Emit generated C code\n");
  std::fprintf(stderr, "  -c            Compile to object file\n");
  std::fprintf(stderr, "  -O<level>     Optimization level (0-3, default 2)\n");
  std::fprintf(stderr, "  -run          Run via C compiler (temp file)\n");
  std::fprintf(stderr, "  -help         Show this help\n");
}

enum class Action {
  Compile,
  EmitC,
  ObjectFile,
  Run,
};

int main(int argc, char **argv) {
  if (argc < 2) {
    printUsage(argv[0]);
    return 1;
  }

  std::string InputFile;
  std::string OutputFile;
  Action Act = Action::Compile;
  int OptLevel = 2;

  for (int I = 1; I < argc; I++) {
    std::string Arg = argv[I];
    if (Arg == "-o" && I + 1 < argc) {
      OutputFile = argv[++I];
    } else if (Arg == "-S") {
      Act = Action::EmitC;
    } else if (Arg == "-c") {
      Act = Action::ObjectFile;
    } else if (Arg == "-run") {
      Act = Action::Run;
    } else if (Arg == "-help" || Arg == "--help") {
      printUsage(argv[0]);
      return 0;
    } else if (Arg.size() > 2 && Arg[0] == '-' && Arg[1] == 'O') {
      OptLevel = Arg[2] - '0';
      if (OptLevel < 0 || OptLevel > 3)
        OptLevel = 2;
    } else if (Arg[0] != '-') {
      InputFile = Arg;
    }
  }

  if (InputFile.empty()) {
    std::fprintf(stderr, "error: no input file specified\n");
    return 1;
  }

  std::string Source = readFile(InputFile);
  if (Source.empty())
    return 1;

  if (OutputFile.empty()) {
    size_t DotPos = InputFile.find_last_of('.');
    std::string Base = (DotPos != std::string::npos)
                           ? InputFile.substr(0, DotPos)
                           : InputFile;
    switch (Act) {
    case Action::EmitC:
      OutputFile = Base + ".c";
      break;
    case Action::ObjectFile:
      OutputFile = Base + ".o";
      break;
    case Action::Compile:
      OutputFile = Base;
      break;
    case Action::Run:
      break;
    }
  }

  DiagnosticsEngine Diags;

  // Lexer
  Lexer Lex(Diags, InputFile, Source);

  // Parser
  Parser Par(Diags, Lex);
  auto Prog = Par.parseProgram();

  if (Diags.hasError()) {
    Diags.print();
    return 1;
  }

  // Semantic analysis
  Sema SA(Diags);
  if (!SA.analyze(*Prog)) {
    Diags.print();
    return 1;
  }

  // Code generation (C backend)
  CodeGen CG(Diags);

  if (Act == Action::Run) {
    // Generate C code to temp file
    std::string CCode;
    if (!CG.generate(*Prog, CCode)) {
      Diags.print();
      return 1;
    }

    std::string TmpPath = std::tmpnam(nullptr);
    TmpPath += ".c";

    {
      std::ofstream Out(TmpPath);
      if (!Out) {
        std::fprintf(stderr, "error: could not create temp file\n");
        return 1;
      }
      Out << CCode;
    }

    // Compile and run
    std::string ExePath = TmpPath + ".exe";
    if (CG.compileToExecutable(TmpPath, ExePath, OptLevel)) {
      int Ret = std::system(ExePath.c_str());
      std::remove(TmpPath.c_str());
      std::remove(ExePath.c_str());
      return Ret;
    }
    std::remove(TmpPath.c_str());
    std::fprintf(stderr, "error: compilation failed\n");
    return 1;
  }

  // Standard output modes
  std::string CCode;
  if (!CG.generate(*Prog, CCode)) {
    Diags.print();
    return 1;
  }

  if (Diags.hasError()) {
    Diags.print();
    return 1;
  }

  switch (Act) {
  case Action::EmitC: {
    std::ofstream Out(OutputFile);
    if (!Out) {
      std::fprintf(stderr, "error: could not open '%s'\n", OutputFile.c_str());
      return 1;
    }
    Out << CCode;
    std::printf("C source written to %s\n", OutputFile.c_str());
    break;
  }

  case Action::ObjectFile: {
    // Write C code to temp file
    std::string CTmpFile = OutputFile + ".citius_tmp.c";
    {
      std::ofstream Out(CTmpFile);
      Out << CCode;
    }
    if (!CG.compileToObject(CTmpFile, OutputFile, OptLevel)) {
      std::fprintf(stderr, "error: compilation failed\n");
      std::remove(CTmpFile.c_str());
      return 1;
    }
    std::remove(CTmpFile.c_str());
    std::printf("Object file written to %s\n", OutputFile.c_str());
    break;
  }

  case Action::Compile: {
    std::string CTmpFile = OutputFile + ".citius_tmp.c";
    {
      std::ofstream Out(CTmpFile);
      Out << CCode;
    }
    if (!CG.compileToExecutable(CTmpFile, OutputFile, OptLevel)) {
      std::fprintf(stderr, "error: compilation failed\n");
      std::remove(CTmpFile.c_str());
      return 1;
    }
    std::remove(CTmpFile.c_str());
    std::printf("Executable written to %s\n", OutputFile.c_str());
    break;
  }

  case Action::Run:
    break;
  }

  return 0;
}
