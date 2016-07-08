from .opcode import Opcodes
from .registers import Registers
import struct

class Instruction(object):
    def __init__(self, opcode, reg1=None, reg2=None, reg3=None, imm=None):
        self.opcode = opcode
        self.reg1 = reg1
        self.reg2 = reg2
        self.reg3 = reg3
        self.imm = imm

    def compose(self):
        op = Opcodes.normalize(self.opcode)
        if not op:
            raise ValueError('`opcode` is not opcode (%r)' % self.opcode)

        regs = []
        for i in range(1, 1 + op.regs):
            reg = Registers.normalize(getattr(self, 'reg%d' % i))
            if not reg:
                raise ValueError('`reg%d` is missing' % i)
            regs.append(reg.code)
        regs += [0] * (3 - len(regs))

        if op.type_ == 'I':
            imm = self.imm & 0xffff
        elif op.type_ == 'R':
            imm = regs[2]
        else:
            imm = 0

        return struct.pack('<BBH', op.code, regs[0] << 4 | regs[1], imm)
