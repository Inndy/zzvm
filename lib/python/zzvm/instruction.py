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

    def compose(self):
        """
        return encoded instruction in bytes
        """
        op = Opcodes.normalize(self.opcode)
        if not op:
            raise ValueError('`opcode` is not opcode (%r)' % self.opcode)

        regs = map(Registers.normalize, (self.reg1, self.reg2, self.reg3))
        if not all(regs[i] for i in range(op.regs)):
            raise ValueError('required for this instruction is missing')

        if op.type_ == 'I':
            imm = self.imm & 0xffff
        elif op.type_ == 'R':
            imm = regs[2]
        else:
            imm = 0

        return struct.pack('<BBH', op.code, regs[0] << 4 | regs[1], imm)
