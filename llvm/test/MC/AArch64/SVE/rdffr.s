// RUN: llvm-mc -triple=aarch64 -show-encoding -mattr=+sve < %s \
// RUN:        | FileCheck %s --check-prefixes=CHECK-ENCODING,CHECK-INST
// RUN: not llvm-mc -triple=aarch64 -show-encoding < %s 2>&1 \
// RUN:        | FileCheck %s --check-prefix=CHECK-ERROR
// RUN: not llvm-mc -triple=aarch64 -show-encoding -mattr=+sme < %s 2>&1 \
// RUN:        | FileCheck %s --check-prefix=CHECK-ERROR
// RUN: llvm-mc -triple=aarch64 -filetype=obj -mattr=+sve < %s \
// RUN:        | llvm-objdump -d --mattr=+sve - | FileCheck %s --check-prefix=CHECK-INST
// RUN: llvm-mc -triple=aarch64 -filetype=obj -mattr=+sve < %s \
// RUN:   | llvm-objdump -d --mattr=-sve - | FileCheck %s --check-prefix=CHECK-UNKNOWN

rdffr   p0.b
// CHECK-INST: rdffr   p0.b
// CHECK-ENCODING: [0x00,0xf0,0x19,0x25]
// CHECK-ERROR: instruction requires: sve
// CHECK-UNKNOWN: 2519f000 <unknown>

rdffr   p15.b
// CHECK-INST: rdffr   p15.b
// CHECK-ENCODING: [0x0f,0xf0,0x19,0x25]
// CHECK-ERROR: instruction requires: sve
// CHECK-UNKNOWN: 2519f00f <unknown>

rdffr   p0.b, p0/z
// CHECK-INST: rdffr   p0.b, p0/z
// CHECK-ENCODING: [0x00,0xf0,0x18,0x25]
// CHECK-ERROR: instruction requires: sve
// CHECK-UNKNOWN: 2518f000 <unknown>

rdffr   p15.b, p15/z
// CHECK-INST: rdffr   p15.b, p15/z
// CHECK-ENCODING: [0xef,0xf1,0x18,0x25]
// CHECK-ERROR: instruction requires: sve
// CHECK-UNKNOWN: 2518f1ef <unknown>
