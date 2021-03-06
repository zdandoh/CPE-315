#include <iostream>
#include <iomanip>
#include <bitset>
#include "thumbsim.hpp"
// These are just the register NUMBERS
#define PC_REG 15  
#define LR_REG 14
#define SP_REG 13

// These are the contents of those registers
#define PC rf[PC_REG]
#define LR rf[LR_REG]
#define SP rf[SP_REG]

Stats stats;
Caches caches(0);

// CPE 315: you'll need to implement a custom sign-extension function
// in addition to the ones given below, specifically for the unconditional
// branch instruction, which has an 11-bit immediate field
unsigned int signExtend16to32ui(short i) {
  return static_cast<unsigned int>(static_cast<int>(i));
}

unsigned int signExtend8to32ui(char i) {
  return static_cast<unsigned int>(static_cast<int>(i));
}

int signExtend11to32ui(short i) {
  if(i & 1024) { // 1024 = 0000010000000000 - check if 11th bit is set
    i = i | 63488; // 63488 = 1111100000000000
  }
  return i;
}

void quit() {
  cout << "Unimplemented instruction" << endl;
  exit(1);
}

void printFlags() {
  cout << "Flags: Z: " << int(flags.Z) << ", N: " << int(flags.N) << endl;
}

// This is the global object you'll use to store condition codes N,Z,V,C
// Set these bits appropriately in execute below.
ASPR flags;

// CPE 315: You need to implement a function to set the Negative and Zero
// flags for each instruction that does that. It only needs to take
// one parameter as input, the result of whatever operation is executing
void setNegativeZeroFlags(int result) {
  if(result == 0) {
    flags.Z = 1;
  }
  else {
    flags.Z = 0;
  }

  if(result < 0) {
    flags.N = 1;
  }
  else {
    flags.N = 0;
  }
}

// This function is complete, you should not have to modify it
void setCarryOverflow (int num1, int num2, OFType oftype) {
  switch (oftype) {
    case OF_ADD:
      if (((unsigned long long int)num1 + (unsigned long long int)num2) ==
          ((unsigned int)num1 + (unsigned int)num2)) {
        flags.C = 0;
      }
      else {
        flags.C = 1;
      }
      if (((long long int)num1 + (long long int)num2) ==
          ((int)num1 + (int)num2)) {
        flags.V = 0;
      }
      else {
        flags.V = 1;
      }
      break;
    case OF_SUB:
      if (num1 >= num2) {
        flags.C = 1;
      }
      else if (((unsigned long long int)num1 - (unsigned long long int)num2) ==
          ((unsigned int)num1 - (unsigned int)num2)) {
        flags.C = 0;
      }
      else {
        flags.C = 1;
      }
      if (((num1==0) && (num2==0)) ||
          (((long long int)num1 - (long long int)num2) ==
           ((int)num1 - (int)num2))) {
        flags.V = 0;
      }
      else {
        flags.V = 1;
      }
      break;
    case OF_SHIFT:
      // C flag unaffected for shifts by zero
      if (num2 != 0) {
        if (((unsigned long long int)num1 << (unsigned long long int)num2) ==
            ((unsigned int)num1 << (unsigned int)num2)) {
          flags.C = 0;
        }
        else {
          flags.C = 1;
        }
      }
      // Shift doesn't set overflow
      break;
    default:
      cerr << "Bad OverFlow Type encountered." << __LINE__ << __FILE__ << endl;
      exit(1);
  }
}

