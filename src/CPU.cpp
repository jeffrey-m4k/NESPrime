#include <algorithm>
#include <iostream>
#include <iomanip>
#include "util.h"
#include "CPU.h"
#include "NES.h"
#include "PPU.h"

CPU::CPU() : Processor() {
    this->CPU::reset();
}

void CPU::reset() {
    // Clear the address space and map the CPU's 2KB memory from 0x0000 to 0x2000
    this->aspace.clear();
    for (int i = 0; i < 4; i++) {
        map(static_cast<uint16_t>(i*0x800), this->mem.get_mem(), 0x800);
    }
}

void CPU::init() {
    cycle = 7;
    nes->out.fill('0');
    reg.pc = read(0xFFFD) << 8 | read(0xFFFC);
    //reg.pc = 0xC000;// uncomment for nestest
    reg.p = 0x24; // Initialize STATUS register to 00100100
    reg.s = 0xFD; // Initialize stack head pointer
    nes->out << "\n\n===== RUNNING =====";
}

bool CPU::run() {
    if (!this->Processor::run()) return false;
    std::ostringstream regs_log;
    regs_log << "A:"; print_hex(regs_log, reg.acc);
    regs_log << " X:"; print_hex(regs_log,reg.x);
    regs_log << " Y:"; print_hex(regs_log,reg.y);
    regs_log << " P:"; print_hex(regs_log,reg.p);
    regs_log << " SP:"; print_hex(regs_log,reg.s);
    regs_log << " CYC:" << cycle;
    regs_log << " | PPU:";
    for (int i = 0; i < 9; i++) {
        print_hex(regs_log, nes->get_ppu()->read_reg(i, cycle)); regs_log << " ";
    }
    regs_log << "CYC:" << nes->get_ppu()->get_cycle() << " ";
    regs_log << "| CLOCK:" << nes->get_clock();

    nes->out << "\n";
    uint8_t opcode = read(reg.pc);
    print_hex(nes->out, reg.pc, "  ");
    exec(opcode);

    nes->out << setw(35) << std::left << regs_log.str();

    idle_cycles--;
    cycle++;
    return true;
}

