//===- DAGISelMatcherEmitter.cpp - Matcher Emitter ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains code to generate C++ code a matcher.
//
//===----------------------------------------------------------------------===//

#include "DAGISelMatcher.h"
#include "CodeGenDAGPatterns.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/FormattedStream.h"
using namespace llvm;

namespace {
enum {
  CommentIndent = 25
};
}

/// ClassifyInt - Classify an integer by size, return '1','2','4','8' if this
/// fits in 1, 2, 4, or 8 sign extended bytes.
static char ClassifyInt(int64_t Val) {
  if (Val == int8_t(Val))  return '1';
  if (Val == int16_t(Val)) return '2';
  if (Val == int32_t(Val)) return '4';
  return '8';
}

/// EmitInt - Emit the specified integer, returning the number of bytes emitted.
static unsigned EmitInt(int64_t Val, formatted_raw_ostream &OS) {
  unsigned BytesEmitted = 1;
  OS << (int)(unsigned char)Val << ", ";
  if (Val == int8_t(Val)) {
    OS << "\n";
    return BytesEmitted;
  }
  
  OS << (int)(unsigned char)(Val >> 8) << ", ";
  ++BytesEmitted;
  
  if (Val != int16_t(Val)) {
    OS << (int)(unsigned char)(Val >> 16) << ','
       << (int)(unsigned char)(Val >> 24) << ',';
    BytesEmitted += 2;
    
    if (Val != int32_t(Val)) {
      OS << (int)(unsigned char)(Val >> 32) << ','
         << (int)(unsigned char)(Val >> 40) << ','
         << (int)(unsigned char)(Val >> 48) << ','
         << (int)(unsigned char)(Val >> 56) << ',';
      BytesEmitted += 4;
    }   
  }
  
  OS.PadToColumn(CommentIndent) << "// " << Val << '\n';
  return BytesEmitted;
}

namespace {
class MatcherTableEmitter {
  formatted_raw_ostream &OS;
  
  StringMap<unsigned> NodePredicateMap, PatternPredicateMap;
  std::vector<std::string> NodePredicates, PatternPredicates;
  
public:
  MatcherTableEmitter(formatted_raw_ostream &os) : OS(os) {}

  unsigned EmitMatcherAndChildren(const MatcherNode *N, unsigned Indent);
  
  void EmitPredicateFunctions();
private:
  unsigned EmitMatcher(const MatcherNode *N, unsigned Indent);
  
  unsigned getNodePredicate(StringRef PredName) {
    unsigned &Entry = NodePredicateMap[PredName];
    if (Entry == 0) {
      NodePredicates.push_back(PredName.str());
      Entry = NodePredicates.size();
    }
    return Entry-1;
  }
  unsigned getPatternPredicate(StringRef PredName) {
    unsigned &Entry = PatternPredicateMap[PredName];
    if (Entry == 0) {
      PatternPredicates.push_back(PredName.str());
      Entry = PatternPredicates.size();
    }
    return Entry-1;
  }
};
} // end anonymous namespace.

/// EmitMatcherOpcodes - Emit bytes for the specified matcher and return
/// the number of bytes emitted.
unsigned MatcherTableEmitter::
EmitMatcher(const MatcherNode *N, unsigned Indent) {
  OS.PadToColumn(Indent*2);
  
  switch (N->getKind()) {
  case MatcherNode::Push: assert(0 && "Should be handled by caller");
  case MatcherNode::EmitNode:
    OS << "OPC_Emit, /*XXX*/";
    OS.PadToColumn(CommentIndent) << "// Src: "
      << *cast<EmitNodeMatcherNode>(N)->getPattern().getSrcPattern() << '\n';
    OS.PadToColumn(CommentIndent) << "// Dst: "
      << *cast<EmitNodeMatcherNode>(N)->getPattern().getDstPattern() << '\n';
    return 1;
  case MatcherNode::Record:
    OS << "OPC_Record,\n";
    return 1;
  case MatcherNode::MoveChild:
    OS << "OPC_MoveChild, "
       << cast<MoveChildMatcherNode>(N)->getChildNo() << ",\n";
    return 2;
      
  case MatcherNode::MoveParent:
    OS << "OPC_MoveParent,\n";
    return 1;
      
  case MatcherNode::CheckSame:
    OS << "OPC_CheckSame, "
       << cast<CheckSameMatcherNode>(N)->getMatchNumber() << ",\n";
    return 2;

  case MatcherNode::CheckPatternPredicate: {
    StringRef Pred = cast<CheckPatternPredicateMatcherNode>(N)->getPredicate();
    OS << "OPC_CheckPatternPredicate, " << getPatternPredicate(Pred) << ',';
    OS.PadToColumn(CommentIndent) << "// " << Pred << '\n';
    return 2;
  }
  case MatcherNode::CheckPredicate: {
    StringRef Pred = cast<CheckPredicateMatcherNode>(N)->getPredicateName();
    OS << "OPC_CheckPredicate, " << getNodePredicate(Pred) << ',';
    OS.PadToColumn(CommentIndent) << "// " << Pred << '\n';
    return 2;
  }

  case MatcherNode::CheckOpcode:
    OS << "OPC_CheckOpcode, "
       << cast<CheckOpcodeMatcherNode>(N)->getOpcodeName() << ",\n";
    return 2;
      
  case MatcherNode::CheckType:
    OS << "OPC_CheckType, "
       << getEnumName(cast<CheckTypeMatcherNode>(N)->getType()) << ",\n";
    return 2;

  case MatcherNode::CheckInteger: {
    int64_t Val = cast<CheckIntegerMatcherNode>(N)->getValue();
    OS << "OPC_CheckInteger" << ClassifyInt(Val) << ", ";
    return EmitInt(Val, OS)+1;
  }   
  case MatcherNode::CheckCondCode:
    OS << "OPC_CheckCondCode, ISD::"
       << cast<CheckCondCodeMatcherNode>(N)->getCondCodeName() << ",\n";
    return 2;
      
  case MatcherNode::CheckValueType:
    OS << "OPC_CheckValueType, MVT::"
       << cast<CheckValueTypeMatcherNode>(N)->getTypeName() << ",\n";
    return 2;

  case MatcherNode::CheckComplexPat:
    OS << "OPC_CheckComplexPat, 0/*XXX*/,\n";
    return 2;
      
  case MatcherNode::CheckAndImm: {
    int64_t Val = cast<CheckAndImmMatcherNode>(N)->getValue();
    OS << "OPC_CheckAndImm" << ClassifyInt(Val) << ", ";
    return EmitInt(Val, OS)+1;
  }

  case MatcherNode::CheckOrImm: {
    int64_t Val = cast<CheckOrImmMatcherNode>(N)->getValue();
    OS << "OPC_CheckOrImm" << ClassifyInt(Val) << ", ";
    return EmitInt(Val, OS)+1;
  }
  case MatcherNode::CheckProfitableToFold:
    OS << "OPC_IsProfitableToFold,\n";
    return 1;
  case MatcherNode::CheckLegalToFold:
    OS << "OPC_IsLegalToFold,\n";
    return 1;
  }
  assert(0 && "Unreachable");
  return 0;
}

