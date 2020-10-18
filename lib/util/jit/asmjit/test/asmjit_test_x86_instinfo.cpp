// AsmJit - Machine code generation for C++
//
//  * Official AsmJit Home Page: https://asmjit.com
//  * Official Github Repository: https://github.com/asmjit/asmjit
//
// Copyright (c) 2008-2020 The AsmJit Authors
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include <asmjit/core.h>

#if defined(ASMJIT_BUILD_X86)
#include <asmjit/x86.h>
#endif

#include <stdio.h>

using namespace asmjit;

static char accessLetter(bool r, bool w) noexcept {
  return r && w ? 'X' : r ? 'R' : w ? 'W' : '_';
}

static void printInfo(uint32_t arch, const BaseInst& inst, const Operand_* operands, size_t opCount) {
  StringTmp<512> sb;

  // Read & Write Information
  // ------------------------

  InstRWInfo rw;
  InstAPI::queryRWInfo(arch, inst, operands, opCount, &rw);

  sb.append("Instruction:\n");
  sb.append("  ");
#ifndef ASMJIT_NO_LOGGING
  Formatter::formatInstruction(sb, 0, nullptr, arch, inst, operands, opCount);
#else
  sb.append("<Logging-Not-Available>");
#endif
  sb.append("\n");

  sb.append("Operands:\n");
  for (uint32_t i = 0; i < rw.opCount(); i++) {
    const OpRWInfo& op = rw.operand(i);

    sb.appendFormat("  [%u] Op=%c Read=%016llX Write=%016llX Extend=%016llX",
                    i,
                    accessLetter(op.isRead(), op.isWrite()),
                    op.readByteMask(),
                    op.writeByteMask(),
                    op.extendByteMask());

    if (op.isMemBaseUsed()) {
      sb.appendFormat(" Base=%c", accessLetter(op.isMemBaseRead(), op.isMemBaseWrite()));
      if (op.isMemBasePreModify())
        sb.appendFormat(" <PRE>");
      if (op.isMemBasePostModify())
        sb.appendFormat(" <POST>");
    }

    if (op.isMemIndexUsed()) {
      sb.appendFormat(" Index=%c", accessLetter(op.isMemIndexRead(), op.isMemIndexWrite()));
    }

    sb.append("\n");
  }

  if (rw.readFlags() | rw.writeFlags()) {
    sb.append("Flags: \n");

    struct FlagMap {
      uint32_t flag;
      char name[4];
    };

    static const FlagMap flagMap[] = {
      { x86::Status::kCF, "CF" },
      { x86::Status::kOF, "OF" },
      { x86::Status::kSF, "SF" },
      { x86::Status::kZF, "ZF" },
      { x86::Status::kAF, "AF" },
      { x86::Status::kPF, "PF" },
      { x86::Status::kDF, "DF" },
      { x86::Status::kIF, "IF" },
      { x86::Status::kAC, "AC" },
      { x86::Status::kC0, "C0" },
      { x86::Status::kC1, "C1" },
      { x86::Status::kC2, "C2" },
      { x86::Status::kC3, "C3" }
    };

    sb.append("  ");
    for (uint32_t f = 0; f < 13; f++) {
      char c = accessLetter((rw.readFlags() & flagMap[f].flag) != 0,
                            (rw.writeFlags() & flagMap[f].flag) != 0);
      if (c != '_')
        sb.appendFormat("%s=%c ", flagMap[f].name, c);
    }

    sb.append("\n");
  }

  // CPU Features
  // ------------

  BaseFeatures features;
  InstAPI::queryFeatures(arch, inst, operands, opCount, &features);

#ifndef ASMJIT_NO_LOGGING
  if (!features.empty()) {
    sb.append("Features:\n");
    sb.append("  ");

    bool first = true;
    BaseFeatures::Iterator it(features.iterator());
    while (it.hasNext()) {
      uint32_t featureId = uint32_t(it.next());
      if (!first)
        sb.append(" & ");
      Formatter::formatFeature(sb, arch, featureId);
      first = false;
    }
    sb.append("\n");
  }
#endif

  printf("%s\n", sb.data());
}

template<typename... Args>
static void printInfoSimple(uint32_t arch, uint32_t instId, Args&&... args) {
  BaseInst inst(instId);
  Operand_ opArray[] = { std::forward<Args>(args)... };
  printInfo(arch, inst, opArray, sizeof...(args));
}

static void testX86Arch() {
#if defined(ASMJIT_BUILD_X86)
  uint32_t arch = Environment::kArchX64;

  printInfoSimple(arch,
                  x86::Inst::kIdAdd,
                  x86::eax, x86::ebx);

  printInfoSimple(arch,
                  x86::Inst::kIdLods,
                  x86::eax , dword_ptr(x86::rsi));

  printInfoSimple(arch,
                  x86::Inst::kIdPshufd,
                  x86::xmm0, x86::xmm1, imm(0));

  printInfoSimple(arch,
                  x86::Inst::kIdPextrw,
                  x86::eax, x86::xmm1, imm(0));

  printInfoSimple(arch,
                  x86::Inst::kIdPextrw,
                  x86::ptr(x86::rax), x86::xmm1, imm(0));

  printInfoSimple(arch,
                  x86::Inst::kIdVaddpd,
                  x86::ymm0, x86::ymm1, x86::ymm2);

  printInfoSimple(arch,
                  x86::Inst::kIdVaddpd,
                  x86::ymm0, x86::ymm30, x86::ymm31);

  printInfoSimple(arch,
                  x86::Inst::kIdVaddpd,
                  x86::zmm0, x86::zmm1, x86::zmm2);
#endif
}

int main() {
  printf("AsmJit Instruction Information Test\n\n");

  testX86Arch();

  return 0;
}