// CPE 315: You're given the code for evaluating BEQ, and you'll need to 
// complete the rest of these conditions. See Page 208 of the armv7 manual
static int checkCondition(unsigned short cond) {
  switch(cond) {
    case EQ:
      if (flags.Z == 1) {
        return TRUE;
      }
      break;
    case NE:
      if (flags.Z == 0) {
        return TRUE;
      }
      break;
    case CS:
      if (flags.C == 1) {
        return TRUE;
      }
      break;
    case CC:
      if(flags.C == 0) {
        return TRUE;
      }
      break;
    case MI:
      if(flags.N == 1) {
        return TRUE;
      }
      break;
    case PL:
      if(flags.N == 0) {
        return TRUE;
      }
      break;
    case VS:
      if(flags.V == 1) {
        return TRUE;
      }
      break;
    case VC:
      if(flags.V == 0) {
        return TRUE;
      }
      break;
    case HI:
      if(flags.C == 1 && flags.Z == 0) {
        return TRUE;
      }
      break;
    case LS:
      if(flags.C == 0 || flags.Z == 1) {
        return TRUE;
      }
      break;
    case GE:
      if(flags.N == flags.V) {
        return TRUE;
      }
      break;
    case LT:
      if(flags.N != flags.V) {
        return TRUE;
      }
      break;
    case GT:
      if(flags.Z == 0 && flags.N == flags.V) {
        return TRUE;
      }
      break;
    case LE:
      if(flags.Z == 1 || flags.N != flags.V) {
        return TRUE;
      }
      break;
    case AL:
      return TRUE;
      break;
  }
  return FALSE;
}

void elapseCycle(int cycle_count) {
  for(int k = 0; k < cycle_count; k++) {
    for(int i = 0; i < 16; i++) {
      if(stats.delays[i] > 0 ){
        stats.delays[i] -= 1;
      }
    }
    stats.cycles++;
  }
}

void registerInst(string type, int dest, int dep1, int dep2, int dep3 = -1) {
  int instructionCycles = 0;

  int delay1 = 0;
  int delay2 = 0;
  int delay3 = 0;
  if(dep1 > -1) {
    delay1 = stats.delays[dep1];
  }
  if(dep2 > -1) {
    delay2 = stats.delays[dep2];
  }
  if(dep3 > -1) {
    delay3 = stats.delays[dep3];
  }

  // calculate the needed delay for the source registers
  int delayCount = max(delay1, delay2);
  delayCount = max(delayCount, delay3);

  elapseCycle(delayCount); // stall if there are previous data dependencies that need resolving

  if(type == "math") {
    // some math instructions do not have a result, but if they do it is a 1 cycle delay, which will elapse by
    // the time the next instruction is executed
    instructionCycles = 1;
  }
  else if(type == "uncond") {
    instructionCycles = 0;
  }
  else if(type == "bl") {
    instructionCycles = 1;
  }
  else if(type == "taken") {
    instructionCycles = 5; // branch predictor assumes untaken
  }
  else if(type == "untaken") {
    instructionCycles = 1; // if branch isn't taken
  }
  else if(type == "load") { // since there are no ldms/stms in any of the simulations > 2 registers, this works for them too.
    instructionCycles = 1;
    stats.delays[dest] = 4; // the delay needs to be 4 because it has a latency of 3 and one cycle immediately elapses afterwards
  }
  else if(type == "store") {
    instructionCycles = 1;
  }
  else if(type == "canread") {
    instructionCycles = 0;
  }
  else if(type == "writereg") {
    instructionCycles = 0;
    stats.delays[dest] = 4;
  }
  else if(type == "ldmstm") {
    instructionCycles = 1;
  }
  else if(type == "ldrl") {
    instructionCycles = 4;
  }
  else {
    cout << "Unknown inst type: " << type << endl;
    exit(1);
  }

  elapseCycle(instructionCycles);
}

