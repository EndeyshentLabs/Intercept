#ifndef LCC_MODULE_HH
#define LCC_MODULE_HH

#include <lcc/forward.hh>
#include <lcc/ir/ir.hh>
#include <lcc/utils.hh>

namespace lcc {
/// An LCC IR module.
class Module {
    Context* _ctx;

    std::vector<Function*> _code;
    std::vector<Value*> _vars;

    Module(Module&) = delete;
    Module(Module&&) = delete;
    Module& operator=(Module&) = delete;
    Module& operator=(Module&&) = delete;

public:
    /// Create a new LCC module.
    ///
    /// It is recommended to allocate these on the heap since
    /// they can’t be moved.
    Module(Context* ctx) : _ctx(ctx) {}

    /// Get the context that owns the module.
    auto context() const -> Context* { return _ctx; }

    /// Emit the module as LLVM IR.
    auto llvm() -> std::string;

    /// Print the IR of this module.
    void print_ir();

    auto code() -> std::vector<Function*>& { return _code; }
    auto code() const -> std::vector<Function*> { return _code; }
    auto vars() -> std::vector<Value*>& { return _vars; }
    auto vars() const -> std::vector<Value*> { return _vars; }

    void add_function(Function* func) { _code.push_back(func); }
    void add_var(Value* var) { _vars.push_back(var); }
};
} // namespace lcc

#endif // LCC_MODULE_HH
