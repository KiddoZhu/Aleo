from ctypes import *
from functools import partial

def fread(ctype, fin):
    buffer = fin.read(sizeof(ctype))
    return cast(c_char_p(buffer), POINTER(ctype)).contents