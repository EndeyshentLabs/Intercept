#include <codegen/opt/opt-internal.h>

typedef Vector(IRBlock *) BlockVector;

/// ===========================================================================
///  Helpers
/// ===========================================================================
#ifdef _MSC_VER
#  include <intrin.h>

uint32_t ctzll(uint64_t value) {
  unsigned long zero = 0;
  return _BitScanForward64(&zero, value)
         ? (uint32_t) zero
         : 64;
}
#else
#  define ctzll __builtin_ctzll
#endif

#define IR_REDUCE_BINARY(op)           \
  if (is_immediate_pair(i)) {          \
    IRInstruction *lhs = i->lhs;       \
    IRInstruction *rhs = i->rhs;       \
    ir_remove_use(lhs, i);             \
    ir_remove_use(rhs, i);             \
    i->kind = IR_IMMEDIATE;            \
    i->imm = imm_lhs(i) op imm_rhs(i); \
    changed = true;                    \
  }

static bool is_immediate_pair(IRInstruction *i) {
  return i->lhs->kind == IR_IMMEDIATE &&
         i->rhs->kind == IR_IMMEDIATE;
}

static u64 imm_lhs(IRInstruction *i) {
  return i->lhs->imm;
}

static u64 imm_rhs(IRInstruction *i) {
  return i->rhs->imm;
}

static bool power_of_two(u64 value) {
  return value > 0 && (value & (value - 1)) == 0;
}

static bool has_side_effects(IRInstruction *i) {
  STATIC_ASSERT(IR_COUNT == 39, "Handle all instructions");
  switch (i->kind) {
    /// These do NOT have side effects.
    case IR_IMMEDIATE:
    case IR_LOAD:
    case IR_PARAMETER:
    case IR_NOT:
    case IR_STATIC_REF:
    case IR_FUNC_REF:
    case IR_LIT_INTEGER:
    case IR_LIT_STRING:
    case IR_ALLOCA:
    case IR_ZERO_EXTEND:
    case IR_SIGN_EXTEND:
    case IR_TRUNCATE:
    case IR_BITCAST:
      ALL_BINARY_INSTRUCTION_CASES()
      return false;

    case IR_CALL:
      return i->call.is_indirect || !i->call.callee_function->attr_pure || i->call.tail_call;

    default:
      return true;
  }
}

