#pragma once

#include "antigo/Context.h"

#include <bitset>
#include <cassert>
#include <cstdint>
#include <fmt/format.h>
#include <memory>
#include <optional>
#include <variant>
#include <string>

#include "antigo/impl/complex_values.h"
#include "antigo/ResolvedContext.h"

// TODO: support cpptrace
#define ANTIGO_TRY try
#define ANTIGO_CATCH catch
#define ANTIGO_CURRENT_EXCEPTION std::string{"cpptrace is disabled in this build"}
// #define ANTIGO_CURRENT_EXCEPTION cpptrace::from_current_exception().to_string()

namespace antigo {

namespace {

struct OwnedValue {
  std::function<std::string()> printerFunc;
};

std::string Strip(const std::string& s) {
  size_t len = s.length();
  while (len > 0 && s[len - 1] == '\n') {
    --len;
  }
  return s.substr(0, len);
}

// FIXME: should be separate for each size
// (uint) uint: 1736811162163836790 | int: 1736811162163836790 | hex: 0x181a648ccab57776 | bin: 0001100000011010011001001000110011001010101101010111011101110110
std::string IntRepresentations(uint64_t val) {
  std::bitset<64> binaryVal(val);

  return fmt::format(
    "{} | hex: 0x{:x} | bin: {}",
    val,
    val,
    binaryVal.to_string()
  );
}

std::string IntRepresentations(int64_t val) {
  std::bitset<64> binaryVal(val);

  return fmt::format(
    "{} | hex as uint: 0x{:x} | bin: {}",
    val,
    static_cast<uint64_t>(val),
    binaryVal.to_string()
  );
}

} // namespace

struct OnstackDataFrame;

struct OnstackDataFrame {
  using ValueType = std::variant<const char*, uint64_t, int64_t, std::unique_ptr<OwnedValue>, impl::ReferencedValue>;
  ValueType value;

  struct ToPreparedVisitor {
    ResolvedMessageEntry operator()(const char* v) const {
      return {"cstr", v};
    }

    ResolvedMessageEntry operator()(uint64_t v) const {
      return {"uint", IntRepresentations(v)};
    }

    ResolvedMessageEntry operator()(int64_t v) const {
      return {"int", IntRepresentations(v)};
    }

    ResolvedMessageEntry operator()(const std::unique_ptr<OwnedValue>& v) const {
      return {"custom", Strip(v->printerFunc())};
    }

    ResolvedMessageEntry operator()(const impl::ReferencedValue& v) const {
      if (v.guard == nullptr) {
        return {"custom", "(reference possibly expired or Arm() not called)"};
      }
      return {"custom", Strip(v.guard->printerFunc())};
    }
  };

  ResolvedMessageEntry Resolve() const {
    ANTIGO_TRY {
      return std::visit(ToPreparedVisitor{}, value);
    } ANTIGO_CATCH (const std::exception& e) {
      return {"error", "ctx: error while resolving value, variant index " + std::to_string(value.index()) + "\nwhat: " + e.what() + '\n' + ANTIGO_CURRENT_EXCEPTION};
    }
    return {"error", "resolve error: this code should be unreachable?"};
  }
};
static_assert(sizeof(OnstackDataFrame) == 16);

struct OnstackContextImplSlow;

class OnstackContextImpl {
public:
  OnstackContextImpl(const char* filename_, size_t linenum_, const char* funcname_);

  ~OnstackContextImpl();

  ResolvedContextEntry ResolveCurrentImpl() const;

  [[nodiscard]]
  ResolvedContext ResolveCtxStackImpl(std::string reason) const;

  [[nodiscard]]
  OnstackDataFrame* TryEmplaceFrame() {
    if (h.dataFramesCnt == kMaxDataFrameCnt) {
      static_assert(255 == std::numeric_limits<decltype(h.skippedDataFramesCnt)>::max());
      if (h.skippedDataFramesCnt != 255) {
        ++h.skippedDataFramesCnt;
      }
      return nullptr;
    }
    return &dataFrames[h.dataFramesCnt++];
  }

  template <class T>
  T* TryEmplaceFrameValue() {
    if (auto* frame = TryEmplaceFrame()) {
      return &frame->value.emplace<T>();
    }
    return nullptr;
  }

  void AddMessage(const char* message) {
    if (auto* frame = TryEmplaceFrame()) {
      frame->value = message;
    }
  }

  void AddPtr(const void* ptr) {
    static_assert(sizeof(const void*) == sizeof(uint64_t));
    AddUnsigned(reinterpret_cast<uint64_t>(ptr));
  }

  void AddUnsigned(uint64_t val) {
    if (auto* frame = TryEmplaceFrame()) {
      frame->value = val;
    }
  }

  void AddSigned(int64_t val) {
    if (auto* frame = TryEmplaceFrame()) {
      frame->value = val;
    }
  }

  void AddLambdaWithOwned(std::function<std::string()> printerFunc) {
    if (auto* frame = TryEmplaceFrame()) {
      frame->value = std::make_unique<OwnedValue>(OwnedValue{printerFunc});
    }
  }

  [[nodiscard]] impl::ReferencedValueGuard AddLambdaWithRef(std::function<std::string()> printerFunc) {
    if (auto* frame = TryEmplaceFrame()) {
      auto& value = frame->value.emplace<impl::ReferencedValue>(impl::ReferencedValue{/*guard=*/nullptr}); // to be filled via Arm()
      return {&value, std::move(printerFunc)};
    }
    return {};
  }

  ResolvedContext Resolve() const;
  void Orphan() const;

  void LogInnerExecution();
  bool IsLoggingInnerExecution() const;

  constexpr static size_t kSize = OnstackContext::kSize;

private:
  OnstackContextImplSlow& AccessSlow() {
    if (!h.slow) {
      h.slow = std::make_unique<OnstackContextImplSlow>();
    }
    return *h.slow;
  }

private:
  // header, compactly stores some essential data
  struct {
    // location data
    // (also frame cnt bc needs to be tightly packed)
    const char* filename;
    uint16_t linenum;
    uint8_t dataFramesCnt;
    uint8_t skippedDataFramesCnt;
    bool destructing;
    bool errorOnTop;
    const char* funcname;

    // back/forward pointers
    OnstackContextImpl* downCtx;

    std::unique_ptr<OnstackContextImplSlow> slow;
    uint64_t padding[2];
    int uncaughtExceptions;
  } h;
  static_assert(sizeof(h) == 64);

  // calculate how much frames can we store with the remaining space and assert some stuff
  static_assert((kSize - sizeof(h)) % sizeof(OnstackDataFrame) == 0);
  static constexpr size_t kMaxDataFrameCnt = (kSize - sizeof(h)) / sizeof(OnstackDataFrame);
  static_assert(kMaxDataFrameCnt <= std::numeric_limits<decltype(h.dataFramesCnt)>::max());

  static_assert(kMaxDataFrameCnt == 60, "update value in tests");

  // messages that are associated with this context (= logged by the current function or a lightweight context above)
  OnstackDataFrame dataFrames[kMaxDataFrameCnt];
};

struct InnerExecutionEvent
{
  std::string type; // enter/leave
  ResolvedContextEntry resolvedCtxEntry; // when type == leave
};

struct OnstackContextImplSlow
{
  std::shared_ptr<std::vector<InnerExecutionEvent>> innerExecEvts;
};

}  // namespace antigo
