#include <algorithm>
#include <iostream>
#include <iomanip>
#include "Processor.h"
#include "util.h"

AddressSpace::iterator AddressSpace::get_block(const uint16_t &addr) {
    auto dummy = MappedBlock{addr, 0, 0};
    auto it = lower_bound(dummy);
    if (it->start_addr == addr || (--it)->start_addr + it->size > addr)
        return it;
    else
        return this->end();
}

uint8_t AddressSpace::read(const int& addr) {
    auto it = get_block(addr);
    if (it != this->end())
        return it->start_mem[addr-it->start_addr];
    else
        return 0;
}

bool AddressSpace::write(const uint16_t& addr, const uint8_t& data) {
    auto it = get_block(addr);
    if (it != this->end()) {
        it->start_mem[addr-it->start_addr] = data;
        return true;
    } else
        return false;
}

Processor::Processor() {
    this->mem.init(0x800);
}

CPU::CPU() : Processor() {
    this->CPU::reset();
}

void CPU::reset() {
    // Clear the address space and map the CPU'n 2KB memory from 0x0000 to 0x2000
    this->aspace.clear();
    for (int i = 0; i < 4; i++) {
        this->aspace.insert(MappedBlock{static_cast<uint16_t>(i*0x800), this->mem.get_mem(), 0x800});
    }
}

void CPU::run() {
    cycle = 7;
    cout.fill('0');
    reg.pc = read(0xFFFD) << 8 | read(0xFFFC);
    reg.pc = 0xC000; // TODO remove, for nestest only
    reg.p = 0x24; // Initialize STATUS register to 00100100
    reg.s = 0xFD; // Initialize stack head pointer TODO change to FF, for nestest only
    cout << "\n\n===== RUNNING =====";
    int z = 10000;
    for(;;) {
        std::ostringstream regs_log;
        regs_log << "A:"; print_hex(regs_log, reg.acc);
        regs_log << " X:"; print_hex(regs_log,reg.x);
        regs_log << " Y:"; print_hex(regs_log,reg.y);
        regs_log << " P:"; print_hex(regs_log,reg.p);
        regs_log << " SP:"; print_hex(regs_log,reg.s);
        regs_log << " CYC:" << cycle;

        cout << "\n";
        uint8_t opcode = read(reg.pc);
        print_hex(cout, reg.pc, "  ");
        exec(opcode);

        cout << setw(30) << std::left << regs_log.str();
        z--;
        if (z <= 0) break;
    }
}