/// ===========================================================================
///  Instruction combination
/// ===========================================================================
/// Everything that merges instructions or performs strength reduction,
/// folding, etc. etc. goes here. If you’re unsure where to put something,
/// put it here.
///
/// Note: Take care to remove uses etc. *before* overwriting the `imm` field
/// as it is in a union together with whatever it is whose uses you want to
/// remove.
static bool opt_instcombine(IRFunction *f) {
  bool changed = false;
  list_foreach (b, f->blocks) {
    list_foreach (i, b->instructions) {
      switch (i->kind) {
        case IR_ADD:
          IR_REDUCE_BINARY(+)
          else {
            // Adding zero to something == no-op
            if (i->lhs->kind == IR_IMMEDIATE && imm_lhs(i) == 0) {
              ir_remove_use(i->lhs, i);
              ir_remove_use(i->rhs, i);
              ir_replace_uses(i, i->rhs);
            } else if (i->rhs->kind == IR_IMMEDIATE && imm_rhs(i) == 0) {
              ir_remove_use(i->lhs, i);
              ir_remove_use(i->rhs, i);
              ir_replace_uses(i, i->lhs);
            }
          }
          break;
        case IR_SUB:
          IR_REDUCE_BINARY(-)
          else {
            // Subtracting zero from something == no-op
            if (i->rhs->kind == IR_IMMEDIATE && imm_rhs(i) == 0) {
              ir_remove_use(i->lhs, i);
              ir_remove_use(i->rhs, i);
              ir_replace_uses(i, i->lhs);
            }
          }
          break;
        case IR_MUL:
          IR_REDUCE_BINARY(*)
          else {
            // Multiplying by zero == zero
            if ((i->lhs->kind == IR_IMMEDIATE && imm_lhs(i) == 0) || (i->rhs->kind == IR_IMMEDIATE && imm_rhs(i) == 0)) {
              ir_remove_use(i->lhs, i);
              ir_remove_use(i->rhs, i);
              i->kind = IR_IMMEDIATE;
              i->imm = 0;
            }
            // Multiplying 1 * rhs == rhs
            else if (i->lhs->kind == IR_IMMEDIATE && imm_lhs(i) == 1) {
              ir_remove_use(i->lhs, i);
              ir_remove_use(i->rhs, i);
              ir_replace_uses(i, i->rhs);
            }
            // Multiplying lhs * 1 == lhs
            else if (i->rhs->kind == IR_IMMEDIATE && imm_rhs(i) == 1) {
              ir_remove_use(i->lhs, i);
              ir_remove_use(i->rhs, i);
              ir_replace_uses(i, i->lhs);
            }
          }
          break;

        case IR_DIV:
          IR_REDUCE_BINARY(/)
          else {
            IRInstruction *divisor = i->rhs;
            if (divisor->kind == IR_IMMEDIATE) {
              /// Division by 1 does nothing.
              if (divisor->imm == 1) {
                ir_remove_use(i->lhs, i);
                ir_remove_use(divisor, i);
                ir_replace_uses(i, i->rhs);
              }

              /// Replace division by a power of two with a shift.
              else if (power_of_two(divisor->imm)) {
                i->kind = IR_SAR;
                divisor->imm = (u64) ctzll(divisor->imm);
                changed = true;
              }
            }
          }
          break;
        case IR_MOD:
          IR_REDUCE_BINARY(%)
          break;

        case IR_SHL:
          IR_REDUCE_BINARY(<<)
          break;
        case IR_SHR:
          IR_REDUCE_BINARY(>>)
          break;
        case IR_SAR:
          if (is_immediate_pair(i)) {
            IRInstruction *lhs = i->lhs;
            IRInstruction *rhs = i->rhs;
            ir_remove_use(lhs, i);
            ir_remove_use(rhs, i);
            i->kind = IR_IMMEDIATE;
            i->imm = (u64) ((i64) imm_lhs(i) >> imm_rhs(i));
            changed = true;
          }
          break;
        case IR_AND:
          IR_REDUCE_BINARY(&)
          break;
        case IR_OR:
          IR_REDUCE_BINARY(|)
          break;
        case IR_NOT:
          if (i->operand->kind == IR_IMMEDIATE) {
            /// Note: operand and value share the same union field, so
            /// be careful to remove uses before overwriting the union.
            ir_remove_use(i->operand, i);
            i->kind = IR_IMMEDIATE;
            i->imm = ~i->operand->imm;
            changed = true;
          }
          break;
        default: break;

        /// Simplify conditional branches with constant conditions.
        case IR_BRANCH_CONDITIONAL: {
          if (i->cond_br.condition->kind != IR_IMMEDIATE) break;

          /// Remove use of condition.
          i->kind = IR_BRANCH;
          ir_remove_use(i->cond_br.condition, i);

          /// Convert to unconditional branch.
          i->destination_block = i->cond_br.condition->imm
            ? i->cond_br.then
            : i->cond_br.else_;
        } break;

        /// Simplify PHIs that contain only a single argument.
        case IR_PHI: {
          if (i->phi_args.size > 1) break;
          ir_remove_use(i->phi_args.data[0]->value, i);
          ir_replace_uses(i, i->phi_args.data[0]->value);
          ir_remove(i);
        } break;

        /// Simplify indirect calls to direct calls.
        case IR_CALL: {
          if (!i->call.is_indirect) break;
          IRInstruction *callee = i->call.callee_instruction;
          switch (callee->kind) {
            default: break;

            case IR_FUNC_REF:
              i->call.is_indirect = false;
              i->call.callee_function = callee->function_ref;
              ir_remove_use(callee, i);
              break;

            case IR_BITCAST: {
              if (callee->operand->kind == IR_FUNC_REF) {
                i->call.is_indirect = false;
                i->call.callee_function = callee->operand->function_ref;
                ir_remove_use(callee->operand, callee);
                ir_remove_use(callee, i);
              }
            } break;
          }
        }
      }
    }
  }
  return changed;
}

/// ===========================================================================
///  DCE
/// ===========================================================================
static bool opt_dce(IRFunction *f) {
  bool changed = false;
  list_foreach (b, f->blocks) {
    for (IRInstruction *i = b->instructions.first; i;) {
      if (!i->users.size && !has_side_effects(i)) {
        IRInstruction *next = i->next;
        ir_remove(i);
        changed = true;
        i = next;
      } else {
        i = i->next;
      }
    }
  }
  return changed;
}