void CPU::exec(const uint8_t opcode) {
    oss.str("");

    auto it = OPCODES.find(opcode);
    if (it == OPCODES.end()) {
        nes->out << "\n\nError: unknown opcode! ";
        print_hex(nes->out, opcode);
        exit;
    }
    curr_op = it->second;
    inc_pc = true;
    bool crossed_page = false;

    idle_cycles += (curr_op.cycles & ~PAGE_SENSITIVE);
    print_hex(oss, opcode, " ");

    switch (curr_op.mode) {
        case Accumulator:
            nes->out << setw(10) << std::left << oss.str(); oss.str("");
            oss << OP_NAMES[curr_op.id] << " A";
            break;
        case Implicit:
            nes->out << setw(10) << std::left << oss.str(); oss.str("");
            oss << OP_NAMES[curr_op.id];
            break;
        case Immediate:
            ops[0] = read(++reg.pc);
            print_hex(oss, ops[0], " ");
            nes->out << setw(10) << std::left << oss.str(); oss.str("");
            oss << OP_NAMES[curr_op.id] << " #$"; print_hex(oss, ops[0]);
            break;
        case Absolute: {
            uint8_t lo = read(++reg.pc);
            uint8_t hi = read(++reg.pc);
            addrs[0] = create_address(lo, hi);
            ops[0] = read(addrs[0]);
            print_hex(oss, lo, " ");
            print_hex(oss, hi, " ");
            nes->out << setw(10) << std::left << oss.str(); oss.str("");
            oss << OP_NAMES[curr_op.id] << " $"; print_hex(oss, addrs[0]); oss << " = "; print_hex(oss, ops[0]);
            break;
        }
        case ZeroPage: {
            uint8_t lo = read(++reg.pc);
            addrs[0] = lo;
            ops[0] = read(addrs[0]);
            print_hex(oss, lo, " ");
            nes->out << setw(10) << std::left << oss.str(); oss.str("");
            oss << OP_NAMES[curr_op.id] << " $"; print_hex(oss, addrs[0]); oss << " = "; print_hex(oss, ops[0]);
            break;
        }
        case IndexedAbsoluteX:
        case IndexedAbsoluteY: {
            uint8_t lo = read(++reg.pc);
            uint8_t hi = read(++reg.pc);
            uint16_t addr = create_address(lo, hi);
            addrs[0] = addr + (curr_op.mode == IndexedAbsoluteX ? reg.x : reg.y);
            ops[0] = read(addrs[0]);
            if (!same_page(addr, addrs[0])) crossed_page = true;
            print_hex(oss, lo, " ");
            print_hex(oss, hi, " ");
            nes->out << setw(10) << std::left << oss.str(); oss.str("");
            oss << OP_NAMES[curr_op.id] << " $"; print_hex(oss, addr); oss << (curr_op.mode == IndexedAbsoluteX ? ",X" : ",Y") << " @ ";
            print_hex(oss, addrs[0]); oss << " = "; print_hex(oss, ops[0]);
            break;
        }
        case IndexedZeroX:
        case IndexedZeroY: {
            uint8_t lo = read(++reg.pc);
            addrs[0] = (uint8_t)(lo + (curr_op.mode == IndexedZeroX ? reg.x : reg.y));
            ops[0] = read(addrs[0]);
            print_hex(oss, lo, " ");
            nes->out << setw(10) << std::left << oss.str(); oss.str("");
            oss << OP_NAMES[curr_op.id] << " $"; print_hex(oss, lo); oss << (curr_op.mode == IndexedZeroX ? ",X" : ",Y") << " @ ";
            print_hex(oss, addrs[0]); oss << " = "; print_hex(oss, ops[0]);
            break;
        }
        case Indirect: {
            uint8_t lo = read(++reg.pc);
            uint8_t hi = read(++reg.pc);
            uint16_t at = create_address(lo, hi);
            addrs[0] = create_address(read(at), read((uint8_t)(at+1) | (at & 0xFF00))); // Jump address wraps around due to 6502 bug
            print_hex(oss, lo, " ");
            print_hex(oss, hi, " ");
            nes->out << setw(10) << std::left << oss.str(); oss.str("");
            oss << OP_NAMES[curr_op.id] << " ($"; print_hex(oss, at); oss << ") = "; print_hex(oss, addrs[0]);
            break;
        }
        case IndexedIndirectX: {
            uint8_t base = read(++reg.pc);
            auto addr = (uint8_t)(base+reg.x);
            addrs[0] = create_address(read(addr), read((uint8_t)(addr+1)));
            ops[0] = read(addrs[0]);
            print_hex(oss, base, " ");
            nes->out << setw(10) << std::left << oss.str(); oss.str("");
            oss << OP_NAMES[curr_op.id] << " ($"; print_hex(oss, base); oss << ",X) @ "; print_hex(oss, addr); oss << " = "; print_hex(oss, addrs[0]);
            oss << " = "; print_hex(oss, ops[0]);
            break;
        }
        case IndexedIndirectY: {
            uint8_t base = read(++reg.pc);
            uint16_t addr = create_address(read(base), read((uint8_t)(base+1)));
            addrs[0] = addr + reg.y;
            ops[0] = read(addrs[0]);
            if (!same_page(addr, addrs[0])) crossed_page = true;
            print_hex(oss, base, " ");
            nes->out << setw(10) << std::left << oss.str(); oss.str("");
            oss << OP_NAMES[curr_op.id] << " ($"; print_hex(oss, base); oss << "),Y = "; print_hex(oss, addrs[0]); oss << " @ "; print_hex(oss, addrs[0]);
            oss << " = "; print_hex(oss, ops[0]);
            break;
        }
        case Relative:
            offset = (int8_t)read(++reg.pc);
            addrs[0] = reg.pc + 1 + offset;
            print_hex(oss, offset, " ");
            nes->out << setw(10) << std::left << oss.str(); oss.str("");
            oss << OP_NAMES[curr_op.id] << " $"; print_hex(oss, addrs[0]);
            break;
        default:
            break;
    }
    if (((curr_op.cycles & PAGE_SENSITIVE) >> 4) & 1 && crossed_page) ++idle_cycles;

    switch (curr_op.id) {
        case Op::ADC: ADC(); break;
        case Op::AND: AND(); break;
        case Op::ASL: ASL(); break;
        case Op::BCC: BCC(); break;
        case Op::BCS: BCS(); break;
        case Op::BEQ: BEQ(); break;
        case Op::BIT: BIT(); break;
        case Op::BMI: BMI(); break;
        case Op::BNE: BNE(); break;
        case Op::BPL: BPL(); break;
        case Op::BRK: BRK(); break;
        case Op::BVC: BVC(); break;
        case Op::BVS: BVS(); break;
        case Op::CLC: CLC(); break;
        case Op::CLD: CLD(); break;
        case Op::CLI: CLI(); break;
        case Op::CLV: CLV(); break;
        case Op::CMP: CMP(); break;
        case Op::CPX: CPX(); break;
        case Op::CPY: CPY(); break;
        case Op::DCP: DCP(); break;
        case Op::DEC: DEC(); break;
        case Op::DEX: DEX(); break;
        case Op::DEY: DEY(); break;
        case Op::EOR: EOR(); break;
        case Op::INC: INC(); break;
        case Op::INX: INX(); break;
        case Op::INY: INY(); break;
        case Op::ISB: ISB(); break;
        case Op::JMP: JMP(); break;
        case Op::JSR: JSR(); break;
        case Op::LAX: LAX(); break;
        case Op::LDA: LDA(); break;
        case Op::LDX: LDX(); break;
        case Op::LDY: LDY(); break;
        case Op::LSR: LSR(); break;
        case Op::NOP: NOP(); break;
        case Op::ORA: ORA(); break;
        case Op::PHA: PHA(); break;
        case Op::PHP: PHP(); break;
        case Op::PLA: PLA(); break;
        case Op::PLP: PLP(); break;
        case Op::RLA: RLA(); break;
        case Op::ROL: ROL(); break;
        case Op::ROR: ROR(); break;
        case Op::RRA: RRA(); break;
        case Op::RTI: RTI(); break;
        case Op::RTS: RTS(); break;
        case Op::SAX: SAX(); break;
        case Op::SBC: SBC(); break;
        case Op::SEC: SEC(); break;
        case Op::SED: SED(); break;
        case Op::SEI: SEI(); break;
        case Op::SLO: SLO(); break;
        case Op::SRE: SRE(); break;
        case Op::STA: STA(); break;
        case Op::STX: STX(); break;
        case Op::STY: STY(); break;
        case Op::TAX: TAX(); break;
        case Op::TAY: TAY(); break;
        case Op::TSX: TSX(); break;
        case Op::TXA: TXA(); break;
        case Op::TXS: TXS(); break;
        case Op::TYA: TYA(); break;
        case Op::USBC: USBC(); break;
        default:
            break;
    }
    nes->out << setw(30) << std::left << oss.str();
    if (inc_pc) reg.pc++;
}

