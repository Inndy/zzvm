import os

code_to_opcode_mapping = {}
name_to_opcode_mapping = {}

class Opcode(object):
    def __init__(self, name, code, type_, regs):
        global code_to_opcode_mapping
        global name_to_opcode_mapping

        self.name = name
        self.code = code
        self.type_ = type_
        self.regs = regs

        code_to_opcode_mapping[code] = self
        name_to_opcode_mapping[name] = self

    def __int__(self):
        return self.code

    def __str__(self):
        return self.name

    def __repr__(self):
        return 'Opcode(name=%r, code=%r, type_=%r, regs=%r)' % (
                    self.name, self.code, self.type_, self.regs
                )

class Opcodes:
    @classmethod
    def normalize(class_, obj):
        if type(obj) is Opcode:
            return obj
        elif type(obj) is int:
            return code_to_opcode_mapping.get(obj)
        elif type(obj) is str:
            return name_to_opcode_mapping.get(obj.upper())

with open(os.path.abspath(os.path.join(os.path.dirname(__file__),
                                       '../../../zzvm/zzcode.h'))) as f:
    for line in f.readlines():
        line = line.strip()
        if line.startswith('ZZOP_'):
            opname, _, opcode, _, optype, opregs = line.split()

            opname = opname[5:]  # skip 'ZZOP_'
            opcode = int(opcode[:-1], 16)  # remove trailing ','
            opregs = int(opregs)

            setattr(Opcodes, opname, Opcode(opname, opcode, optype, opregs))