/// ===========================================================================
///  TCE
/// ===========================================================================
typedef struct {
  IRInstruction *call;
  Vector(IRInstruction *) phis;
} tail_call_info;

/// See opt_tail_call_elim() for more info.
static bool tail_call_possible_iter(tail_call_info *tc, IRBlock *b) {
  for (IRInstruction *i = b == tc->call->parent_block ? tc->call->next : b->instructions.first; i; i = i->next) {
    if (i->kind == IR_PHI) {
      /// If this is a phi node, then the call or a previous phi
      /// must be an argument of the phi.
      foreach_val (arg, i->phi_args) {
        if (arg->value == tc->call) { goto phi; }
        foreach_val (a, tc->phis) {
          if (a == arg->value) { goto phi; }
        }
      }
      return false;

    phi:
      vector_push(tc->phis, i);
      continue;
    }

    /// If we encounter a return instruction, then a tail call
    /// is only possible if the return value is the call, or
    /// any of the PHIs.
    if (i->kind == IR_RETURN) {
      foreach_val (a, tc->phis) {
        if (a == i->operand) { return true; }
      }
      return i->operand == tc->call;
    }

    if (i->kind == IR_BRANCH) { return tail_call_possible_iter(tc, i->destination_block); }
    if (i->kind == IR_BRANCH_CONDITIONAL) {
      return tail_call_possible_iter(tc, i->cond_br.then) &&
             tail_call_possible_iter(tc, i->cond_br.else_);
    }

    /// Any other instruction means that the call is not the last
    /// relevant instruction before a return.
    return false;
  }

  return false;
}

static bool tail_call_possible(IRInstruction *i) {
  tail_call_info tc_info = {0};
  tc_info.call = i;
  bool possible = tail_call_possible_iter(&tc_info, i->parent_block);
  vector_delete(tc_info.phis);
  return possible;
}

bool opt_try_convert_to_tail_call(IRInstruction *i) {
  /// An instruction is a tail call iff there are no other instruction
  /// between it and the next return instruction other than branches
  /// and phis.
  if (tail_call_possible(i)) {
    /// The actual tail call optimisation takes place in the code generator.
    i->call.tail_call = true;
    ir_mark_unreachable(i->parent_block);
    return true;
  }

  return false;
}

static bool opt_tail_call_elim(IRFunction *f) {
  bool changed = false;
  list_foreach (b, f->blocks) {
    list_foreach (i, b->instructions) {
      if (i->kind != IR_CALL) { continue; }

      /// We can’t have more than two tail calls in a single block.
      if (opt_try_convert_to_tail_call(i)) goto next_block;
    }
  next_block:;
  }
  return changed;
}

/// ===========================================================================
///  Mem2Reg
/// ===========================================================================
static bool opt_mem2reg(IRFunction *f) {
  bool changed = false;

  /// A stack variable.
  typedef struct {
    IRInstruction *alloca;
    IRInstruction *store;
    Vector(IRInstruction *) loads;
    bool unoptimisable;
  } stack_var;
  Vector(stack_var) vars = {0};

  /// Collect all stack variables that are stored into once, and
  /// whose address is never taken.
  list_foreach (b, f->blocks) {
    list_foreach (i, b->instructions) {
      switch (i->kind) {
        default: break;

        /// New variable.
        case IR_ALLOCA: {
          stack_var v = {0};
          v.alloca = i;
          vector_push(vars, v);
        } break;

        /// Record the first store into a variable.
        case IR_STORE: {
          foreach (a, vars) {
            if (!a->unoptimisable && a->alloca == i->store.addr) {
              /// If there are multiple stores, mark the variable as unoptimisable.
              if (a->store) a->unoptimisable = true;
              else a->store = i;
              break;
            }
          }
        } break;

        /// Record all loads; also check for loads before the first store.
        case IR_LOAD: {
          foreach (a, vars) {
            if (!a->unoptimisable && a->alloca == i->operand) {
              /// Load before store.
              if (!a->store) {
                a->unoptimisable = true;
                /// TODO: Proper warning once we have types in the IR.
                eprint("Warning: Load of uninitialised variable in function %S\n", f->name);
              } else {
                vector_push(a->loads, i);
              }
              break;
            }
          }
        } break;
      }
    }
  }

  /// Optimise all optimisable variables.
  foreach (a, vars) {
    /// If the variable is unoptimisable, do nothing.
    ///
    /// Since we don’t have `addressof` instructions or anything like
    /// that, check if the address is taken anywhere by checking if
    /// there are any uses of the alloca excepting the store and loads.
    if (a->unoptimisable || !a->store || a->alloca->users.size != a->loads.size + 1) {
      vector_delete(a->loads);
      continue;
    }

    /// If we get here, we can yeet the variable.
    changed = true;

    /// Replace all loads with the stored value.
    foreach_val (i, a->loads) {
      ir_replace_uses(i, a->store->store.value);
      ir_remove(i);
    }
    vector_delete(a->loads);

    /// Remove the store.
    ASSERT(a->store->users.size <= 1);
    vector_clear(a->store->users);
    ir_remove(a->store);

    /// Remove the alloca.
    ir_remove(a->alloca);
  }

  vector_delete(vars);
  return changed;
}