void CPU::set_status(const STATUS status, bool value) {
    reg.p = value ? reg.p | status : reg.p & ~status;
}

bool CPU::get_status(const STATUS status) const {
    return (reg.p & status) == status;
}

void CPU::set_value_status(const uint8_t val) {
    set_status(STATUS::z, val == 0);
    set_status(STATUS::n, (bool)(val>>7));
}

void CPU::push_stack(const uint8_t byte) {
    write(create_address(reg.s--, 0x01), byte);
}

uint8_t CPU::pop_stack() {
    uint8_t data = read(create_address(++reg.s, 0x01));
    return data;
}

uint8_t CPU::peek_stack(const uint8_t bytes) {
    return read(create_address(reg.s + bytes, 0x01));
}

void CPU::branch(const STATUS status, const bool check_against) {
    if (get_status(status) == check_against) {
        if (!same_page(reg.pc + 1, addrs[0]))
            idle_cycles += 2;
        else
            idle_cycles += 1;
        reg.pc = addrs[0];
        inc_pc = false;
    }
}

void CPU::compare(const uint8_t a, const uint8_t b) {
    if (a == b) {
        set_status(STATUS::n, false);
        set_status(STATUS::z, true);
        set_status(STATUS::c, true);
    } else {
        set_status(STATUS::z, false);
        uint8_t diff = a - b;
        set_status(STATUS::n, (bool)(diff>>7));
        set_status(STATUS::c, a > b);
    }
}