void execute() {
  // DEBUG
  // for(int i = 0; i < 16; i++) {
  //   cout << i << ": " << setbase(10) << (int)rf[i] << ", ";
  // }
  // cout << endl;
  // getchar();

  Data16 instr = imem[PC];
  Data16 instr2;
  Data32 temp(0); // Use this for STRB instructions
  Thumb_Types itype;
  // the following counts as a read to PC
  unsigned int pctarget = PC + 2;
  unsigned int addr;
  int i, n, offset;
  unsigned int list, mask;
  int num1, num2, result, BitCount;
  unsigned int bit;

  /* Convert instruction to correct type */
  /* Types are described in Section A5 of the armv7 manual */
  BL_Type blupper(instr);
  ALU_Type alu(instr);
  SP_Type sp(instr);
  DP_Type dp(instr);
  LD_ST_Type ld_st(instr);
  MISC_Type misc(instr);
  COND_Type cond(instr);
  UNCOND_Type uncond(instr);
  LDM_Type ldm(instr);
  STM_Type stm(instr);
  LDRL_Type ldrl(instr);
  ADD_SP_Type addsp(instr);

  BL_Ops bl_ops;
  ALU_Ops add_ops;
  DP_Ops dp_ops;
  SP_Ops sp_ops;
  LD_ST_Ops ldst_ops;
  MISC_Ops misc_ops;

  // This counts as a write to the PC register
  rf.write(PC_REG, pctarget);
  stats.numRegReads++;
  stats.numRegWrites++;

  itype = decode(ALL_Types(instr));

  // CPE 315: The bulk of your work is in the following switch statement
  // All instructions will need to have stats and cache access info added
  // as appropriate for that instruction.
  stats.instrs++;
  switch(itype) {
    case ALU:
      add_ops = decode(alu);
      switch(add_ops) {
        case ALU_LSLI:
          rf.write(alu.instr.lsli.rd, rf[alu.instr.lsli.rm] << alu.instr.lsli.imm);
          setCarryOverflow(rf[alu.instr.lsli.rm], rf[alu.instr.lsli.imm], OF_SHIFT);
          setNegativeZeroFlags(rf[alu.instr.lsli.rm] << alu.instr.lsli.imm);
          stats.numRegReads++;
          stats.numRegWrites++;
          registerInst("math", alu.instr.lsli.rd, alu.instr.lsli.rm, -1);
          break;
        case ALU_ADDR:
          // needs stats and flags
          rf.write(alu.instr.addr.rd, rf[alu.instr.addr.rn] + rf[alu.instr.addr.rm]);
          setCarryOverflow(rf[alu.instr.addr.rn], rf[alu.instr.addr.rm], OF_ADD);
          setNegativeZeroFlags(rf[alu.instr.addr.rn] + rf[alu.instr.addr.rm]);
          stats.numRegReads += 2;
          stats.numRegWrites++;
          registerInst("math", alu.instr.addr.rd, alu.instr.addr.rn, alu.instr.addr.rm);
          break;
        case ALU_SUBR:
          rf.write(alu.instr.subr.rd, rf[alu.instr.subr.rn] - rf[alu.instr.subr.rm]);
          setCarryOverflow(rf[alu.instr.subr.rn], rf[alu.instr.subr.rm], OF_SUB);
          setNegativeZeroFlags(rf[alu.instr.subr.rn] - rf[alu.instr.subr.rm]);
          stats.numRegReads += 2;
          stats.numRegWrites++;
          registerInst("math", alu.instr.subr.rd, alu.instr.subr.rn, alu.instr.subr.rm);
          break;
        case ALU_ADD3I:
          // needs stats and flags
          rf.write(alu.instr.add3i.rd, rf[alu.instr.add3i.rn] + alu.instr.add3i.imm);
          setCarryOverflow(rf[alu.instr.add3i.rn], alu.instr.add3i.imm, OF_ADD);
          setNegativeZeroFlags(rf[alu.instr.add3i.rn] + alu.instr.add3i.imm);
          stats.numRegReads++;
          stats.numRegWrites++;
          registerInst("math", alu.instr.add3i.rd, alu.instr.add3i.rn, -1);
          break;
        case ALU_SUB3I:
          rf.write(alu.instr.sub3i.rd, rf[alu.instr.sub3i.rn] - alu.instr.sub3i.imm);
          setCarryOverflow(rf[alu.instr.sub3i.rn], alu.instr.sub3i.imm, OF_SUB);
          setNegativeZeroFlags(rf[alu.instr.sub3i.rn] - alu.instr.sub3i.imm);
          stats.numRegReads++;
          stats.numRegWrites++;
          registerInst("math", alu.instr.sub3i.rd, alu.instr.sub3i.rn, -1);
          break;
        case ALU_MOV:
          // needs stats and flags
          rf.write(alu.instr.mov.rdn, alu.instr.mov.imm);
          setNegativeZeroFlags(alu.instr.mov.imm);
          setCarryOverflow(rf[alu.instr.mov.rdn], alu.instr.mov.imm, OF_SUB);
          stats.numRegWrites++;
          registerInst("math", alu.instr.mov.rdn, -1, -1);
          break;
        case ALU_CMP:
          setCarryOverflow(rf[alu.instr.cmp.rdn], alu.instr.cmp.imm, OF_SUB);
          setNegativeZeroFlags(rf[alu.instr.cmp.rdn] - alu.instr.cmp.imm);
          stats.numRegReads++;
          registerInst("math", alu.instr.cmp.rdn, -1, -1);
          break;
        case ALU_ADD8I:
          // needs stats and flags
          rf.write(alu.instr.add8i.rdn, rf[alu.instr.add8i.rdn] + alu.instr.add8i.imm);
          setCarryOverflow(rf[alu.instr.add8i.rdn], alu.instr.add8i.imm, OF_ADD);
          setNegativeZeroFlags(rf[alu.instr.add8i.rdn] + alu.instr.add8i.imm);
          stats.numRegReads++;
          stats.numRegWrites++;
          stats.cycles++;
          registerInst("math", alu.instr.add8i.rdn, alu.instr.add8i.rdn, -1);
          break;
        case ALU_SUB8I:
          rf.write(alu.instr.sub8i.rdn, rf[alu.instr.sub8i.rdn] - alu.instr.sub8i.imm);
          setCarryOverflow(rf[alu.instr.sub8i.rdn], alu.instr.sub8i.imm, OF_SUB);
          setNegativeZeroFlags(rf[alu.instr.sub8i.rdn] + alu.instr.sub8i.imm);
          stats.numRegReads++;
          stats.numRegWrites++;
          stats.cycles++;
          registerInst("math", alu.instr.sub8i.rdn, alu.instr.sub8i.rdn, -1);
          break;
        default:
          cout << "instruction not implemented" << endl;
          exit(1);
          break;
      }
      break;
    case BL: 
      // This instruction is complete, nothing needed here
      bl_ops = decode(blupper);
      if (bl_ops == BL_UPPER) {
        // PC has already been incremented above
        instr2 = imem[PC];
        BL_Type bllower(instr2);
        if (blupper.instr.bl_upper.s) {
          addr = static_cast<unsigned int>(0xff<<24) | 
            ((~(bllower.instr.bl_lower.j1 ^ blupper.instr.bl_upper.s))<<23) |
            ((~(bllower.instr.bl_lower.j2 ^ blupper.instr.bl_upper.s))<<22) |
            ((blupper.instr.bl_upper.imm10)<<12) |
            ((bllower.instr.bl_lower.imm11)<<1);
        }
        else {
          addr = ((blupper.instr.bl_upper.imm10)<<12) |
            ((bllower.instr.bl_lower.imm11)<<1);
        }
        // return address is 4-bytes away from the start of the BL insn
        rf.write(LR_REG, PC + 2);
        // Target address is also computed from that point
        rf.write(PC_REG, PC + 2 + addr);

        stats.numRegReads += 1;
        stats.numRegWrites += 2;
        registerInst("bl", -1, -1, -1);
      }
      else {
        cerr << "Bad BL format." << endl;
        exit(1);
      }
      break;
    case DP:
      dp_ops = decode(dp);
      switch(dp_ops) {
        case DP_CMP:
          setCarryOverflow(rf[dp.instr.DP_Instr.rdn], rf[dp.instr.DP_Instr.rm], OF_SUB);
          setNegativeZeroFlags(rf[dp.instr.DP_Instr.rdn] - rf[dp.instr.DP_Instr.rm]);
          stats.numRegReads += 2;
          registerInst("math", -1, dp.instr.DP_Instr.rdn, dp.instr.DP_Instr.rm);
          break;
      }
      break;
    case SPECIAL:
      sp_ops = decode(sp);
      switch(sp_ops) {
        case SP_MOV:
          // needs stats and flags
          rf.write((sp.instr.mov.d << 3 ) | sp.instr.mov.rd, rf[sp.instr.mov.rm]);
          setNegativeZeroFlags(rf[sp.instr.mov.rm]);
          stats.numRegReads++;
          stats.numRegWrites++;
          registerInst("math", -1, dp.instr.DP_Instr.rdn, dp.instr.DP_Instr.rm);
          break;
        case SP_ADD:
          {
            int rd = sp.instr.add.rd;
            if(sp.instr.add.d) {
              rd += 8;
            }

            rf.write(rd, rf[rd] + rf[sp.instr.add.rm]);
            setNegativeZeroFlags(rf[rd] + rf[sp.instr.add.rm]);
            setCarryOverflow(rf[rd], rf[sp.instr.add.rm], OF_ADD);
            stats.numRegWrites++;
            stats.numRegReads += 2;
            registerInst("math", rd, rd, sp.instr.add.rm);
          }
          break;
        case SP_CMP:
          // need to implement these
          {
            int rd = sp.instr.cmp.rd;
            if(sp.instr.cmp.d) {
              rd += 8;
            }

            setNegativeZeroFlags(rf[rd] - rf[sp.instr.cmp.rm]);
            setCarryOverflow(rf[rd], rf[sp.instr.add.rm], OF_SUB);
            stats.numRegReads += 2;
            registerInst("math", -1, rd, sp.instr.cmp.rm);
          }
          break;
      }
      break;
    case LD_ST:
      // You'll want to use these load and store models
      // to implement ldrb/strb, ldm/stm and push/pop
      ldst_ops = decode(ld_st);
      switch(ldst_ops) {
        case STRI:
          // functionally complete, needs stats
          addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm * 4;
          dmem.write(addr, rf[ld_st.instr.ld_st_imm.rt]);
          caches.access(addr);
          stats.numMemWrites++;
          stats.numRegReads += 2;
          registerInst("store", -1, ld_st.instr.ld_st_imm.rn, ld_st.instr.ld_st_imm.rt);
          break;
        case LDRI:
          // functionally complete, needs stats
          addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm * 4;
          rf.write(ld_st.instr.ld_st_imm.rt, dmem[addr]);
          caches.access(addr);
          stats.numMemReads++;
          stats.numRegReads++;
          stats.numRegWrites++;
          registerInst("load", ld_st.instr.ld_st_imm.rt, ld_st.instr.ld_st_imm.rn, -1);
          break;
        case STRR:
          // need to implement
          addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];
          dmem.write(addr, rf[ld_st.instr.ld_st_reg.rt]);
          caches.access(addr);
          stats.numMemWrites++;
          stats.numRegReads += 3;
          registerInst("store", -1, ld_st.instr.ld_st_reg.rn, ld_st.instr.ld_st_reg.rm, ld_st.instr.ld_st_reg.rt);
          break;
        case LDRR:
          // need to implement
          addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];
          rf.write(ld_st.instr.ld_st_reg.rt, dmem[addr]);
          caches.access(addr);
          stats.numMemReads++;
          stats.numRegReads += 2;
          stats.numRegWrites++;
          registerInst("load", ld_st.instr.ld_st_reg.rt, ld_st.instr.ld_st_reg.rn, ld_st.instr.ld_st_reg.rm);
          break;
        case STRBI:
          // need to implement
          {
            addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm;
            unsigned int data = dmem[addr];
            data = data & 0x00FFFFFF; // clear the top 8 bits
            data = data | (rf[ld_st.instr.ld_st_imm.rt] << 24); // modify the top 8 bits
            dmem.write(addr, data);
            caches.access(addr);
            stats.numRegReads += 2;
            stats.numMemWrites++;
            registerInst("store", -1, ld_st.instr.ld_st_imm.rn, ld_st.instr.ld_st_imm.rt);
          }
          break;
        case LDRBI:
          // need to implement
          {
            addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm;
            unsigned int data = dmem[addr] >> 24;
            caches.access(addr);
            rf.write(ld_st.instr.ld_st_imm.rt, data);
            stats.numMemReads++;
            stats.numRegReads++;
            stats.numRegWrites++;
            registerInst("load", ld_st.instr.ld_st_imm.rt, ld_st.instr.ld_st_imm.rn, -1);
          }
          break;
        case STRBR:
          // need to implement
          {
            int addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];
            unsigned int data = dmem[addr];
            data = data & 0x00FFFFFF; // clear the top 8 bits
            data = data | (rf[ld_st.instr.ld_st_reg.rt] << 24); // modify the top 8 bits
            dmem.write(addr, data);
            caches.access(addr);
            stats.numRegReads += 3;
            stats.numMemWrites++;
            registerInst("store", -1, ld_st.instr.ld_st_reg.rn, ld_st.instr.ld_st_reg.rm, ld_st.instr.ld_st_reg.rt);
          }
          break;
        case LDRBR:
          // need to implement
          {
            int addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];
            unsigned int data = dmem[addr] >> 24;
            caches.access(addr);
            rf.write(ld_st.instr.ld_st_reg.rt, data);
            stats.numMemReads++;
            stats.numRegReads += 2;
            stats.numRegWrites++;
            registerInst("load", ld_st.instr.ld_st_reg.rt, ld_st.instr.ld_st_reg.rn, ld_st.instr.ld_st_reg.rm);
          }
          break;
      }
      break;
    case MISC:
      misc_ops = decode(misc);
      switch(misc_ops) {
        case MISC_PUSH:
          {
            // Calculate initial address
            std::bitset<8> bc(misc.instr.push.reg_list);
            int addr = bc.count();
            if(misc.instr.push.m) {
              addr++;
            }
            addr *= 4;
            addr = SP - addr;
            stats.numRegReads++;
            registerInst("canread", -1, SP_REG, -1);
            int cp_addr = addr;
            
            for(int i = 0; i < 8; i++) {
              if(misc.instr.push.reg_list & (1 << i)) {
                dmem.write(addr, rf[i]);
                caches.access(addr);
                addr += 4;
                stats.numMemWrites++;
                stats.numRegReads++;
                registerInst("store", -1, i, -1);
              }
            }
            
            if(misc.instr.push.m) {
              dmem.write(addr, LR);
              caches.access(addr);
              stats.numMemWrites++;
              stats.numRegReads++;
              registerInst("store", -1, LR_REG, -1);
            }

            rf.write(SP_REG, cp_addr);
            stats.numRegWrites++;
            registerInst("store", -1, SP_REG, -1);
          }
          // need to implement
          break;
        case MISC_POP:
          // need to implement
          {
            // Calculate initial address
            int addr = SP;
            registerInst("canread", -1, SP_REG, -1);
            stats.numRegReads++;
            
            for(int i = 0; i < 8; i++) {
              if(misc.instr.pop.reg_list & (1 << i)) {
                rf.write(i, dmem[addr]);
                caches.access(addr);
                addr += 4;
                stats.numMemReads++;
                stats.numRegWrites++;
                registerInst("load", i, -1, -1);
              }
            }
            
            if(misc.instr.push.m) {
              rf.write(PC_REG, dmem[addr]);
              caches.access(addr);
              addr += 4;
              stats.numMemReads++;
              stats.numRegWrites++;
              registerInst("load", PC_REG, -1, -1);
            }

            rf.write(SP_REG, addr);
            stats.numRegWrites++;
            registerInst("store", -1, SP_REG, -1);
          }
          break;
        case MISC_SUB:
          // functionally complete, needs stats
          rf.write(SP_REG, SP - (misc.instr.sub.imm*4));
          stats.numRegWrites++;
          stats.numRegReads++;
          registerInst("math", SP_REG, SP_REG, -1);
          break;
        case MISC_ADD:
          // functionally complete, needs stats
          rf.write(SP_REG, SP + (misc.instr.add.imm*4));
          stats.numRegWrites++;
          stats.numRegReads++;
          registerInst("math", SP_REG, SP_REG, -1);
          break;
      }
      break;
    case COND:
      decode(cond);
      // Once you've completed the checkCondition function,
      // this should work for all your conditional branches.
      // needs stats
      {
        int taken = 0;
        int not_taken = 1;
        if (checkCondition(cond.instr.b.cond)){
          rf.write(PC_REG, PC + 2 * signExtend8to32ui(cond.instr.b.imm) + 2);
          stats.numRegWrites++;
          stats.numRegReads++;
          taken = 1;
          not_taken = 0;
          registerInst("taken", -1, -1, -1);
        } else {
          registerInst("untaken", -1, -1, -1);
        }

        if((int)signExtend8to32ui(cond.instr.b.imm) > 0) {
          stats.numForwardBranchesTaken += taken;
          stats.numForwardBranchesNotTaken += not_taken;
        }
        else {
          stats.numBackwardBranchesTaken += taken;
          stats.numBackwardBranchesNotTaken += not_taken;
        }
      }
      break;
    case UNCOND:
      // Essentially the same as the conditional branches, but with no
      // condition check, and an 11-bit immediate field
      decode(uncond);
      rf.write(PC_REG, PC + 2 * signExtend11to32ui(uncond.instr.b.imm) + 2);
      stats.numRegReads++;
      stats.numRegWrites++;
      registerInst("uncond", -1, -1, -1);
      break;
    case LDM:
      decode(ldm);
      // need to implement
      addr = rf[ldm.instr.ldm.rn];
      stats.numRegReads++;
      for(int i = 0; i < 8; i++) {
        if(ldm.instr.ldm.reg_list & (1 << i)) {
          rf.write(i, dmem[addr]);
          stats.numRegWrites++;
          stats.numMemReads++;
          caches.access(addr);
          addr += 4;
          registerInst("writereg", i, -1, -1);
        }
      }
      rf.write(ldm.instr.ldm.rn, addr);
      stats.numRegWrites++;
      registerInst("ldmstm", -1, -1, -1);
      break;
    case STM:
      decode(stm);
      
      addr = rf[ldm.instr.ldm.rn];
      stats.numRegReads++;
      for(int i = 0; i < 8; i++) {
        if(ldm.instr.ldm.reg_list & (1 << i)) {
          dmem.write(addr, rf[i]);
          caches.access(addr);
          stats.numRegReads++;
          stats.numMemWrites++;
          addr += 4;
          registerInst("canread", -1, i, -1);
        }
      }
      rf.write(ldm.instr.ldm.rn, addr);
      registerInst("ldmstm", -1, -1, -1);
      stats.numRegWrites++;
      break;
    case LDRL:
      // This instruction is complete, nothing needed
      decode(ldrl);
      // Need to check for alignment by 4
      if (PC & 2) {
        addr = PC + 2 + (ldrl.instr.ldrl.imm)*4;
      }
      else {
        addr = PC + (ldrl.instr.ldrl.imm)*4;
      }
      // Requires two consecutive imem locations pieced together
      temp = imem[addr] | (imem[addr+2]<<16);  // temp is a Data32
      rf.write(ldrl.instr.ldrl.rt, temp);

      // One write for updated reg
      stats.numRegWrites++;
      // One read of the PC
      stats.numRegReads++;
      // One mem read, even though it's imem, and there's two of them
      stats.numMemReads++;
      registerInst("ldrl", PC_REG, ldrl.instr.ldrl.rt, -1);
      break;
    case ADD_SP:
      // needs stats
      decode(addsp);
      rf.write(addsp.instr.add.rd, SP + (addsp.instr.add.imm*4));
      setNegativeZeroFlags(SP + (addsp.instr.add.imm*4));
      setCarryOverflow(SP, addsp.instr.add.imm*4, OF_ADD);
      stats.numRegWrites++;
      stats.numRegReads++;
      registerInst("math", addsp.instr.add.rd, SP_REG, -1);
      break;
    default:
      cout << "[ERROR] Unknown Instruction to be executed" << endl;
      exit(1);
      break;
  }
}
