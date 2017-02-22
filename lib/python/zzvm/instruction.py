from .opcode import Opcodes
from .registers import Registers
import struct

class Instruction(object):
    def __init__(self, opcode, reg1=None, reg2=None, reg3=None, imm=None):
        """
        opcode		type(opcode) in (int, str, Opcode)
        registers	type(reg) in (int, str, Register)
        imm		type(imm) is int
        """
        self.opcode = opcode
        self.reg1 = reg1
        self.reg2 = reg2
        self.reg3 = reg3
        self.imm = imm

    def _raw_regs_to_code(self, required_count):
        raw_regs = (self.reg1, self.reg2, self.reg3)
        regs = [ Registers.normalize(r) for r in raw_regs ]

        if any(regs[i] is None for i in range(required_count)):
            raise ValueError('required register instruction is missing')

        return [ r.code if r else 0 for r in regs ]

    def compose(self):
        """
        return encoded instruction in bytes
        """
        op = Opcodes.normalize(self.opcode)
        if not op:
            raise ValueError('`opcode` is not opcode (%r)' % self.opcode)

        regs_val = self._raw_regs_to_code(op.regs)

        if op.type_ == 'I':
            imm = self.imm & 0xffff
        elif op.type_ == 'R':
            imm = regs_val[2]
        else:
            imm = 0

        return struct.pack('<BBH', op.code, regs_val[0] << 4 | regs_val[1], imm)