void CPU::push_address(const uint16_t addr) {
    push_stack((uint8_t)((addr & 0xFF00) >> 8));
    push_stack((uint8_t)(addr & 0x00FF));
}

uint16_t CPU::pop_address() {
    uint8_t a = pop_stack();
    uint8_t b = pop_stack();
    return create_address(a, b);
}

uint16_t CPU::read_address(const uint16_t addr) {
    return create_address(read(addr), read(addr+1));
}

uint16_t CPU::create_address(const uint8_t lo, const uint8_t hi) {
    return ((uint16_t)hi) << 8 | lo;
}

bool CPU::same_page(const uint16_t addr1, const uint16_t addr2) {
    return (addr1 >> 8 == addr2 >> 8);
}

uint8_t CPU::shift_right(uint8_t byte) {
    set_status(STATUS::c, (bool)(byte & 0x1));
    byte = byte >> 1;
    return byte;
}

uint8_t CPU::shift_left(uint8_t byte) {
    set_status(STATUS::c, (bool)((byte >> 7) & 0x1));
    byte = byte << 1;
    return byte;
}

uint8_t CPU::rot_right(uint8_t byte) {
    uint8_t c = get_status(STATUS::c);
    byte = shift_right(byte) | (c << 7);
    return byte;
}

uint8_t CPU::rot_left(uint8_t byte) {
    uint8_t c = get_status(STATUS::c);
    byte = shift_left(byte) | c;
    return byte;
}

void CPU::add_with_carry(bool sub) {
    if (sub) ops[0] = ~ops[0];
    bool carry = get_status(STATUS::c);
    uint16_t sum = reg.acc + ops[0] + carry;
    set_status(STATUS::c, sum > 0xFF);
    set_status(STATUS::v, ~(reg.acc ^ ops[0]) & (reg.acc ^ sum) & 0x80);
    reg.acc = sum;
    set_value_status(sum);
}

