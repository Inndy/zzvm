code_to_reg_mapping = {}
name_to_reg_mapping = {}

class Register(object):
    def __init__(self, name, code):
        global code_to_reg_mapping
        global name_to_reg_mapping

        self.name = name
        self.code = code
        code_to_reg_mapping[code] = self
        name_to_reg_mapping[name] = self

    def __int__(self):
        return self.code

    def __str__(self):
        return self.name

    def __repr__(self):
        return 'Register(%r)' % (self.name)
        # return 'Register(name=%r, code=%r)' % (self.name, self.code)

class Registers:
    RA = Register('RA', 0)
    R1 = Register('R1', 1)
    R2 = Register('R2', 2)
    R3 = Register('R3', 3)
    R4 = Register('R4', 4)
    R5 = Register('R5', 5)
    SP = Register('SP', 6)
    IP = Register('IP', 7)

    @classmethod
    def normalize(class_, reg):
        if type(reg) is Register:
            return reg
        elif type(reg) is str:
            return name_to_reg_mapping.get(reg.upper())
        elif type(reg) is int:
            return code_to_reg_mapping.get(reg)
