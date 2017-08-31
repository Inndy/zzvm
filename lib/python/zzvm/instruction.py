from .opcode import Opcodes
from .registers import Registers
from .symbol import Symbol
import struct

class Instruction(object):
    def __init__(self, opcode, reg1=None, reg2=None, reg3=None, imm=None):
        """
        opcode		type(opcode) in (int, str, Opcode)
        registers	type(reg) in (int, str, Register)
        imm		type(imm) is int
        """
        self.opcode = Opcodes.normalize(opcode)
        if not self.opcode:
            raise ValueError('Unknown instruction: %r' % opcode)
        self.reg1 = self._reg(reg1)
        self.reg2 = self._reg(reg2)
        self.reg3 = self._reg(reg3)
        self.imm = imm

    def __repr__(self):
        ATTR = ('reg1', 'reg2', 'reg3', 'imm')
        return 'Instruction(opcode=%r, %s)' % (
            self.opcode.name,
            ', '.join('%s=%r' % (i, getattr(self, i)) for i in ATTR if getattr(self, i) is not None)
        )

    def _reg(self, r):
        if r:
            return Registers.normalize(r)
        else:
            return r

    def _raw_regs_to_code(self, required_count):
        raw_regs = (self.reg1, self.reg2, self.reg3)
        regs = [ Registers.normalize(r) for r in raw_regs ]

        if any(regs[i] is None for i in range(required_count)):
            raise ValueError('required register instruction is missing')

        return [ r.code if r else 0 for r in regs ]

    def compose(self, offset=None):
        """
        return encoded instruction in bytes
        """
        op = self.opcode
        if not op:
            raise ValueError('`opcode` is not opcode (%r)' % self.opcode)

        regs_val = self._raw_regs_to_code(op.regs)

        if type(self.imm) is Symbol:
            assert offset is not None
            imm = offset & 0xffff
        elif op.type_ == 'I':
            imm = self.imm & 0xffff
        elif op.type_ == 'R':
            imm = regs_val[2]
        else:
            imm = 0

        return struct.pack('<BBH', op.code, regs_val[0] << 4 | regs_val[1], imm)
