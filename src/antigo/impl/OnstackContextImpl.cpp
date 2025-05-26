#include "antigo/impl/OnstackContextImpl.h"

#include <cassert>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>

#include "OnstackContextImpl.h"
#include "antigo/Context.h"
#include "antigo/ResolvedContext.h"
#include "antigo/impl/ExecutionData.h"

namespace antigo {

namespace {
std::string ToString(const std::vector<InnerExecutionEvent>& evts) {
  constexpr size_t maxToPrint = 30;
  size_t start = 0;
  if (start + maxToPrint < evts.size()) {
    start = evts.size() - maxToPrint;
  }
  std::stringstream ss;
  ss << "inner tracer messages [" << start << ".." << evts.size() << ") - latest exits = earliest enter-s last\n";
  for (size_t i = start; i < evts.size(); ++i) {
    ss << evts[i].resolvedCtxEntry << "\n";
    ss << "[" << i << "] = " << evts[i].type << " ^";
    if (evts[i].type == "enter") {
      ss << ">>>>>>>>>";
    }
    if (evts[i].type == "leave") {
      ss << "<<<<<<<<<";
    }
    if (i + 1 != evts.size()) {
      ss << "\n";
    }
  }
  return std::move(ss).str();
}
} // namespace

OnstackContextImpl::OnstackContextImpl(const char* filename_, size_t linenum_, const char* funcname_): h{}, dataFrames{} {
  h.filename = filename_;
  h.linenum = std::min<size_t>(linenum_, std::numeric_limits<decltype(h.linenum)>::max());
  h.dataFramesCnt = 0;
  h.skippedDataFramesCnt = 0;
  h.destructing = false;
  h.errorOnTop = false;
  h.funcname = funcname_;

  h.downCtx = nullptr;
  if (GetCurrentExecutionData().stackCtxChain.size()) {
    assert(GetCurrentExecutionData().stackCtxChain.back());
    h.downCtx = GetCurrentExecutionData().stackCtxChain.back();
  }

  h.slow = nullptr;

  h.uncaughtExceptions = std::uncaught_exceptions();

  if (h.uncaughtExceptions) {
    AddMessage("ctx: has uncaught exceptions; count=");
    AddUnsigned(h.uncaughtExceptions);
  }

  GetCurrentExecutionData().stackCtxChain.push_back(this);

  if (h.downCtx != nullptr && h.downCtx->h.slow != nullptr) {
    auto& evts = AccessSlow().innerExecEvts = h.downCtx->h.slow->innerExecEvts;
    evts->push_back({"enter", ResolveCurrentImpl()});
  }
}

OnstackContextImpl::~OnstackContextImpl() {
  ANTIGO_TRY {
    h.destructing = true;

    if (h.slow != nullptr) {
      auto& evts = h.slow->innerExecEvts;
      if (h.downCtx == nullptr || h.downCtx->h.slow == nullptr || h.downCtx->h.slow->innerExecEvts == nullptr) {
        // stop
      } else {
        evts->push_back({"leave", ResolveCurrentImpl()});
      }
    }

    assert(GetCurrentExecutionData().stackCtxChain.size() && GetCurrentExecutionData().stackCtxChain.back() == this);
    if (std::uncaught_exceptions() != h.uncaughtExceptions || (h.downCtx && h.downCtx->h.uncaughtExceptions != h.uncaughtExceptions)) {
      auto& w = GetCurrentExecutionData().errorWitnesses;
      if (!h.errorOnTop) {
        ANTIGO_TRY {
          w.emplace_back(ResolveCtxStackImpl("exception"));
        } ANTIGO_CATCH (const std::exception& e) {
          std::cerr << "dtor: context resolved with an exception, what: " << e.what() << "\n";
          // cpptrace::from_current_exception().print();
          std::cerr << ANTIGO_CURRENT_EXCEPTION << std::endl;
        }
      }
      if (h.downCtx == nullptr) {
        while (!w.empty()) {
          GetCurrentExecutionData().orphans.push_back(std::move(w.back()));
          w.pop_back();
        }
      } else {
        h.downCtx->h.errorOnTop = true;
      }
    } else {
      auto& w = GetCurrentExecutionData().errorWitnesses;
      while (!w.empty()) {
        GetCurrentExecutionData().orphans.push_back(std::move(w.back()));
        w.pop_back();
      }
    }
    GetCurrentExecutionData().stackCtxChain.pop_back();
  } ANTIGO_CATCH (const std::exception& e) {
    std::cerr << "fatal exception, what: " << e.what() << "\n";
    // cpptrace::from_current_exception().print();
    std::cerr << ANTIGO_CURRENT_EXCEPTION << std::endl;
    std::terminate();
    // terminating here on purpose, our state could get inconsistent
  }
}

ResolvedContextEntry OnstackContextImpl::ResolveCurrentImpl() const {
  ResolvedContextEntry entry;
  entry.sourceLoc.filename = h.filename;
  entry.sourceLoc.line = h.linenum;
  entry.sourceLoc.func = h.funcname;

  for (size_t i = 0; i < h.dataFramesCnt; ++i) {
    entry.messages.emplace_back(dataFrames[i].Resolve());
  }

  if (h.skippedDataFramesCnt) {
    std::string msg = std::to_string(h.skippedDataFramesCnt);
    if (h.skippedDataFramesCnt == std::numeric_limits<decltype(h.skippedDataFramesCnt)>::max()) {
      msg += "+";
    }
    msg += " last messages didn't fit into buffer";
    entry.messages.push_back(ResolvedMessageEntry{"meta", std::move(msg)});
  }
  return entry;
}

ResolvedContext OnstackContextImpl::ResolveCtxStackImpl(std::string reason) const {
  ResolvedContext result;
  result.reason = std::move(reason);
  const auto& chain = GetCurrentExecutionData().stackCtxChain;
  for (size_t i = chain.size(); i > 0; --i) {
    result.entries.push_back(chain[i - 1]->ResolveCurrentImpl());
  }
  if (!result.entries.empty()) {
    // result.entries[0].messages.push_back({"stacktrace", cpptrace::generate_trace().to_string()});
    if (h.slow != nullptr && h.slow->innerExecEvts != nullptr) {
      result.entries[0].messages.push_back({"inner_tracer", ToString(*h.slow->innerExecEvts)});
    }
    // result.entries[0].messages.push_back({"resolution_id", "TODO currenttime.rand"});
  }
  return result;
}

ResolvedContext OnstackContextImpl::Resolve() const {
  return ResolveCtxStackImpl("ondemand");
}

void OnstackContextImpl::Orphan() const {
  GetCurrentExecutionData().orphans.emplace_back(Resolve());
}

void OnstackContextImpl::LogInnerExecution() {
  auto& slow = AccessSlow();
  slow.innerExecEvts = std::make_shared<std::vector<InnerExecutionEvent>>();
}

bool OnstackContextImpl::IsLoggingInnerExecution() const {
  return h.slow != nullptr && h.slow->innerExecEvts != nullptr;
}

}
