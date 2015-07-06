; RUN: opt -memcpyopt -S %s | FileCheck %s

target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"

; CHECK-LABEL: define void @test
; CHECK-NEXT: call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %src_size, i32 1, i1 false)
; CHECK-DAG: [[DST:%[0-9]+]] = getelementptr i8, i8* %dst, i64 %src_size
; CHECK-DAG: [[ULE:%[0-9]+]] = icmp ule i64 %dst_size, %src_size
; CHECK-DAG: [[SIZEDIFF:%[0-9]+]] = sub i64 %dst_size, %src_size
; CHECK-DAG: [[SIZE:%[0-9]+]] = select i1 [[ULE]], i64 0, i64 [[SIZEDIFF]]
; CHECK-NEXT: call void @llvm.memset.p0i8.i64(i8* [[DST]], i8 %c, i64 [[SIZE]], i32 1, i1 false)
; CHECK-NEXT: ret void
define void @test(i8* %src, i64 %src_size, i8* %dst, i64 %dst_size, i8 %c) {
  call void @llvm.memset.p0i8.i64(i8* %dst, i8 %c, i64 %dst_size, i32 1, i1 false)
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %src_size, i32 1, i1 false)
  ret void
}

; CHECK-LABEL: define void @test_different_types_i32_i64
; CHECK-NEXT: call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %src_size, i32 1, i1 false)
; CHECK-DAG: [[DSTSIZE:%[0-9]+]] = zext i32 %dst_size to i64
; CHECK-DAG: [[DST:%[0-9]+]] = getelementptr i8, i8* %dst, i64 %src_size
; CHECK-DAG: [[ULE:%[0-9]+]] = icmp ule i64 [[DSTSIZE]], %src_size
; CHECK-DAG: [[SIZEDIFF:%[0-9]+]] = sub i64 [[DSTSIZE]], %src_size
; CHECK-DAG: [[SIZE:%[0-9]+]] = select i1 [[ULE]], i64 0, i64 [[SIZEDIFF]]
; CHECK-NEXT: call void @llvm.memset.p0i8.i64(i8* [[DST]], i8 %c, i64 [[SIZE]], i32 1, i1 false)
; CHECK-NEXT: ret void
define void @test_different_types_i32_i64(i8* %dst, i8* %src, i32 %dst_size, i64 %src_size, i8 %c) {
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 %c, i32 %dst_size, i32 1, i1 false)
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %src_size, i32 1, i1 false)
  ret void
}

; CHECK-LABEL: define void @test_different_types_i128_i32
; CHECK-NEXT: call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src, i32 %src_size, i32 1, i1 false)
; CHECK-DAG: [[SRCSIZE:%[0-9]+]] = zext i32 %src_size to i128
; CHECK-DAG: [[DST:%[0-9]+]] = getelementptr i8, i8* %dst, i128 [[SRCSIZE]]
; CHECK-DAG: [[ULE:%[0-9]+]] = icmp ule i128 %dst_size, [[SRCSIZE]]
; CHECK-DAG: [[SIZEDIFF:%[0-9]+]] = sub i128 %dst_size, [[SRCSIZE]]
; CHECK-DAG: [[SIZE:%[0-9]+]] = select i1 [[ULE]], i128 0, i128 [[SIZEDIFF]]
; CHECK-NEXT: call void @llvm.memset.p0i8.i128(i8* [[DST]], i8 %c, i128 [[SIZE]], i32 1, i1 false)
; CHECK-NEXT: ret void
define void @test_different_types_i128_i32(i8* %dst, i8* %src, i128 %dst_size, i32 %src_size, i8 %c) {
  call void @llvm.memset.p0i8.i128(i8* %dst, i8 %c, i128 %dst_size, i32 1, i1 false)
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src, i32 %src_size, i32 1, i1 false)
  ret void
}

; CHECK-LABEL: define void @test_different_types_i32_i128
; CHECK-NEXT: call void @llvm.memcpy.p0i8.p0i8.i128(i8* %dst, i8* %src, i128 %src_size, i32 1, i1 false)
; CHECK-DAG: [[DSTSIZE:%[0-9]+]] = zext i32 %dst_size to i128
; CHECK-DAG: [[DST:%[0-9]+]] = getelementptr i8, i8* %dst, i128 %src_size
; CHECK-DAG: [[ULE:%[0-9]+]] = icmp ule i128 [[DSTSIZE]], %src_size
; CHECK-DAG: [[SIZEDIFF:%[0-9]+]] = sub i128 [[DSTSIZE]], %src_size
; CHECK-DAG: [[SIZE:%[0-9]+]] = select i1 [[ULE]], i128 0, i128 [[SIZEDIFF]]
; CHECK-NEXT: call void @llvm.memset.p0i8.i128(i8* [[DST]], i8 %c, i128 [[SIZE]], i32 1, i1 false)
; CHECK-NEXT: ret void
define void @test_different_types_i32_i128(i8* %dst, i8* %src, i32 %dst_size, i128 %src_size, i8 %c) {
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 %c, i32 %dst_size, i32 1, i1 false)
  call void @llvm.memcpy.p0i8.p0i8.i128(i8* %dst, i8* %src, i128 %src_size, i32 1, i1 false)
  ret void
}