/// ===========================================================================
///  Analyse functions.
/// ===========================================================================
/// This function returns whether the pure attribute of the function has changed,
/// *not* whether it’s pure or not.
bool opt_check_pure(IRFunction *f) {
  /// Iterate over all instructions and check if they have nonlocal side effects.
  FOREACH_INSTRUCTION_IN_FUNCTION (f) {
    if (!has_side_effects(instruction)) continue;
    if (ir_is_branch(instruction)) continue;

    /// Even if an instruction in a function has side effects, the function
    /// may still be pure, e.g. if the instruction is a call to a pure function
    /// or a store to a local variable.
    switch (instruction->kind) {
      case IR_STORE:
        if (instruction->store.addr->kind == IR_ALLOCA) continue;
        break;
      case IR_CALL:
        if (!instruction->call.is_indirect && instruction->call.callee_function->attr_pure) continue;
        break;
      default: break;
    }

    /// Function is not pure.
    if (!f->attr_pure) return false;
    f->attr_pure = false;
    return true;
  }

  /// Function is pure.
  if (f->attr_pure) return false;
  f->attr_pure = true;
  return true;
}

/// Check if a function is a leaf function. This function returns whether
/// the leaf attribute of the function has changed, *not* whether it’s a
/// leaf or not.
bool opt_check_leaf(IRFunction *f) {
  /// A leaf function may not contain any calls except for recursive tail calls
  /// or tail calls to other leaf functions.
  FOREACH_INSTRUCTION_IN_FUNCTION (f) {
    if (instruction->kind != IR_CALL) continue;
    if (!instruction->call.is_indirect && instruction->call.tail_call) {
      IRFunction *callee = instruction->call.callee_function;
      if (callee == f || callee->attr_leaf) continue;
    }

    /// Function is not a leaf.
    if (!f->attr_leaf) return false;
    f->attr_leaf = false;
    return true;
  }

  /// Function is a leaf.
  if (f->attr_leaf) return false;
  f->attr_leaf = true;
  return true;
}

/// Check whether a function does not return. This function returns whether
/// the noreturn attribute of the function has changed, *not* whether it actually
/// returns or not.
///
/// If a function is marked as noreturn, then that means that it NEVER returns.
/// A function that only sometimes doesn’t return is *not* noreturn.
bool opt_check_noreturn(IRFunction *f) {
  FOREACH_INSTRUCTION_IN_FUNCTION (f) {
    /// A function that contains a tail call returns, unless the callee does not return.
    ///
    /// Checking regular calls doesn’t help, since we’re checking whether a function may
    /// return, not whether it might not return; tail calls, however, are different, since
    /// they are basically like return statements. We don’t care whether or not regular calls
    /// return, but if a tail call returns, then we must also return.
    if (instruction->kind == IR_CALL && instruction->call.tail_call) {
      /// If the call is a direct call, we can check the noreturn attribute of the callee.
      /// We can’t know whether an indirect call returns, so we must assume that we return.
      if (instruction->call.is_indirect || !instruction->call.callee_function->attr_noreturn) goto may_return;
    }

    /// If a return instruction is encountered, then this function obviously returns.
    else if (instruction->kind == IR_RETURN) {
    may_return:
      if (!f->attr_noreturn) return false;
      f->attr_noreturn = false;
      return true;
    }
  }

  /// Function does not return.
  if (f->attr_noreturn) return false;
  f->attr_noreturn = true;
  return true;
}