void CPU::exec(const uint8_t& opcode) {
    auto it = OPCODES.find(opcode);
    if (it == OPCODES.end()) {
        cout << "\n\nError: unknown opcode! ";
        print_hex(cout, opcode);
        exit;
    }
    opcode_info inf = it->second;
    uint8_t ops[2];
    uint16_t addrs[2];
    int8_t offset;

    bool inc_pc = true;

    cycle += inf.cycles;
    std::ostringstream oss;
    print_hex(oss, opcode, " ");

    switch (inf.mode) {
        case Immediate:
            ops[0] = read(++reg.pc);
            print_hex(oss, ops[0], " ");
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id << " #$"; print_hex(oss, ops[0]);
            break;
        case Absolute: {
            uint8_t lo = read(++reg.pc);
            uint8_t hi = read(++reg.pc);
            addrs[0] = create_address(lo, hi);
            ops[0] = read(addrs[0]);
            print_hex(oss, lo, " ");
            print_hex(oss, hi, " ");
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id << " $"; print_hex(oss, addrs[0]);
            break;
        }
        case ZeroPage: {
            uint8_t lo = read(++reg.pc);
            addrs[0] = lo;
            ops[0] = read(addrs[0]);
            print_hex(oss, lo, " ");
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id << " $"; print_hex(oss, addrs[0]);
            break;
        }
        case Implicit:
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id;
            break;
        case IndexedIndirectX: {
            uint8_t base = read(++reg.pc);
            auto addr = (uint8_t)(base+reg.x);
            addrs[0] = create_address(read(addr), read((uint8_t)(addr+1)));
            ops[0] = read(addrs[0]);
            print_hex(oss, base, " ");
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id << " ($"; print_hex(oss, base); oss << ",X) @ "; print_hex(oss, addr); oss << " = "; print_hex(oss, addrs[0]);
            oss << " = "; print_hex(oss, ops[0]);
            break;
        }
        case IndexedIndirectY: {
            // TODO add cycle for page cross for certain opcodes
            uint8_t base = read(++reg.pc);
            uint16_t addr = create_address(read(base), read((uint8_t)(base+1)));
            addrs[0] = addr + reg.y;
            ops[0] = read(addrs[0]);
            print_hex(oss, base, " ");
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id << " ($"; print_hex(oss, base); oss << "),Y = "; print_hex(oss, addrs[0]); oss << " @ "; print_hex(oss, addrs[0]);
            oss << " = "; print_hex(oss, ops[0]);
            break;
        }
        case Relative:
            offset = (int8_t)read(++reg.pc);
            print_hex(oss, offset, " ");
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id << " $"; print_hex(oss, reg.pc + offset + 1);
            break;
        case Accumulator:
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id << " A";
            break;
        default:
            break;
    }

    switch (opcode) {
        //SEI
        case 0x78:
            set_status(STATUS::i, true);
            break;
        //CLD
        case 0xD8:
            set_status(STATUS::d, false);
            break;
        //LDA
        case 0xA1: case 0xA5: case 0xAD: case 0xA9:
            set_value_status(ops[0]);
            reg.acc = ops[0];
            break;
        //STA
        case 0x81: case 0x85: case 0x8D:
            write(addrs[0], reg.acc);
            oss << " = "; print_hex(oss, reg.acc);
            break;
        //JMP
        case 0x4C:
            reg.pc = addrs[0];
            inc_pc = false;
            break;
        //LDX
        case 0xA6: case 0xAE: case 0xA2:
            set_value_status(ops[0]);
            reg.x = ops[0];
            break;
        //LDY
        case 0xA4: case 0xAC: case 0xA0:
            set_value_status(ops[0]);
            reg.y = ops[0];
            break;
        //JSR
        case 0x20:
            push_address(reg.pc);
            reg.pc = addrs[0];
            inc_pc = false;
            break;
        //NOP
        case 0xEA:
            break;
        //SEC
        case 0x38:
            set_status(STATUS::c, true);
            break;
        //BCS
        case 0xB0:
            branch(STATUS::c, true, offset);
            break;
        //CLC
        case 0x18:
            set_status(STATUS::c, false);
            break;
        //BCC
        case 0x90:
            branch(STATUS::c, false, offset);
            break;
        //BEQ
        case 0xF0:
            branch(STATUS::z, true, offset);
            break;
        //BNE
        case 0xD0:
            branch(STATUS::z, false, offset);
            break;
        //BIT
        case 0x2C: case 0x24: {
            uint8_t test = read(addrs[0]);
            oss << " = "; print_hex(oss, test);
            set_status(STATUS::n, (bool) (test >> 7 & 1));
            set_status(STATUS::v, (bool) (test >> 6 & 1));
            set_status(STATUS::z, (bool) ((test & reg.acc) == 0));
            break;
        }
        //BVS
        case 0x70:
            branch(STATUS::v, true, offset);
            break;
        //BVC
        case 0x50:
            branch(STATUS::v, false, offset);
            break;
        //BPL
        case 0x10:
            branch(STATUS::n, false, offset);
            break;
        //RTS
        case 0x60:
            reg.pc = pop_address();
            break;
        //SED
        case 0xF8:
            set_status(STATUS::d, true);
            break;
        //PHP
        case 0x08:
            push_stack(reg.p);
            break;
        //PLA
        case 0x68:
            reg.acc = pop_stack();
            set_value_status(reg.acc);
            break;
        //AND
        case 0x2D: case 0x21: case 0x25: case 0x29:
            reg.acc = reg.acc & ops[0];
            set_value_status(reg.acc);
            break;
        //CMP
        case 0xCD: case 0xC1: case 0xC5: case 0xC9:
            compare(reg.acc, ops[0]);
            break;
        //CPY
        case 0xC4: case 0xC0: case 0xCC:
            compare(reg.y, ops[0]);
            break;
        //CPX
        case 0xE4: case 0xE0: case 0xEC:
            compare(reg.x, ops[0]);
            break;
        //PHA
        case 0x48:
            push_stack(reg.acc);
            break;
        //PLP
        case 0x28:
            pull_status();
            break;
        //BMI
        case 0x30:
            branch(STATUS::n, true, offset);
            break;
        //ORA
        case 0x0D: case 0x01: case 0x05: case 0x09:
            reg.acc = reg.acc | ops[0];
            set_value_status(reg.acc);
            break;
        //CLV
        case 0xB8:
            set_status(STATUS::v, false);
            break;
        //EOR
        case 0x41: case 0x45: case 0x4D: case 0x49:
            reg.acc = reg.acc ^ ops[0];
            set_value_status(reg.acc);
            break;
        //ADC
        case 0x61: case 0x65: case 0x69: case 0x6D:
            adc(ops[0]);
            break;
        //SBC
        case 0xE1: case 0xE5: case 0xE9: case 0xED:
            adc(~ops[0]);
            break;
        //INX
        case 0xE8:
            set_value_status(++reg.x);
            break;
        //INY
        case 0xC8:
            set_value_status(++reg.y);
            break;
        //DEX
        case 0xCA:
            set_value_status(--reg.x);
            break;
        //DEY
        case 0x88:
            set_value_status(--reg.y);
            break;
        //TAX
        case 0xAA:
            reg.x = reg.acc;
            set_value_status(reg.x);
            break;
        //TAY
        case 0xA8:
            reg.y = reg.acc;
            set_value_status(reg.y);
            break;
        //TXA
        case 0x8A:
            reg.acc = reg.x;
            set_value_status(reg.acc);
            break;
        //TYA
        case 0x98:
            reg.acc = reg.y;
            set_value_status(reg.acc);
            break;
        //TSX
        case 0xBA:
            reg.x = reg.s;
            set_value_status(reg.x);
            break;
        //TXS
        case 0x9A:
            reg.s = reg.x;
            break;
        //STX
        case 0x86: case 0x8E:
            write(addrs[0], reg.x);
            oss << " = "; print_hex(oss, reg.x);
            break;
        //STY
        case 0x84: case 0x8C:
            write(addrs[0], reg.y);
            oss << " = "; print_hex(oss, reg.y);
            break;
        //RTI
        case 0x40:
            pull_status();
            reg.pc = pop_address();
            inc_pc = false;
            break;
        //LSR (Accumulator)
        case 0x4A:
            reg.acc = shift_right(reg.acc);
            set_value_status(reg.acc);
            break;
        //LSR
        case 0x46: case 0x4E:
            ops[0] = shift_right(ops[0]);
            write(addrs[0], ops[0]);
            set_value_status(ops[0]);
            break;
        //ASL (Accumulator)
        case 0x0A:
            reg.acc = shift_left(reg.acc);
            set_value_status(reg.acc);
            break;
        //ASL
        case 0x06: case 0x0E:
            ops[0] = shift_left(ops[0]);
            write(addrs[0], ops[0]);
            set_value_status(ops[0]);
            break;
        //ROR (Accumulator)
        case 0x6A:
            reg.acc = rot_right(reg.acc);
            set_value_status(reg.acc);
            break;
        //ROR
        case 0x66: case 0x6E:
            ops[0] = rot_right(ops[0]);
            write(addrs[0], ops[0]);
            set_value_status(ops[0]);
            break;
        //ROL (Accumulator)
        case 0x2A:
            reg.acc = rot_left(reg.acc);
            set_value_status(reg.acc);
            break;
        //ROL
        case 0x26: case 0x2E:
            ops[0] = rot_left(ops[0]);
            write(addrs[0], ops[0]);
            set_value_status(ops[0]);
            break;
        //INC
        case 0xEE: case 0xE6:
            write(addrs[0], ++ops[0]);
            set_value_status(ops[0]);
            break;
        //DEC
        case 0xCE: case 0xC6:
            write(addrs[0], --ops[0]);
            set_value_status(ops[0]);
            break;
        default:
            break;
    }
    cout << setw(30) << std::left << oss.str();
    if (inc_pc) reg.pc++;
}

