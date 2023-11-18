#include <lcc/codegen/x86_64.hh>

#include <lcc/codegen/mir.hh>

namespace lcc {
namespace x86_64 {

std::string ToString(Opcode op) {
    switch (op) {
        case Opcode::Poison: return "x86_64.poison";
        case Opcode::Return: return "ret";
        case Opcode::Jump: return "jmp";
        case Opcode::Call: return "call";
        case Opcode::MoveDereferenceLHS:
        case Opcode::MoveDereferenceRHS:
        case Opcode::Move: return "mov";
        case Opcode::MoveSignExtended: return "movsx";
        case Opcode::LoadEffectiveAddress: return "lea";
        case Opcode::Add: return "add";
        case Opcode::Multiply: return "mul";
        case Opcode::Sub: return "sub";
        case Opcode::Push: return "push";
        case Opcode::Pop: return "pop";
        case Opcode::Test: return "test";
        case Opcode::JumpIfZeroFlag: return "jz";
        case Opcode::Compare: return "cmp";
        case Opcode::SetByteIfEqual: return "sete";
        case Opcode::SetByteIfLessUnsigned: return "setb";
        case Opcode::SetByteIfLessSigned: return "setl";
        case Opcode::SetByteIfGreaterUnsigned: return "seta";
        case Opcode::SetByteIfGreaterSigned: return "setg";
        case Opcode::SetByteIfEqualOrLessUnsigned: return "setbe";
        case Opcode::SetByteIfEqualOrLessSigned: return "setle";
        case Opcode::SetByteIfEqualOrGreaterUnsigned: return "setae";
        case Opcode::SetByteIfEqualOrGreaterSigned: return "setge";
    }
    LCC_UNREACHABLE();
}

std::string opcode_to_string(usz opcode) {
    if (opcode >= +MInst::Kind::ArchStart)
        return ToString(static_cast<Opcode>(opcode));
    return MInstOpcodeToString(opcode);
}

std::string ToString(RegisterId id, usz size) {
    if (not size) size = 64;
    switch (id) {
        case RegisterId::INVALID: return "x86_64.INVALID";
        case RegisterId::RAX: {
            if (size == 64) return "rax";
            if (size == 32) return "eax";
            if (size == 16) return "ax";
            if (size == 8) return "al";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::RBX: {
            if (size == 64) return "rbx";
            if (size == 32) return "ebx";
            if (size == 16) return "bx";
            if (size == 8) return "bl";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::RCX: {
            if (size == 64) return "rcx";
            if (size == 32) return "ecx";
            if (size == 16) return "cx";
            if (size == 8) return "cl";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::RDX: {
            if (size == 64) return "rdx";
            if (size == 32) return "edx";
            if (size == 16) return "dx";
            if (size == 8) return "dl";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::R8: {
            if (size == 64) return "r8";
            if (size == 32) return "r8d";
            if (size == 16) return "r8w";
            if (size == 8) return "r8b";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::R9: {
            if (size == 64) return "r9";
            if (size == 32) return "r9d";
            if (size == 16) return "r9w";
            if (size == 8) return "r9b";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::R10: {
            if (size == 64) return "r10";
            if (size == 32) return "r10d";
            if (size == 16) return "r10w";
            if (size == 8) return "r10b";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::R11: {
            if (size == 64) return "r11";
            if (size == 32) return "r11d";
            if (size == 16) return "r11w";
            if (size == 8) return "r11b";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::R12: {
            if (size == 64) return "r12";
            if (size == 32) return "r12d";
            if (size == 16) return "12w";
            if (size == 8) return "r12b";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::R13: {
            if (size == 64) return "r13";
            if (size == 32) return "r13d";
            if (size == 16) return "r13w";
            if (size == 8) return "r13b";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::R14: {
            if (size == 64) return "r14";
            if (size == 32) return "r14d";
            if (size == 16) return "r14w";
            if (size == 8) return "r14b";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::R15: {
            if (size == 64) return "r15";
            if (size == 32) return "r15w";
            if (size == 16) return "r15d";
            if (size == 8) return "r15b";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::RDI: {
            if (size == 64) return "rdi";
            if (size == 32) return "edi";
            if (size == 16) return "di";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::RSI: {
            if (size == 64) return "rsi";
            if (size == 32) return "esi";
            if (size == 16) return "si";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::RBP: {
            if (size == 64) return "rbp";
            if (size == 32) return "ebp";
            if (size == 16) return "bp";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::RSP: {
            if (size == 64) return "rsp";
            if (size == 32) return "esp";
            if (size == 16) return "sp";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::RIP: {
            if (size == 64) return "rip";
            if (size == 32) return "eip";
            if (size == 16) return "ip";
            LCC_ASSERT(false, "Invalid size: {}", size);
        }
        case RegisterId::RETURN: return "x86_64.RETURN";
    }
    LCC_UNREACHABLE();
}

std::string ToString(RegisterId id) {
    return ToString(id, 64);
}

} // namespace x86_64
} // namespace lcc
