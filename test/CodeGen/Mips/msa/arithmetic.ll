; RUN: llc -march=mips -mattr=+msa,+fp64 < %s | FileCheck %s

define void @add_v16i8(<16 x i8>* %c, <16 x i8>* %a, <16 x i8>* %b) nounwind {
  ; CHECK: add_v16i8:

  %1 = load <16 x i8>* %a
  ; CHECK-DAG: ld.b [[R1:\$w[0-9]+]], 0($5)
  %2 = load <16 x i8>* %b
  ; CHECK-DAG: ld.b [[R2:\$w[0-9]+]], 0($6)
  %3 = add <16 x i8> %1, %2
  ; CHECK-DAG: addv.b [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <16 x i8> %3, <16 x i8>* %c
  ; CHECK-DAG: st.b [[R3]], 0($4)

  ret void
  ; CHECK: .size add_v16i8
}

define void @add_v8i16(<8 x i16>* %c, <8 x i16>* %a, <8 x i16>* %b) nounwind {
  ; CHECK: add_v8i16:

  %1 = load <8 x i16>* %a
  ; CHECK-DAG: ld.h [[R1:\$w[0-9]+]], 0($5)
  %2 = load <8 x i16>* %b
  ; CHECK-DAG: ld.h [[R2:\$w[0-9]+]], 0($6)
  %3 = add <8 x i16> %1, %2
  ; CHECK-DAG: addv.h [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <8 x i16> %3, <8 x i16>* %c
  ; CHECK-DAG: st.h [[R3]], 0($4)

  ret void
  ; CHECK: .size add_v8i16
}

define void @add_v4i32(<4 x i32>* %c, <4 x i32>* %a, <4 x i32>* %b) nounwind {
  ; CHECK: add_v4i32:

  %1 = load <4 x i32>* %a
  ; CHECK-DAG: ld.w [[R1:\$w[0-9]+]], 0($5)
  %2 = load <4 x i32>* %b
  ; CHECK-DAG: ld.w [[R2:\$w[0-9]+]], 0($6)
  %3 = add <4 x i32> %1, %2
  ; CHECK-DAG: addv.w [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <4 x i32> %3, <4 x i32>* %c
  ; CHECK-DAG: st.w [[R3]], 0($4)

  ret void
  ; CHECK: .size add_v4i32
}

define void @add_v2i64(<2 x i64>* %c, <2 x i64>* %a, <2 x i64>* %b) nounwind {
  ; CHECK: add_v2i64:

  %1 = load <2 x i64>* %a
  ; CHECK-DAG: ld.d [[R1:\$w[0-9]+]], 0($5)
  %2 = load <2 x i64>* %b
  ; CHECK-DAG: ld.d [[R2:\$w[0-9]+]], 0($6)
  %3 = add <2 x i64> %1, %2
  ; CHECK-DAG: addv.d [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <2 x i64> %3, <2 x i64>* %c
  ; CHECK-DAG: st.d [[R3]], 0($4)

  ret void
  ; CHECK: .size add_v2i64
}

define void @add_v16i8_i(<16 x i8>* %c, <16 x i8>* %a) nounwind {
  ; CHECK: add_v16i8_i:

  %1 = load <16 x i8>* %a
  ; CHECK-DAG: ld.b [[R1:\$w[0-9]+]], 0($5)
  %2 = add <16 x i8> %1, <i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1,
                          i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1>
  ; CHECK-DAG: addvi.b [[R3:\$w[0-9]+]], [[R1]], 1
  store <16 x i8> %2, <16 x i8>* %c
  ; CHECK-DAG: st.b [[R3]], 0($4)

  ret void
  ; CHECK: .size add_v16i8_i
}

define void @add_v8i16_i(<8 x i16>* %c, <8 x i16>* %a) nounwind {
  ; CHECK: add_v8i16_i:

  %1 = load <8 x i16>* %a
  ; CHECK-DAG: ld.h [[R1:\$w[0-9]+]], 0($5)
  %2 = add <8 x i16> %1, <i16 1, i16 1, i16 1, i16 1,
                          i16 1, i16 1, i16 1, i16 1>
  ; CHECK-DAG: addvi.h [[R3:\$w[0-9]+]], [[R1]], 1
  store <8 x i16> %2, <8 x i16>* %c
  ; CHECK-DAG: st.h [[R3]], 0($4)

  ret void
  ; CHECK: .size add_v8i16_i
}

define void @add_v4i32_i(<4 x i32>* %c, <4 x i32>* %a) nounwind {
  ; CHECK: add_v4i32_i:

  %1 = load <4 x i32>* %a
  ; CHECK-DAG: ld.w [[R1:\$w[0-9]+]], 0($5)
  %2 = add <4 x i32> %1, <i32 1, i32 1, i32 1, i32 1>
  ; CHECK-DAG: addvi.w [[R3:\$w[0-9]+]], [[R1]], 1
  store <4 x i32> %2, <4 x i32>* %c
  ; CHECK-DAG: st.w [[R3]], 0($4)

  ret void
  ; CHECK: .size add_v4i32_i
}

define void @add_v2i64_i(<2 x i64>* %c, <2 x i64>* %a) nounwind {
  ; CHECK: add_v2i64_i:

  %1 = load <2 x i64>* %a
  ; CHECK-DAG: ld.d [[R1:\$w[0-9]+]], 0($5)
  %2 = add <2 x i64> %1, <i64 1, i64 1>
  ; CHECK-DAG: addvi.d [[R3:\$w[0-9]+]], [[R1]], 1
  store <2 x i64> %2, <2 x i64>* %c
  ; CHECK-DAG: st.d [[R3]], 0($4)

  ret void
  ; CHECK: .size add_v2i64_i
}

define void @sub_v16i8(<16 x i8>* %c, <16 x i8>* %a, <16 x i8>* %b) nounwind {
  ; CHECK: sub_v16i8:

  %1 = load <16 x i8>* %a
  ; CHECK-DAG: ld.b [[R1:\$w[0-9]+]], 0($5)
  %2 = load <16 x i8>* %b
  ; CHECK-DAG: ld.b [[R2:\$w[0-9]+]], 0($6)
  %3 = sub <16 x i8> %1, %2
  ; CHECK-DAG: subv.b [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <16 x i8> %3, <16 x i8>* %c
  ; CHECK-DAG: st.b [[R3]], 0($4)

  ret void
  ; CHECK: .size sub_v16i8
}

define void @sub_v8i16(<8 x i16>* %c, <8 x i16>* %a, <8 x i16>* %b) nounwind {
  ; CHECK: sub_v8i16:

  %1 = load <8 x i16>* %a
  ; CHECK-DAG: ld.h [[R1:\$w[0-9]+]], 0($5)
  %2 = load <8 x i16>* %b
  ; CHECK-DAG: ld.h [[R2:\$w[0-9]+]], 0($6)
  %3 = sub <8 x i16> %1, %2
  ; CHECK-DAG: subv.h [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <8 x i16> %3, <8 x i16>* %c
  ; CHECK-DAG: st.h [[R3]], 0($4)

  ret void
  ; CHECK: .size sub_v8i16
}

define void @sub_v4i32(<4 x i32>* %c, <4 x i32>* %a, <4 x i32>* %b) nounwind {
  ; CHECK: sub_v4i32:

  %1 = load <4 x i32>* %a
  ; CHECK-DAG: ld.w [[R1:\$w[0-9]+]], 0($5)
  %2 = load <4 x i32>* %b
  ; CHECK-DAG: ld.w [[R2:\$w[0-9]+]], 0($6)
  %3 = sub <4 x i32> %1, %2
  ; CHECK-DAG: subv.w [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <4 x i32> %3, <4 x i32>* %c
  ; CHECK-DAG: st.w [[R3]], 0($4)

  ret void
  ; CHECK: .size sub_v4i32
}

define void @sub_v2i64(<2 x i64>* %c, <2 x i64>* %a, <2 x i64>* %b) nounwind {
  ; CHECK: sub_v2i64:

  %1 = load <2 x i64>* %a
  ; CHECK-DAG: ld.d [[R1:\$w[0-9]+]], 0($5)
  %2 = load <2 x i64>* %b
  ; CHECK-DAG: ld.d [[R2:\$w[0-9]+]], 0($6)
  %3 = sub <2 x i64> %1, %2
  ; CHECK-DAG: subv.d [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <2 x i64> %3, <2 x i64>* %c
  ; CHECK-DAG: st.d [[R3]], 0($4)

  ret void
  ; CHECK: .size sub_v2i64
}

define void @sub_v16i8_i(<16 x i8>* %c, <16 x i8>* %a) nounwind {
  ; CHECK: sub_v16i8_i:

  %1 = load <16 x i8>* %a
  ; CHECK-DAG: ld.b [[R1:\$w[0-9]+]], 0($5)
  %2 = sub <16 x i8> %1, <i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1,
                          i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1>
  ; CHECK-DAG: subvi.b [[R3:\$w[0-9]+]], [[R1]], 1
  store <16 x i8> %2, <16 x i8>* %c
  ; CHECK-DAG: st.b [[R3]], 0($4)

  ret void
  ; CHECK: .size sub_v16i8_i
}

define void @sub_v8i16_i(<8 x i16>* %c, <8 x i16>* %a) nounwind {
  ; CHECK: sub_v8i16_i:

  %1 = load <8 x i16>* %a
  ; CHECK-DAG: ld.h [[R1:\$w[0-9]+]], 0($5)
  %2 = sub <8 x i16> %1, <i16 1, i16 1, i16 1, i16 1,
                          i16 1, i16 1, i16 1, i16 1>
  ; CHECK-DAG: subvi.h [[R3:\$w[0-9]+]], [[R1]], 1
  store <8 x i16> %2, <8 x i16>* %c
  ; CHECK-DAG: st.h [[R3]], 0($4)

  ret void
  ; CHECK: .size sub_v8i16_i
}

define void @sub_v4i32_i(<4 x i32>* %c, <4 x i32>* %a) nounwind {
  ; CHECK: sub_v4i32_i:

  %1 = load <4 x i32>* %a
  ; CHECK-DAG: ld.w [[R1:\$w[0-9]+]], 0($5)
  %2 = sub <4 x i32> %1, <i32 1, i32 1, i32 1, i32 1>
  ; CHECK-DAG: subvi.w [[R3:\$w[0-9]+]], [[R1]], 1
  store <4 x i32> %2, <4 x i32>* %c
  ; CHECK-DAG: st.w [[R3]], 0($4)

  ret void
  ; CHECK: .size sub_v4i32_i
}

define void @sub_v2i64_i(<2 x i64>* %c, <2 x i64>* %a) nounwind {
  ; CHECK: sub_v2i64_i:

  %1 = load <2 x i64>* %a
  ; CHECK-DAG: ld.d [[R1:\$w[0-9]+]], 0($5)
  %2 = sub <2 x i64> %1, <i64 1, i64 1>
  ; CHECK-DAG: subvi.d [[R3:\$w[0-9]+]], [[R1]], 1
  store <2 x i64> %2, <2 x i64>* %c
  ; CHECK-DAG: st.d [[R3]], 0($4)

  ret void
  ; CHECK: .size sub_v2i64_i
}

define void @mul_v16i8(<16 x i8>* %c, <16 x i8>* %a, <16 x i8>* %b) nounwind {
  ; CHECK: mul_v16i8:

  %1 = load <16 x i8>* %a
  ; CHECK-DAG: ld.b [[R1:\$w[0-9]+]], 0($5)
  %2 = load <16 x i8>* %b
  ; CHECK-DAG: ld.b [[R2:\$w[0-9]+]], 0($6)
  %3 = mul <16 x i8> %1, %2
  ; CHECK-DAG: mulv.b [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <16 x i8> %3, <16 x i8>* %c
  ; CHECK-DAG: st.b [[R3]], 0($4)

  ret void
  ; CHECK: .size mul_v16i8
}

define void @mul_v8i16(<8 x i16>* %c, <8 x i16>* %a, <8 x i16>* %b) nounwind {
  ; CHECK: mul_v8i16:

  %1 = load <8 x i16>* %a
  ; CHECK-DAG: ld.h [[R1:\$w[0-9]+]], 0($5)
  %2 = load <8 x i16>* %b
  ; CHECK-DAG: ld.h [[R2:\$w[0-9]+]], 0($6)
  %3 = mul <8 x i16> %1, %2
  ; CHECK-DAG: mulv.h [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <8 x i16> %3, <8 x i16>* %c
  ; CHECK-DAG: st.h [[R3]], 0($4)

  ret void
  ; CHECK: .size mul_v8i16
}

define void @mul_v4i32(<4 x i32>* %c, <4 x i32>* %a, <4 x i32>* %b) nounwind {
  ; CHECK: mul_v4i32:

  %1 = load <4 x i32>* %a
  ; CHECK-DAG: ld.w [[R1:\$w[0-9]+]], 0($5)
  %2 = load <4 x i32>* %b
  ; CHECK-DAG: ld.w [[R2:\$w[0-9]+]], 0($6)
  %3 = mul <4 x i32> %1, %2
  ; CHECK-DAG: mulv.w [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <4 x i32> %3, <4 x i32>* %c
  ; CHECK-DAG: st.w [[R3]], 0($4)

  ret void
  ; CHECK: .size mul_v4i32
}

define void @mul_v2i64(<2 x i64>* %c, <2 x i64>* %a, <2 x i64>* %b) nounwind {
  ; CHECK: mul_v2i64:

  %1 = load <2 x i64>* %a
  ; CHECK-DAG: ld.d [[R1:\$w[0-9]+]], 0($5)
  %2 = load <2 x i64>* %b
  ; CHECK-DAG: ld.d [[R2:\$w[0-9]+]], 0($6)
  %3 = mul <2 x i64> %1, %2
  ; CHECK-DAG: mulv.d [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <2 x i64> %3, <2 x i64>* %c
  ; CHECK-DAG: st.d [[R3]], 0($4)

  ret void
  ; CHECK: .size mul_v2i64
}

define void @div_s_v16i8(<16 x i8>* %c, <16 x i8>* %a, <16 x i8>* %b) nounwind {
  ; CHECK: div_s_v16i8:

  %1 = load <16 x i8>* %a
  ; CHECK-DAG: ld.b [[R1:\$w[0-9]+]], 0($5)
  %2 = load <16 x i8>* %b
  ; CHECK-DAG: ld.b [[R2:\$w[0-9]+]], 0($6)
  %3 = sdiv <16 x i8> %1, %2
  ; CHECK-DAG: div_s.b [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <16 x i8> %3, <16 x i8>* %c
  ; CHECK-DAG: st.b [[R3]], 0($4)

  ret void
  ; CHECK: .size div_s_v16i8
}

define void @div_s_v8i16(<8 x i16>* %c, <8 x i16>* %a, <8 x i16>* %b) nounwind {
  ; CHECK: div_s_v8i16:

  %1 = load <8 x i16>* %a
  ; CHECK-DAG: ld.h [[R1:\$w[0-9]+]], 0($5)
  %2 = load <8 x i16>* %b
  ; CHECK-DAG: ld.h [[R2:\$w[0-9]+]], 0($6)
  %3 = sdiv <8 x i16> %1, %2
  ; CHECK-DAG: div_s.h [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <8 x i16> %3, <8 x i16>* %c
  ; CHECK-DAG: st.h [[R3]], 0($4)

  ret void
  ; CHECK: .size div_s_v8i16
}

define void @div_s_v4i32(<4 x i32>* %c, <4 x i32>* %a, <4 x i32>* %b) nounwind {
  ; CHECK: div_s_v4i32:

  %1 = load <4 x i32>* %a
  ; CHECK-DAG: ld.w [[R1:\$w[0-9]+]], 0($5)
  %2 = load <4 x i32>* %b
  ; CHECK-DAG: ld.w [[R2:\$w[0-9]+]], 0($6)
  %3 = sdiv <4 x i32> %1, %2
  ; CHECK-DAG: div_s.w [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <4 x i32> %3, <4 x i32>* %c
  ; CHECK-DAG: st.w [[R3]], 0($4)

  ret void
  ; CHECK: .size div_s_v4i32
}

define void @div_s_v2i64(<2 x i64>* %c, <2 x i64>* %a, <2 x i64>* %b) nounwind {
  ; CHECK: div_s_v2i64:

  %1 = load <2 x i64>* %a
  ; CHECK-DAG: ld.d [[R1:\$w[0-9]+]], 0($5)
  %2 = load <2 x i64>* %b
  ; CHECK-DAG: ld.d [[R2:\$w[0-9]+]], 0($6)
  %3 = sdiv <2 x i64> %1, %2
  ; CHECK-DAG: div_s.d [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <2 x i64> %3, <2 x i64>* %c
  ; CHECK-DAG: st.d [[R3]], 0($4)

  ret void
  ; CHECK: .size div_s_v2i64
}

define void @div_u_v16i8(<16 x i8>* %c, <16 x i8>* %a, <16 x i8>* %b) nounwind {
  ; CHECK: div_u_v16i8:

  %1 = load <16 x i8>* %a
  ; CHECK-DAG: ld.b [[R1:\$w[0-9]+]], 0($5)
  %2 = load <16 x i8>* %b
  ; CHECK-DAG: ld.b [[R2:\$w[0-9]+]], 0($6)
  %3 = udiv <16 x i8> %1, %2
  ; CHECK-DAG: div_u.b [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <16 x i8> %3, <16 x i8>* %c
  ; CHECK-DAG: st.b [[R3]], 0($4)

  ret void
  ; CHECK: .size div_u_v16i8
}

define void @div_u_v8i16(<8 x i16>* %c, <8 x i16>* %a, <8 x i16>* %b) nounwind {
  ; CHECK: div_u_v8i16:

  %1 = load <8 x i16>* %a
  ; CHECK-DAG: ld.h [[R1:\$w[0-9]+]], 0($5)
  %2 = load <8 x i16>* %b
  ; CHECK-DAG: ld.h [[R2:\$w[0-9]+]], 0($6)
  %3 = udiv <8 x i16> %1, %2
  ; CHECK-DAG: div_u.h [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <8 x i16> %3, <8 x i16>* %c
  ; CHECK-DAG: st.h [[R3]], 0($4)

  ret void
  ; CHECK: .size div_u_v8i16
}

define void @div_u_v4i32(<4 x i32>* %c, <4 x i32>* %a, <4 x i32>* %b) nounwind {
  ; CHECK: div_u_v4i32:

  %1 = load <4 x i32>* %a
  ; CHECK-DAG: ld.w [[R1:\$w[0-9]+]], 0($5)
  %2 = load <4 x i32>* %b
  ; CHECK-DAG: ld.w [[R2:\$w[0-9]+]], 0($6)
  %3 = udiv <4 x i32> %1, %2
  ; CHECK-DAG: div_u.w [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <4 x i32> %3, <4 x i32>* %c
  ; CHECK-DAG: st.w [[R3]], 0($4)

  ret void
  ; CHECK: .size div_u_v4i32
}

define void @div_u_v2i64(<2 x i64>* %c, <2 x i64>* %a, <2 x i64>* %b) nounwind {
  ; CHECK: div_u_v2i64:

  %1 = load <2 x i64>* %a
  ; CHECK-DAG: ld.d [[R1:\$w[0-9]+]], 0($5)
  %2 = load <2 x i64>* %b
  ; CHECK-DAG: ld.d [[R2:\$w[0-9]+]], 0($6)
  %3 = udiv <2 x i64> %1, %2
  ; CHECK-DAG: div_u.d [[R3:\$w[0-9]+]], [[R1]], [[R2]]
  store <2 x i64> %3, <2 x i64>* %c
  ; CHECK-DAG: st.d [[R3]], 0($4)

  ret void
  ; CHECK: .size div_u_v2i64
}