bool CPU::map(const uint16_t &addr, uint8_t *block, const uint16_t &size) {
    // Validate that the mapped block would not intersect another before mapping
    auto mb = MappedBlock{addr, block, size};
    auto it = aspace.lower_bound(mb);
    if (it != aspace.begin() && (--it)->start_addr + it->size > addr) return false;

    aspace.insert(mb);
    return true;
}

void CPU::set_status(const STATUS &status, bool value) {
    reg.p = value ? reg.p | status : reg.p & ~status;
}

bool CPU::get_status(const STATUS &status) {
    return (reg.p & status) == status;
}

void CPU::set_value_status(const uint8_t &val) {
    set_status(STATUS::z, val == 0);
    set_status(STATUS::n, (bool)(val>>7));
}

void CPU::push_stack(const uint8_t &byte) {
    write(create_address(reg.s--, 0x01), byte);
}

uint8_t CPU::pop_stack() {
    uint8_t data = read(create_address(++reg.s, 0x01));
    return data;
}

uint8_t CPU::peek_stack(const uint8_t& bytes) {
    return read(create_address(reg.s + bytes, 0x01));
}

void CPU::branch(const STATUS &status, const bool &check_against, const uint8_t &offset) {
    if (get_status(status) == check_against) {
        if (!same_page(reg.pc + 1, reg.pc + 1 + offset))
            cycle += 2;
        else
            cycle += 1;
        reg.pc += offset;
    }
}

void CPU::compare(const uint8_t &a, const uint8_t &b) {
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

void CPU::adc(const uint8_t &arg) {
    bool carry = get_status(STATUS::c);
    uint16_t sum = reg.acc + arg + carry;
    set_status(STATUS::c, sum > 0xFF);
    set_status(STATUS::v, ~(reg.acc ^ arg) & (reg.acc ^ sum) & 0x80);
    reg.acc = sum;
    set_value_status(sum);
}

void CPU::pull_status() {
    uint8_t status = pop_stack();
    bool b = get_status(STATUS::b);
    bool bit_5 = get_status(STATUS::bit_5);
    reg.p = status;
    set_status(STATUS::b, b);
    set_status(STATUS::bit_5, bit_5);
}

void CPU::push_address(const uint16_t &addr) {
    push_stack((uint8_t)((addr & 0xFF00) >> 8));
    push_stack((uint8_t)(addr & 0x00FF));
}

uint16_t CPU::pop_address() {
    uint8_t a = pop_stack();
    uint8_t b = pop_stack();
    return create_address(a, b);
}

uint16_t CPU::read_address(const uint16_t& addr) {
    return create_address(read(addr), read(addr+1));
}

uint16_t CPU::create_address(const uint8_t &lo, const uint8_t &hi) {
    return ((uint16_t)hi) << 8 | lo;
}

bool CPU::same_page(const uint16_t& addr1, const uint16_t& addr2) {
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