/// EmitMatcherAndChildren - Emit the bytes for the specified matcher subtree.
unsigned MatcherTableEmitter::
EmitMatcherAndChildren(const MatcherNode *N, unsigned Indent) {
  unsigned Size = 0;
  while (1) {
    // Push is a special case since it is binary.
    if (const PushMatcherNode *PMN = dyn_cast<PushMatcherNode>(N)) {
      // We need to encode the child and the offset of the failure code before
      // emitting either of them.  Handle this by buffering the output into a
      // string while we get the size.
      SmallString<128> TmpBuf;
      unsigned ChildSize;
      {
        raw_svector_ostream OS(TmpBuf);
        formatted_raw_ostream FOS(OS);
        ChildSize = 
          EmitMatcherAndChildren(cast<PushMatcherNode>(N)->getChild(),Indent+1);
      }
      
      if (ChildSize > 255) {
        errs() <<
          "Tblgen internal error: can't handle predicate this complex yet\n";
        exit(1);
      }
      
      OS.PadToColumn(Indent*2);
      OS << "OPC_Push, " << ChildSize << ",\n";
      OS << TmpBuf.str();
      
      Size += 2 + ChildSize;
      
      N = PMN->getFailure();
      continue;
    }
  
    Size += EmitMatcher(N, Indent);
    
    // If there are children of this node, iterate to them, otherwise we're
    // done.
    if (const MatcherNodeWithChild *MNWC = dyn_cast<MatcherNodeWithChild>(N))
      N = MNWC->getChild();
    else
      return Size;
  }
}

void MatcherTableEmitter::EmitPredicateFunctions() {
  OS << "bool CheckPatternPredicate(unsigned PredNo) const {\n";
  OS << "  switch (PredNo) {\n";
  OS << "  default: assert(0 && \"Invalid predicate in table?\");\n";
  for (unsigned i = 0, e = PatternPredicates.size(); i != e; ++i)
    OS << "  case " << i << ": return "  << PatternPredicates[i] << ";\n";
  OS << "  }\n";
  OS << "}\n\n";

  OS << "bool CheckNodePredicate(SDNode *N, unsigned PredNo) const {\n";
  OS << "  switch (PredNo) {\n";
  OS << "  default: assert(0 && \"Invalid predicate in table?\");\n";
  for (unsigned i = 0, e = NodePredicates.size(); i != e; ++i)
    OS << "  case " << i << ": return "  << NodePredicates[i] << "(N);\n";
  OS << "  }\n";
  OS << "}\n\n";
}


void llvm::EmitMatcherTable(const MatcherNode *Matcher, raw_ostream &O) {
  formatted_raw_ostream OS(O);
  
  OS << "// The main instruction selector code.\n";
  OS << "SDNode *SelectCode2(SDNode *N) {\n";

  MatcherTableEmitter MatcherEmitter(OS);

  OS << "  static const unsigned char MatcherTable[] = {\n";
  unsigned TotalSize = MatcherEmitter.EmitMatcherAndChildren(Matcher, 2);
  OS << "    0\n  }; // Total Array size is " << (TotalSize+1) << " bytes\n\n";
  OS << "  return SelectCodeCommon(N, MatcherTable,sizeof(MatcherTable));\n}\n";
  OS << "\n";
  
  // Next up, emit the function for node and pattern predicates:
  MatcherEmitter.EmitPredicateFunctions();
}