/// Check if a function is referenced by this instruction.
static void check_function_references(IRInstruction *inst) {
  STATIC_ASSERT(IR_COUNT == 39, "Handle all instructions that can reference a function");
  switch (inst->kind) {
    default: break;
    case IR_FUNC_REF: inst->function_ref->is_ever_referenced = true; break;
    case IR_CALL:
      if (!inst->call.is_indirect) inst->call.callee_function->is_ever_referenced = true;
      break;
  }
}

/// Analyse functions to determine whether they’re pure, leaf functions, etc.
bool opt_analyse_functions(CodegenContext *ctx) {
  bool ever_changed = false, changed;
  Vector(usz) removed = {0};
  do {
    changed = false;

    /// Check function attributes.
    foreach_val (f, ctx->functions) {
      if (f->is_extern) continue;
      f->is_ever_referenced = false;
      changed |= opt_check_pure(f);
      changed |= opt_check_leaf(f);
      changed |= opt_check_noreturn(f);
    }

    /// The entry point is always referenced.
    ctx->entry->is_ever_referenced = true;

    /// Check if the functions are ever referenced.
    FOREACH_INSTRUCTION (ctx) check_function_references(instruction);

    /// Also check in global variables and exports.
    foreach_val (var, ctx->static_vars)
      if (var->init)
        check_function_references(var->init);

    /// Free functions that are never referenced.
    vector_clear(removed);
    foreach_index (i, ctx->functions) {
      IRFunction *f = ctx->functions.data[i];
      if (f->is_ever_referenced) continue;
      changed = true;
      ir_free_function(f);
      vector_push(removed, i);
    }

    /// And remove them.
    foreach_rev (i, removed) vector_remove_index(ctx->functions, *i);

    if (changed) ever_changed = true;
  } while (changed);

  vector_delete(removed);
  return ever_changed;
}

/// ===========================================================================
///  Block reordering etc.
/// ===========================================================================
/// Rearrange the blocks in a function according to the dominator tree.
static void opt_reorder_blocks(IRFunction *f, DominatorInfo *info) {
  /// Clear the block list.
  f->blocks.first = NULL;
  f->blocks.last = NULL;

  /// Perform a preorder traversal of the dominator tree
  /// and reorder the blocks so that we can avoid jumps.
  Vector(DomTreeNode *) stack = {0};
  Vector(DomTreeNode *) visited = {0};
  vector_push(stack, info->dominator_tree);
  while (stack.size) {
    DomTreeNode *node = vector_pop(stack);
    list_push_back(f->blocks, node->block);

    /// If a block contains a direct branch or a conditional branch,
    /// we want to put the target block at the top of the stack so
    /// that it gets inserted directly after this block.
    IRBlock *next = NULL;
    IRInstruction *last = node->block->instructions.last;
    DomTreeNode *next_node = NULL;
    if (last->kind == IR_BRANCH) next = last->destination_block;
    else if (last->kind == IR_BRANCH_CONDITIONAL) next = last->cond_br.then;

    /// Insert all children except for the next node.
    foreach_val (child, node->children) {
      if (child->block == next) {
        next_node = child;
        continue;
      }
      if (!vector_contains(visited, child)) vector_push(stack, child);
    }

    /// Insert the next node if there is one.
    if (next_node) {
      if (!vector_contains(visited, next_node)) vector_push(stack, next_node);
    }
  }
  vector_delete(stack);
}

