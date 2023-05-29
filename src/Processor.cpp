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

uint8_t AddressSpace::read(const uint16_t& addr) {
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
    int z = 1000;
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

// TODO add cycles for crossing page boundaries
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
            addrs[0] = lo | hi << 8;
            print_hex(oss, lo, " ");
            print_hex(oss, hi, " ");
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id << " $"; print_hex(oss, addrs[0]);
            break;
        }
        case ZeroPage: {
            uint8_t lo = read(++reg.pc);
            addrs[0] = lo;
            print_hex(oss, lo, " ");
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id << " $"; print_hex(oss, addrs[0]);
            break;
        }
        case Implicit:
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id;
            break;
        case Relative:
            offset = (int8_t)read(++reg.pc);
            print_hex(oss, offset, " ");
            cout << setw(10) << std::left << oss.str(); oss.str("");
            oss << inf.id << " $"; print_hex(oss, reg.pc + offset + 1);
            break;
        default:
            break;
    }

    switch (opcode) {
        case 0x78:
            set_status(STATUS::i, true);
            break;
        case 0xD8:
            set_status(STATUS::d, false);
            break;
        case 0xA9:
            set_value_status(ops[0]);
            reg.acc = ops[0];
            break;
        case 0x85:
        case 0x8D:
            write(addrs[0], reg.acc);
            oss << " = "; print_hex(oss, reg.acc);
            break;
        case 0x4C:
            reg.pc = addrs[0];
            cout << setw(20) << std::left << oss.str();
            return;
        case 0xA2:
            set_value_status(ops[0]);
            reg.x = ops[0];
            break;
        case 0x86:
            write(addrs[0], reg.x);
            oss << " = "; print_hex(oss, reg.x);
            break;
        case 0x20:
            push_stack((uint8_t)(reg.pc & 0x00FF));
            push_stack((uint8_t)((reg.pc & 0xFF00) >> 8));
            reg.pc = addrs[0];
            cout << setw(20) << std::left << oss.str();
            return;
        case 0xEA:
            break;
        case 0x38:
            set_status(STATUS::c, true);
            break;
        case 0xB0:
            branch(STATUS::c, true, offset);
            break;
        case 0x18:
            set_status(STATUS::c, false);
            break;
        case 0x90:
            branch(STATUS::c, false, offset);
            break;
        case 0xF0:
            branch(STATUS::z, true, offset);
            break;
        case 0xD0:
            branch(STATUS::z, false, offset);
            break;
        case 0x24: {
            uint8_t test = read(addrs[0]);
            oss << " = "; print_hex(oss, test);
            set_status(STATUS::n, (bool) (test >> 7 & 1));
            set_status(STATUS::v, (bool) (test >> 6 & 1));
            set_status(STATUS::z, (bool) ((test & reg.acc) == 0));
            break;
        }
        case 0x70:
            branch(STATUS::v, true, offset);
            break;
        case 0x50:
            branch(STATUS::v, false, offset);
            break;
        case 0x10:
            branch(STATUS::n, false, offset);
            break;
        case 0x60: {
            uint8_t a = pop_stack();
            uint8_t b = pop_stack();
            reg.pc = ((uint16_t)a) << 8 | b;
            break;
        }
        case 0xF8:
            set_status(STATUS::d, true);
            break;
        case 0x08: {
            uint8_t status = reg.p | STATUS::b | STATUS::bit_5;
            push_stack(status);
            break;
        }
        case 0x68:
            reg.acc = pop_stack();
            set_value_status(reg.acc);
            break;
        case 0x29:
            reg.acc = reg.acc & ops[0];
            set_value_status(reg.acc);
            break;
        case 0xC9:
            if (reg.acc == ops[0]) {
                set_status(STATUS::n, false);
                set_status(STATUS::z, true);
                set_status(STATUS::c, true);
            } else {
                set_status(STATUS::z, false);
                uint8_t diff = reg.acc - ops[0];
                set_status(STATUS::n, (bool)(diff>>7));
                set_status(STATUS::c, diff > 0);
            }
            break;
        case 0x48:
            push_stack(reg.acc);
            break;
        case 0x28: {
            uint8_t status = pop_stack();
            bool b = get_status(STATUS::b);
            bool bit_5 = get_status(STATUS::bit_5);
            reg.p = status;
            set_status(STATUS::b, b);
            set_status(STATUS::bit_5, bit_5);
            break;
        }
        case 0x30:
            branch(STATUS::n, true, offset);
            break;
        case 0x09:
            reg.acc = reg.acc | ops[0];
            set_value_status(reg.acc);
            break;
        case 0xB8:
            set_status(STATUS::v, false);
            break;
        case 0x49:
            reg.acc = reg.acc ^ ops[0];
            set_value_status(reg.acc);
            break;
        case 0x69:
            //TODO
        default:
            break;
    }
    cout << setw(20) << std::left << oss.str();
    reg.pc++;
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

void CPU::push_stack(const uint8_t &byte) {
    write(reg.s | (0x10 << 8), byte);
    reg.s--;
}

uint8_t CPU::pop_stack() {
    uint8_t data = read(++reg.s | (0x10 << 8));
    return data;
}

uint8_t CPU::peek_stack(const uint8_t& bytes) {
    return read((reg.s + bytes + 1) | (0x10 << 8));
}

void CPU::branch(const STATUS &status, const bool &check_against, const uint8_t &offset) {
    if (get_status(status) == check_against) {
        if ((reg.pc + offset) >> 8 != reg.pc >> 8)
            cycle += 2;
        else
            cycle += 1;
        reg.pc += offset;
    }
}

void CPU::set_value_status(const uint8_t &val) {
    set_status(STATUS::z, val == 0);
    set_status(STATUS::n, (bool)(val>>7));
}