// ==== OPCODES ====
void CPU::ADC() { add_with_carry(); }
void CPU::AND() {
    reg.acc &= ops[0];
    set_value_status(reg.acc);
}
void CPU::ASL() {
    if (curr_op.mode == Accumulator) {
        reg.acc = shift_left(reg.acc);
        set_value_status(reg.acc);
    } else {
        ops[0] = shift_left(ops[0]);
        write(addrs[0], ops[0]);
        set_value_status(ops[0]);
    }
}
void CPU::BCC() { branch(STATUS::c, false); }
void CPU::BCS() { branch(STATUS::c, true); }
void CPU::BEQ() { branch(STATUS::z, true); }
void CPU::BIT() {
    uint8_t test = read(addrs[0]);
    set_status(STATUS::n, (bool) (test >> 7 & 1));
    set_status(STATUS::v, (bool) (test >> 6 & 1));
    set_status(STATUS::z, (bool) ((test & reg.acc) == 0));
}
void CPU::BMI() { branch(STATUS::n, true); }
void CPU::BNE() { branch(STATUS::z, false); }
void CPU::BPL() { branch(STATUS::n, false); }
void CPU::BRK() {
    push_address(++reg.pc + 1);
    PHP();
    SEI();
}
void CPU::BVC() { branch(STATUS::v, false); }
void CPU::BVS() { branch(STATUS::v, true); }
void CPU::CLC() { set_status(STATUS::c, false); }
void CPU::CLD() { set_status(STATUS::d, false); }
void CPU::CLI() { set_status(STATUS::i, false); }
void CPU::CLV() { set_status(STATUS::v, false); }
void CPU::CMP() { compare(reg.acc, ops[0]); }
void CPU::CPX() { compare(reg.x, ops[0]); }
void CPU::CPY() { compare(reg.y, ops[0]); }
void CPU::DCP() {
    DEC();
    CMP();
}
void CPU::DEC() {
    write(addrs[0], --ops[0]);
    set_value_status(ops[0]);
}
void CPU::DEX() { set_value_status(--reg.x); }
void CPU::DEY() { set_value_status(--reg.y); }
void CPU::EOR() {
    reg.acc ^= ops[0];
    set_value_status(reg.acc);
}
void CPU::INC() {
    write(addrs[0], ++ops[0]);
    set_value_status(ops[0]);
}
void CPU::INX() { set_value_status(++reg.x); }
void CPU::INY() { set_value_status(++reg.y); }
void CPU::ISB() {
    INC();
    SBC();
}
void CPU::JMP() {
    reg.pc = addrs[0];
    inc_pc = false;
}
void CPU::JSR() {
    push_address(reg.pc);
    JMP();
}
void CPU::LAX() {
    LDA();
    LDX();
}
void CPU::LDA() {
    set_value_status(ops[0]);
    reg.acc = ops[0];
}
void CPU::LDX() {
    set_value_status(ops[0]);
    reg.x = ops[0];
}
void CPU::LDY() {
    set_value_status(ops[0]);
    reg.y = ops[0];
}
void CPU::LSR() {
    if (curr_op.mode == Accumulator) {
        reg.acc = shift_right(reg.acc);
        set_value_status(reg.acc);
    } else {
        ops[0] = shift_right(ops[0]);
        CPU::write(addrs[0], ops[0]);
        set_value_status(ops[0]);
    }
}
void CPU::NOP() {}
void CPU::ORA() {
    reg.acc |= ops[0];
    set_value_status(reg.acc);
}
void CPU::PHA() { push_stack(reg.acc); }
void CPU::PHP() { push_stack(reg.p); }
void CPU::PLA() {
    reg.acc = pop_stack();
    set_value_status(reg.acc);
}
void CPU::PLP() {
    uint8_t status = pop_stack();
    bool b = get_status(STATUS::b);
    bool bit_5 = get_status(STATUS::bit_5);
    reg.p = status;
    set_status(STATUS::b, b);
    set_status(STATUS::bit_5, bit_5);
}
void CPU::RLA() {
    ROL();
    AND();
}
void CPU::ROL() {
    if (curr_op.mode == Accumulator) {
        reg.acc = rot_left(reg.acc);
        set_value_status(reg.acc);
    } else {
        ops[0] = rot_left(ops[0]);
        write(addrs[0], ops[0]);
        set_value_status(ops[0]);
    }
}
void CPU::ROR() {
    if (curr_op.mode == Accumulator) {
        reg.acc = rot_right(reg.acc);
        set_value_status(reg.acc);
    } else {
        ops[0] = rot_right(ops[0]);
        write(addrs[0], ops[0]);
        set_value_status(ops[0]);
    }
}
void CPU::RRA() {
    ROR();
    ADC();
}
void CPU::RTI() {
    PLP();
    RTS();
    inc_pc = false;
}
void CPU::RTS() { reg.pc = pop_address(); }
void CPU::SAX() {
    write(addrs[0], reg.acc & reg.x);
}
void CPU::SBC() { add_with_carry(true); }
void CPU::SEC() { set_status(STATUS::c, true); }
void CPU::SED() { set_status(STATUS::d, true); }
void CPU::SEI() { set_status(STATUS::i, true); }
void CPU::SLO() {
    ASL();
    ORA();
}
void CPU::SRE() {
    LSR();
    EOR();
}
void CPU::STA() {
    write(addrs[0], reg.acc);
}
void CPU::STX() {
    write(addrs[0], reg.x);
}
void CPU::STY() {
    write(addrs[0], reg.y);
}
void CPU::TAX() {
    reg.x = reg.acc;
    set_value_status(reg.x);
}
void CPU::TAY() {
    reg.y = reg.acc;
    set_value_status(reg.y);
}
void CPU::TSX() {
    reg.x = reg.s;
    set_value_status(reg.x);
}
void CPU::TXA() {
    reg.acc = reg.x;
    set_value_status(reg.acc);
}
void CPU::TXS() { reg.s = reg.x; }
void CPU::TYA() {
    reg.acc = reg.y;
    set_value_status(reg.acc);
}
void CPU::USBC() {
    SBC();
}

// TODO PPU register handling
uint8_t CPU::read(int addr) {
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        return nes->get_ppu()->read_reg(addr % 8, cycle);
    }
    return Processor::read(addr);
}

bool CPU::write(const uint16_t addr, const uint8_t data) {
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        return nes->get_ppu()->write_reg(addr % 8, data, cycle);
    }
    return Processor::write(addr, data);
}