static bool opt_jump_threading(IRFunction *f, DominatorInfo *info) {
  bool changed = false;

  /// Avoid iterator invalidation.
  BlockVector blocks_to_remove = {0};

  /// Remove blocks that consist of a single direct branch.
  ///
  /// Also simplify conditional branches whose true and false
  /// blocks are the same.
  list_foreach (b, f->blocks) {
    IRInstruction *last = b->instructions.last;
    if (last == b->instructions.first && last->kind == IR_BRANCH) {
      /// Update any blocks that branch to this to branch to our
      /// target instead.
      list_foreach (b2, f->blocks) {
        if (b == b2) continue;

        STATIC_ASSERT(IR_COUNT == 39, "Handle all branch instructions");
        IRInstruction *branch = b2->instructions.last;
        if (branch->kind == IR_BRANCH && branch->destination_block == b) {
          branch->destination_block = last->destination_block;
          changed = true;
        } else if (branch->kind == IR_BRANCH_CONDITIONAL) {
          if (branch->cond_br.then == b) {
            branch->cond_br.then = last->destination_block;
            changed = true;
          }
          if (branch->cond_br.else_ == b) {
            branch->cond_br.else_ = last->destination_block;
            changed = true;
          }
        }

        /// Also update PHIs.
        list_foreach (i, b2->instructions) {
          if (i->kind == IR_PHI) {
            foreach_val (arg, i->phi_args) {
              if (arg->block == b) {
                arg->block = last->destination_block;
                changed = true;
              }
            }
          }
        }
      }

      if (!vector_contains(blocks_to_remove, b)) vector_push(blocks_to_remove, b);
      changed = true;
    }

    /// Simplify branches.
    else if (last->kind == IR_BRANCH_CONDITIONAL && last->cond_br.then == last->cond_br.else_) {
      last->kind = IR_BRANCH;
      ir_remove_use(last->cond_br.condition, last);
      last->destination_block = last->cond_br.then;
      changed = true;
    }
  }

  /// Remove the blocks.
  foreach_val (b, blocks_to_remove) {
    ir_remove_and_free_block(b);
  }

  /// Done.
  vector_delete(blocks_to_remove);
  return changed;
}

/// Foreach block, replace loads from a variable with the last value
/// stored to that variable in that block, if any.
static bool opt_store_forwarding(IRFunction *f) {
  Vector(struct var {
    IRInstruction *alloca;
    IRInstruction *store;
  }) vars = {0};
  bool changed = false;

  list_foreach (block, f->blocks) {
    vector_clear(vars);
    list_foreach (i, block->instructions) {
      switch (i->kind) {
        default: break;
        case IR_STORE: {
          if (i->store.addr->kind == IR_ALLOCA) {
            struct var *v = vector_find_if(el, vars, el->alloca == i->store.addr);
            if (v) {
              /// Eliminate the previous store if the address is never used.
              IRInstruction *ir = v->store->next;
              for (; ir && ir != i; ir = ir->next)
                if (vector_contains(v->alloca->users, ir))
                  break;
              if (ir == i) ir_remove(v->store);

              /// Update the store.
              v->store = i;
            } else vector_push(vars, ((struct var){i->store.addr, i}));
          }
        } break;

        case IR_LOAD: {
          struct var *v = vector_find_if(el, vars, el->alloca == i->operand);
          if (v) {
            ir_remove_use(v->store, i);
            ir_replace_uses(i, v->store->store.value);
            ir_remove(i);
            changed = true;
          }
        } break;
      }
    }
  }

  vector_delete(vars);
  return changed;
}

/// ===========================================================================
///  Driver
/// ===========================================================================
void codegen_optimise(CodegenContext *ctx) {
  opt_analyse_functions(ctx);

  /// Optimise each function individually.
  do {
    foreach_val (f, ctx->functions) {
      if (f->is_extern) continue;

      DominatorInfo dom = {0};
      do {
        build_dominator_tree(f, &dom, true);
        opt_reorder_blocks(f, &dom);
      } while (
        opt_instcombine(f) ||
        opt_dce(f) ||
        opt_mem2reg(f) ||
        opt_jump_threading(f, &dom) ||
        opt_store_forwarding(f) ||
        opt_tail_call_elim(f)
      );
      free_dominator_info(&dom);
    }
  }

  /// Cross-function optimisations.
  while (opt_inline(ctx, 20) || opt_analyse_functions(ctx));
}

/// Called after RA.
void codegen_optimise_blocks(CodegenContext *ctx) {
  foreach_val (f, ctx->functions) {
    if (f->is_extern) continue;

    DominatorInfo dom = {0};
    do {
      build_dominator_tree(f, &dom, true);
      opt_reorder_blocks(f, &dom);
    } while (opt_jump_threading(f, &dom));
    free_dominator_info(&dom);
  }
}