#ifndef CITIUS_DIAGNOSTIC_H
#define CITIUS_DIAGNOSTIC_H

#include "SourceLocation.h"
#include <string>
#include <vector>
#include <cstdio>

enum class DiagKind {
  Error,
  Warning,
  Note,
};

struct Diagnostic {
  DiagKind Kind;
  SourceLocation Loc;
  std::string Message;

  Diagnostic(DiagKind Kind, SourceLocation Loc, std::string Message)
      : Kind(Kind), Loc(Loc), Message(std::move(Message)) {}
};

class DiagnosticsEngine {
public:
  void report(DiagKind Kind, SourceLocation Loc, std::string Message) {
    Diagnostics.push_back({Kind, Loc, std::move(Message)});
  }

  void error(SourceLocation Loc, std::string Message) {
    report(DiagKind::Error, Loc, std::move(Message));
  }

  void warn(SourceLocation Loc, std::string Message) {
    report(DiagKind::Warning, Loc, std::move(Message));
  }

  void note(SourceLocation Loc, std::string Message) {
    report(DiagKind::Note, Loc, std::move(Message));
  }

  bool hasError() const {
    for (auto &D : Diagnostics)
      if (D.Kind == DiagKind::Error)
        return true;
    return false;
  }

  size_t errorCount() const {
    size_t count = 0;
    for (auto &D : Diagnostics)
      if (D.Kind == DiagKind::Error)
        count++;
    return count;
  }

  void print() const {
    for (auto &D : Diagnostics) {
      const char *prefix = "";
      switch (D.Kind) {
      case DiagKind::Error:
        prefix = "error";
        break;
      case DiagKind::Warning:
        prefix = "warning";
        break;
      case DiagKind::Note:
        prefix = "note";
        break;
      }
      std::fprintf(stderr, "%s: %s: %s\n", D.Loc.toString().c_str(), prefix,
                   D.Message.c_str());
    }
  }

  void clear() { Diagnostics.clear(); }

private:
  std::vector<Diagnostic> Diagnostics;
};

#endif
