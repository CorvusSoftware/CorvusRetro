# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Generate original GB/GBC/GBA programs for emulator integration checks.

GB displays stripes and toggles its palette on each A press.
GBC runs the same program in color mode; GBA writes 0x5a to SRAM and loops.
These fixtures exercise the integration, not commercial game compatibility.
"""
from pathlib import Path
import sys
out=Path(sys.argv[1] if len(sys.argv) > 1 else 'build/test-roms')
out.mkdir(parents=True, exist_ok=True)
rom=bytearray(32768)
rom[0x100:0x103]=bytes([0xc3,0x50,0x01])
rom[0x134:0x13f]=b'CORVUS TEST'
code=[];labels={};patches=[]
def emit(*values):code.extend(values)
def label(name):labels[name]=len(code)
def jr(op,name):emit(op,0);patches.append((len(code)-1,name,'rel'))
def jp(name):emit(0xc3,0,0);patches.append((len(code)-2,name,'abs'))
emit(0xf3,0xaf,0xe0,0x40,0x3e,0xe4,0xe0,0x47,0x21,0x00,0x80,0x06,0x08)
label('tile');emit(0x3e,0xaa,0x22,0xaf,0x22,0x05);jr(0x20,'tile')
emit(0x3e,0x91,0xe0,0x40,0x0e,0xe4,0x3e,0x10,0xe0,0x00)
label('press');emit(0xf0,0x00,0xe6,0x01);jr(0x20,'press')
emit(0x79,0x2f,0x4f,0xe0,0x47)
label('release');emit(0xf0,0x00,0xe6,0x01);jr(0x28,'release');jp('press')
for pos,name,kind in patches:
 if kind=='rel':code[pos]=(labels[name]-pos-1)&255
 else:
  address=0x150+labels[name];code[pos]=address&255;code[pos+1]=address>>8
rom[0x150:0x150+len(code)]=bytes(code)
rom[0x14d]=(-sum(rom[0x134:0x14d])-(0x14d-0x134))&255
out.joinpath('Corvus-test.gb').write_bytes(rom)
gbc=bytearray(rom);gbc[0x143]=0x80;gbc[0x14d]=(-sum(gbc[0x134:0x14d])-(0x14d-0x134))&255
out.joinpath('Corvus-test.gbc').write_bytes(gbc)
gba=bytearray(512)
for i,word in enumerate([0xe59f0008,0xe3a0105a,0xe5c01000,0xeafffffe,0x0e000000]):gba[i*4:i*4+4]=word.to_bytes(4,'little')
gba[0xb2]=0x96;gba[0x100:0x106]=b'SRAM_V'
out.joinpath('Corvus-test.gba').write_bytes(gba)
out.joinpath('Corvus-invalid.gb').write_bytes(b'invalid')
print('Synthetic GB/GBC/GBA test content generated, no commercial ROM or BIOS')