; CHECK-LABEL: define void @test_different_types_i64_i32
; CHECK-NEXT: call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src, i32 %src_size, i32 1, i1 false)
; CHECK-DAG: [[SRCSIZE:%[0-9]+]] = zext i32 %src_size to i64
; CHECK-DAG: [[DST:%[0-9]+]] = getelementptr i8, i8* %dst, i64 [[SRCSIZE]]
; CHECK-DAG: [[ULE:%[0-9]+]] = icmp ule i64 %dst_size, [[SRCSIZE]]
; CHECK-DAG: [[SIZEDIFF:%[0-9]+]] = sub i64 %dst_size, [[SRCSIZE]]
; CHECK-DAG: [[SIZE:%[0-9]+]] = select i1 [[ULE]], i64 0, i64 [[SIZEDIFF]]
; CHECK-NEXT: call void @llvm.memset.p0i8.i64(i8* [[DST]], i8 %c, i64 [[SIZE]], i32 1, i1 false)
; CHECK-NEXT: ret void
define void @test_different_types_i64_i32(i8* %dst, i8* %src, i64 %dst_size, i32 %src_size, i8 %c) {
  call void @llvm.memset.p0i8.i64(i8* %dst, i8 %c, i64 %dst_size, i32 1, i1 false)
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src, i32 %src_size, i32 1, i1 false)
  ret void
}

; CHECK-LABEL: define void @test_align_same
; CHECK: call void @llvm.memset.p0i8.i64(i8* {{.*}}, i8 0, i64 {{.*}}, i32 8, i1 false)
define void @test_align_same(i8* %src, i8* %dst, i64 %dst_size) {
  call void @llvm.memset.p0i8.i64(i8* %dst, i8 0, i64 %dst_size, i32 8, i1 false)
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 80, i32 1, i1 false)
  ret void
}

; CHECK-LABEL: define void @test_align_min
; CHECK: call void @llvm.memset.p0i8.i64(i8* {{.*}}, i8 0, i64 {{.*}}, i32 4, i1 false)
define void @test_align_min(i8* %src, i8* %dst, i64 %dst_size) {
  call void @llvm.memset.p0i8.i64(i8* %dst, i8 0, i64 %dst_size, i32 8, i1 false)
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 36, i32 1, i1 false)
  ret void
}

; CHECK-LABEL: define void @test_align_memcpy
; CHECK: call void @llvm.memset.p0i8.i64(i8* {{.*}}, i8 0, i64 {{.*}}, i32 8, i1 false)
define void @test_align_memcpy(i8* %src, i8* %dst, i64 %dst_size) {
  call void @llvm.memset.p0i8.i64(i8* %dst, i8 0, i64 %dst_size, i32 1, i1 false)
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 80, i32 8, i1 false)
  ret void
}

; CHECK-LABEL: define void @test_non_i8_dst_type
; CHECK-NEXT: %dst = bitcast i64* %dst_pi64 to i8*
; CHECK-NEXT: call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %src_size, i32 1, i1 false)
; CHECK-DAG: [[DST:%[0-9]+]] = getelementptr i8, i8* %dst, i64 %src_size
; CHECK-DAG: [[ULE:%[0-9]+]] = icmp ule i64 %dst_size, %src_size
; CHECK-DAG: [[SIZEDIFF:%[0-9]+]] = sub i64 %dst_size, %src_size
; CHECK-DAG: [[SIZE:%[0-9]+]] = select i1 [[ULE]], i64 0, i64 [[SIZEDIFF]]
; CHECK-NEXT: call void @llvm.memset.p0i8.i64(i8* [[DST]], i8 %c, i64 [[SIZE]], i32 1, i1 false)
; CHECK-NEXT: ret void
define void @test_non_i8_dst_type(i8* %src, i64 %src_size, i64* %dst_pi64, i64 %dst_size, i8 %c) {
  %dst = bitcast i64* %dst_pi64 to i8*
  call void @llvm.memset.p0i8.i64(i8* %dst, i8 %c, i64 %dst_size, i32 1, i1 false)
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %src_size, i32 1, i1 false)
  ret void
}

; CHECK-LABEL: define void @test_different_dst
; CHECK-NEXT: call void @llvm.memset.p0i8.i64(i8* %dst, i8 0, i64 %dst_size, i32 1, i1 false)
; CHECK-NEXT: call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst2, i8* %src, i64 %src_size, i32 1, i1 false)
; CHECK-NEXT: ret void
define void @test_different_dst(i8* %dst2, i8* %src, i64 %src_size, i8* %dst, i64 %dst_size) {
  call void @llvm.memset.p0i8.i64(i8* %dst, i8 0, i64 %dst_size, i32 1, i1 false)
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst2, i8* %src, i64 %src_size, i32 1, i1 false)
  ret void
}

declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1)
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1)
declare void @llvm.memset.p0i8.i32(i8* nocapture, i8, i32, i32, i1)
declare void @llvm.memcpy.p0i8.p0i8.i32(i8* nocapture, i8* nocapture readonly, i32, i32, i1)
declare void @llvm.memset.p0i8.i128(i8* nocapture, i8, i128, i32, i1)
declare void @llvm.memcpy.p0i8.p0i8.i128(i8* nocapture, i8* nocapture readonly, i128, i32, i1)
