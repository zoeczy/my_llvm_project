// RUN: llvm-mc -triple=aarch64 -show-encoding -mattr=+sve < %s \
// RUN:        | FileCheck %s --check-prefixes=CHECK-ENCODING,CHECK-INST
// RUN: llvm-mc -triple=aarch64 -show-encoding -mattr=+sme < %s \
// RUN:        | FileCheck %s --check-prefixes=CHECK-ENCODING,CHECK-INST
// RUN: not llvm-mc -triple=aarch64 -show-encoding < %s 2>&1 \
// RUN:        | FileCheck %s --check-prefix=CHECK-ERROR
// RUN: llvm-mc -triple=aarch64 -filetype=obj -mattr=+sve < %s \
// RUN:        | llvm-objdump -d --mattr=+sve - | FileCheck %s --check-prefix=CHECK-INST
// RUN: llvm-mc -triple=aarch64 -filetype=obj -mattr=+sve < %s \
// RUN:   | llvm-objdump -d --mattr=-sve - | FileCheck %s --check-prefix=CHECK-UNKNOWN

sxth    z0.s, p0/m, z0.s
// CHECK-INST: sxth    z0.s, p0/m, z0.s
// CHECK-ENCODING: [0x00,0xa0,0x92,0x04]
// CHECK-ERROR: instruction requires: sve or sme
// CHECK-UNKNOWN: 0492a000 <unknown>

sxth    z0.d, p0/m, z0.d
// CHECK-INST: sxth    z0.d, p0/m, z0.d
// CHECK-ENCODING: [0x00,0xa0,0xd2,0x04]
// CHECK-ERROR: instruction requires: sve or sme
// CHECK-UNKNOWN: 04d2a000 <unknown>

sxth    z31.s, p7/m, z31.s
// CHECK-INST: sxth    z31.s, p7/m, z31.s
// CHECK-ENCODING: [0xff,0xbf,0x92,0x04]
// CHECK-ERROR: instruction requires: sve or sme
// CHECK-UNKNOWN: 0492bfff <unknown>

sxth    z31.d, p7/m, z31.d
// CHECK-INST: sxth    z31.d, p7/m, z31.d
// CHECK-ENCODING: [0xff,0xbf,0xd2,0x04]
// CHECK-ERROR: instruction requires: sve or sme
// CHECK-UNKNOWN: 04d2bfff <unknown>


// --------------------------------------------------------------------------//
// Test compatibility with MOVPRFX instruction.

movprfx z4.d, p7/z, z6.d
// CHECK-INST: movprfx	z4.d, p7/z, z6.d
// CHECK-ENCODING: [0xc4,0x3c,0xd0,0x04]
// CHECK-ERROR: instruction requires: sve or sme
// CHECK-UNKNOWN: 04d03cc4 <unknown>

sxth    z4.d, p7/m, z31.d
// CHECK-INST: sxth	z4.d, p7/m, z31.d
// CHECK-ENCODING: [0xe4,0xbf,0xd2,0x04]
// CHECK-ERROR: instruction requires: sve or sme
// CHECK-UNKNOWN: 04d2bfe4 <unknown>

movprfx z4, z6
// CHECK-INST: movprfx	z4, z6
// CHECK-ENCODING: [0xc4,0xbc,0x20,0x04]
// CHECK-ERROR: instruction requires: sve or sme
// CHECK-UNKNOWN: 0420bcc4 <unknown>

sxth    z4.d, p7/m, z31.d
// CHECK-INST: sxth	z4.d, p7/m, z31.d
// CHECK-ENCODING: [0xe4,0xbf,0xd2,0x04]
// CHECK-ERROR: instruction requires: sve or sme
// CHECK-UNKNOWN: 04d2bfe4 <unknown>
