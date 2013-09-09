// RUN: not llvm-mc -triple aarch64-none-linux-gnu -mattr=+neon < %s 2> %t
// RUN: FileCheck --check-prefix=CHECK-ERROR < %t %s

//------------------------------------------------------------------------------
// Vector Integer Add/sub
//------------------------------------------------------------------------------

        // Mismatched vector types
        add v0.16b, v1.8b, v2.8b
        sub v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         add v0.16b, v1.8b, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sub v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                              ^

//------------------------------------------------------------------------------
// Vector Floating-Point Add/sub
//------------------------------------------------------------------------------

        // Mismatched and invalid vector types
        fadd v0.2d, v1.2s, v2.2s
        fsub v0.4s, v1.2s, v2.4s
        fsub v0.8b, v1.8b, v2.8b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fadd v0.2d, v1.2s, v2.2s
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fsub v0.4s, v1.2s, v2.4s
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fsub v0.8b, v1.8b, v2.8b
// CHECK-ERROR:                  ^

//----------------------------------------------------------------------
// Vector Integer Mul
//----------------------------------------------------------------------

        // Mismatched and invalid vector types
        mul v0.16b, v1.8b, v2.8b
        mul v0.2d, v1.2d, v2.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         mul v0.16b, v1.8b, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         mul v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                ^

//----------------------------------------------------------------------
// Vector Floating-Point Mul/Div
//----------------------------------------------------------------------
        // Mismatched vector types
        fmul v0.16b, v1.8b, v2.8b
        fdiv v0.2s, v1.2d, v2.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fmul v0.16b, v1.8b, v2.8b
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fdiv v0.2s, v1.2d, v2.2d
// CHECK-ERROR:                        ^

//----------------------------------------------------------------------
// Vector And Orr Eor Bsl Bit Bif, Orn, Bic,
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        and v0.8b, v1.16b, v2.8b
        orr v0.4h, v1.4h, v2.4h
        eor v0.2s, v1.2s, v2.2s
        bsl v0.8b, v1.16b, v2.8b
        bsl v0.2s, v1.2s, v2.2s
        bit v0.2d, v1.2d, v2.2d
        bif v0.4h, v1.4h, v2.4h
        orn v0.8b, v1.16b, v2.16b
        bic v0.2d, v1.2d, v2.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         and v0.8b, v1.16b, v2.8b
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         orr v0.4h, v1.4h, v2.4h
// CHECK-ERROR:                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         eor v0.2s, v1.2s, v2.2s
// CHECK-ERROR:                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         bsl v0.8b, v1.16b, v2.8b
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         bsl v0.2s, v1.2s, v2.2s
// CHECK-ERROR:                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         bit v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         bif v0.4h, v1.4h, v2.4h
// CHECK-ERROR:                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         orn v0.8b, v1.16b, v2.16b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         bic v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                ^

//----------------------------------------------------------------------
// Vector Integer Multiply-accumulate and Multiply-subtract
//----------------------------------------------------------------------

        // Mismatched and invalid vector types
        mla v0.16b, v1.8b, v2.8b
        mls v0.2d, v1.2d, v2.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         mla v0.16b, v1.8b, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         mls v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                ^

//----------------------------------------------------------------------
// Vector Floating-Point Multiply-accumulate and Multiply-subtract
//----------------------------------------------------------------------
        // Mismatched vector types
        fmla v0.2s, v1.2d, v2.2d
        fmls v0.16b, v1.8b, v2.8b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fmla v0.2s, v1.2d, v2.2d
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fmls v0.16b, v1.8b, v2.8b
// CHECK-ERROR:                         ^


//----------------------------------------------------------------------
// Vector Move Immediate Shifted
// Vector Move Inverted Immediate Shifted
// Vector Bitwise Bit Clear (AND NOT) - immediate
// Vector Bitwise OR - immedidate
//----------------------------------------------------------------------
      // out of range immediate (0 to 0xff)
      movi v0.2s, #-1
      mvni v1.4s, #256
      // out of range shift (0, 8, 16, 24 and 0, 8)
      bic v15.4h, #1, lsl #7
      orr v31.2s, #1, lsl #25
      movi v5.4h, #10, lsl #16
      // invalid vector type (2s, 4s, 4h, 8h)
      movi v5.8b, #1, lsl #8

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:          movi v0.2s, #-1
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         mvni v1.4s, #256
// CHECK-ERROR:                     ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         bic v15.4h, #1, lsl #7
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         orr v31.2s, #1, lsl #25
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         movi v5.4h, #10, lsl #16
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         movi v5.8b, #1, lsl #8
// CHECK-ERROR:                         ^
//----------------------------------------------------------------------
// Vector Move Immediate Masked
// Vector Move Inverted Immediate Masked
//----------------------------------------------------------------------
      // out of range immediate (0 to 0xff)
      movi v0.2s, #-1, msl #8
      mvni v7.4s, #256, msl #16
      // out of range shift (8, 16)
      movi v3.2s, #1, msl #0
      mvni v17.4s, #255, msl #32
      // invalid vector type (2s, 4s)
      movi v5.4h, #31, msl #8

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         movi v0.2s, #-1, msl #8
// CHECK-ERROR:                     ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         mvni v7.4s, #256, msl #16
// CHECK-ERROR:                     ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         movi v3.2s, #1, msl #0
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         mvni v17.4s, #255, msl #32
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         movi v5.4h, #31, msl #8
// CHECK-ERROR:                          ^

//----------------------------------------------------------------------
// Vector Immediate - per byte
//----------------------------------------------------------------------
        // out of range immediate (0 to 0xff)
        movi v0.8b, #-1
        movi v1.16b, #256

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         movi v0.8b, #-1
// CHECK-ERROR:                     ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         movi v1.16b, #256
// CHECK-ERROR:                      ^


//----------------------------------------------------------------------
// Vector Move Immediate - bytemask, per doubleword
//---------------------------------------------------------------------
        // invalid bytemask (0x00 or 0xff)
        movi v0.2d, #0x10ff00ff00ff00ff

// CHECK:ERROR: error: invalid operand for instruction
// CHECK:ERROR:         movi v0.2d, #0x10ff00ff00ff00ff
// CHECK:ERROR:                     ^

//----------------------------------------------------------------------
// Vector Move Immediate - bytemask, one doubleword
//----------------------------------------------------------------------
        // invalid bytemask (0x00 or 0xff)
        movi v0.2d, #0xffff00ff001f00ff

// CHECK:ERROR: error: invalid operand for instruction
// CHECK:ERROR:         movi v0.2d, #0xffff00ff001f00ff
// CHECK:ERROR:                     ^
//----------------------------------------------------------------------
// Vector Floating Point Move Immediate
//----------------------------------------------------------------------
        // invalid vector type (2s, 4s, 2d)
         fmov v0.4h, #1.0

// CHECK:ERROR: error: invalid operand for instruction
// CHECK:ERROR:         fmov v0.4h, #1.0
// CHECK:ERROR:              ^

//----------------------------------------------------------------------
// Vector Move -  register
//----------------------------------------------------------------------
      // invalid vector type (8b, 16b)
      mov v0.2s, v31.8b
// CHECK:ERROR: error: invalid operand for instruction
// CHECK:ERROR:         mov v0.2s, v31.8b
// CHECK:ERROR:                ^

//----------------------------------------------------------------------
// Vector Absolute Difference and Accumulate (Signed, Unsigned)
//----------------------------------------------------------------------

        // Mismatched and invalid vector types (2d)
        saba v0.16b, v1.8b, v2.8b
        uaba v0.2d, v1.2d, v2.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         saba v0.16b, v1.8b, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uaba v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                ^

//----------------------------------------------------------------------
// Vector Absolute Difference and Accumulate (Signed, Unsigned)
// Vector Absolute Difference (Signed, Unsigned)

        // Mismatched and invalid vector types (2d)
        uaba v0.16b, v1.8b, v2.8b
        saba v0.2d, v1.2d, v2.2d
        uabd v0.4s, v1.2s, v2.2s
        sabd v0.4h, v1.8h, v8.8h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uaba v0.16b, v1.8b, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         saba v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uabd v0.4s, v1.2s, v2.2s
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sabd v0.4h, v1.8h, v8.8h
// CHECK-ERROR:                        ^

//----------------------------------------------------------------------
// Vector Absolute Difference (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        fabd v0.2s, v1.4s, v2.2d
        fabd v0.4h, v1.4h, v2.4h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fabd v0.2s, v1.4s, v2.2d
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fabd v0.4h, v1.4h, v2.4h
// CHECK-ERROR:                 ^
//----------------------------------------------------------------------
// Vector Multiply (Polynomial)
//----------------------------------------------------------------------

        // Mismatched and invalid vector types
         pmul v0.8b, v1.8b, v2.16b
         pmul v0.2s, v1.2s, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         pmul v0.8b, v1.8b, v2.16b
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         pmul v0.2s, v1.2s, v2.2s
// CHECK-ERROR:                 ^

//----------------------------------------------------------------------
// Scalar Integer Add and Sub
//----------------------------------------------------------------------

      // Mismatched registers
         add d0, s1, d2
         sub s1, d1, d2

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         add d0, s1, d2
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sub s1, d1, d2
// CHECK-ERROR:             ^

//----------------------------------------------------------------------
// Vector Reciprocal Step (Floating Point)
//----------------------------------------------------------------------

        // Mismatched and invalid vector types
         frecps v0.4s, v1.2d, v2.4s
         frecps v0.8h, v1.8h, v2.8h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        frecps v0.4s, v1.2d, v2.4s
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        frecps v0.8h, v1.8h, v2.8h
// CHECK-ERROR:                  ^

//----------------------------------------------------------------------
// Vector Reciprocal Square Root Step (Floating Point)
//----------------------------------------------------------------------

        // Mismatched and invalid vector types
         frsqrts v0.2d, v1.2d, v2.2s
         frsqrts v0.4h, v1.4h, v2.4h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        frsqrts v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        frsqrts v0.4h, v1.4h, v2.4h
// CHECK-ERROR:                   ^


//----------------------------------------------------------------------
// Vector Absolute Compare Mask Less Than Or Equal (Floating Point)
//----------------------------------------------------------------------

        // Mismatched and invalid vector types
        facge v0.2d, v1.2s, v2.2d
        facge v0.4h, v1.4h, v2.4h
        facle v0.8h, v1.4h, v2.4h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        facge v0.2d, v1.2s, v2.2d
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        facge v0.4h, v1.4h, v2.4h
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        facle v0.8h, v1.4h, v2.4h
// CHECK-ERROR:                 ^
//----------------------------------------------------------------------
// Vector Absolute Compare Mask Less Than (Floating Point)
//----------------------------------------------------------------------

        // Mismatched and invalid vector types
        facgt v0.2d, v1.2d, v2.4s
        facgt v0.8h, v1.8h, v2.8h
        faclt v0.8b, v1.8b, v2.8b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        facgt v0.2d, v1.2d, v2.4s
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        facgt v0.8h, v1.8h, v2.8h
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        faclt v0.8b, v1.8b, v2.8b
// CHECK-ERROR:                 ^


//----------------------------------------------------------------------
// Vector Compare Mask Equal (Integer)
//----------------------------------------------------------------------

         // Mismatched vector types
         cmeq c0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        cmeq c0.2d, v1.2d, v2.2s
// CHECK-ERROR:                              ^

//----------------------------------------------------------------------
// Vector Compare Mask Higher or Same (Unsigned Integer)
// Vector Compare Mask Less or Same (Unsigned Integer)
// CMLS is alias for CMHS with operands reversed.
//----------------------------------------------------------------------

         // Mismatched vector types
         cmhs c0.4h, v1.8b, v2.8b
         cmls c0.16b, v1.16b, v2.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        cmhs c0.4h, v1.8b, v2.8b
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        cmls c0.16b, v1.16b, v2.2d
// CHECK-ERROR:                                ^

//----------------------------------------------------------------------
// Vector Compare Mask Greater Than or Equal (Integer)
// Vector Compare Mask Less Than or Equal (Integer)
// CMLE is alias for CMGE with operands reversed.
//----------------------------------------------------------------------

         // Mismatched vector types
         cmge c0.8h, v1.8b, v2.8b
         cmle c0.4h, v1.2s, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        cmge c0.8h, v1.8b, v2.8b
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         cmle c0.4h, v1.2s, v2.2s
// CHECK-ERROR:                        ^

//----------------------------------------------------------------------
// Vector Compare Mask Higher (Unsigned Integer)
// Vector Compare Mask Lower (Unsigned Integer)
// CMLO is alias for CMHI with operands reversed.
//----------------------------------------------------------------------

         // Mismatched vector types
         cmhi c0.4s, v1.4s, v2.16b
         cmlo c0.8b, v1.8b, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        cmhi c0.4s, v1.4s, v2.16b
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         cmlo c0.8b, v1.8b, v2.2s
// CHECK-ERROR:                               ^

//----------------------------------------------------------------------
// Vector Compare Mask Greater Than (Integer)
// Vector Compare Mask Less Than (Integer)
// CMLT is alias for CMGT with operands reversed.
//----------------------------------------------------------------------

         // Mismatched vector types
         cmgt c0.8b, v1.4s, v2.16b
         cmlt c0.8h, v1.16b, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         cmgt c0.8b, v1.4s, v2.16b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         cmlt c0.8h, v1.16b, v2.4s
// CHECK-ERROR:                        ^

//----------------------------------------------------------------------
// Vector Compare Mask Bitwise Test (Integer)
//----------------------------------------------------------------------

         // Mismatched vector types
         cmtst c0.16b, v1.16b, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         cmtst c0.16b, v1.16b, v2.4s
// CHECK-ERROR:                                  ^

//----------------------------------------------------------------------
// Vector Compare Mask Equal (Floating Point)
//----------------------------------------------------------------------

        // Mismatched and invalid vector types
        fcmeq v0.2d, v1.2s, v2.2d
        fcmeq v0.16b, v1.16b, v2.16b
        fcmeq v0.8b, v1.4h, v2.4h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmeq v0.2d, v1.2s, v2.2d
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmeq v0.16b, v1.16b, v2.16b
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmeq v0.8b, v1.4h, v2.4h
// CHECK-ERROR:                 ^

//----------------------------------------------------------------------
// Vector Compare Mask Greater Than Or Equal (Floating Point)
// Vector Compare Mask Less Than Or Equal (Floating Point)
// FCMLE is alias for FCMGE with operands reversed.
//----------------------------------------------------------------------

        // Mismatched and invalid vector types
         fcmge v31.4s, v29.2s, v28.4s
         fcmge v3.8b, v8.2s, v12.2s
         fcmle v17.8h, v15.2d, v13.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmge v31.4s, v29.2s, v28.4s
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmge v3.8b, v8.2s, v12.2s
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmle v17.8h, v15.2d, v13.2d
// CHECK-ERROR:                 ^

//----------------------------------------------------------------------
// Vector Compare Mask Greater Than (Floating Point)
// Vector Compare Mask Less Than (Floating Point)
// FCMLT is alias for FCMGT with operands reversed.
//----------------------------------------------------------------------

        // Mismatched and invalid vector types
         fcmgt v0.2d, v31.2s, v16.2s
         fcmgt v4.4s, v7.4s, v15.4h
         fcmlt v29.2d, v5.2d, v2.16b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmgt v0.2d, v31.2s, v16.2s
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: expected floating-point constant #0.0 or invalid register type
// CHECK-ERROR:        fcmgt v4.4s, v7.4s, v15.4h
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: expected floating-point constant #0.0 or invalid register type
// CHECK-ERROR:        fcmlt v29.2d, v5.2d, v2.16b
// CHECK-ERROR:                                ^

//----------------------------------------------------------------------
// Vector Compare Mask Equal to Zero (Integer)
//----------------------------------------------------------------------
        // Mismatched vector types and invalid imm
         // Mismatched vector types
         cmeq c0.2d, v1.2s, #0
         cmeq c0.2d, v1.2d, #1

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        cmeq c0.2d, v1.2s, #0
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        cmeq c0.2d, v1.2d, #1
// CHECK-ERROR:                            ^

//----------------------------------------------------------------------
// Vector Compare Mask Greater Than or Equal to Zero (Signed Integer)
//----------------------------------------------------------------------
        // Mismatched vector types and invalid imm
         cmge c0.8h, v1.8b, #0
         cmge c0.4s, v1.4s, #-1

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        cmge c0.8h, v1.8b, #0
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         cmge c0.4s, v1.4s, #-1
// CHECK-ERROR:                             ^

//----------------------------------------------------------------------
// Vector Compare Mask Greater Than Zero (Signed Integer)
//----------------------------------------------------------------------
        // Mismatched vector types and invalid imm
         cmgt c0.8b, v1.4s, #0
         cmgt c0.8b, v1.8b, #-255

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         cmgt c0.8b, v1.4s, #0
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         cmgt c0.8b, v1.8b, #-255
// CHECK-ERROR:                             ^

//----------------------------------------------------------------------
// Vector Compare Mask Less Than or Equal To Zero (Signed Integer)
//----------------------------------------------------------------------
        // Mismatched vector types and invalid imm
         cmle c0.4h, v1.2s, #0
         cmle c0.16b, v1.16b, #16

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        cmle c0.4h, v1.2s, #0
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         cmle c0.16b, v1.16b, #16
// CHECK-ERROR:                               ^
//----------------------------------------------------------------------
// Vector Compare Mask Less Than Zero (Signed Integer)
//----------------------------------------------------------------------
        // Mismatched vector types and invalid imm
         cmlt c0.8h, v1.16b, #0
         cmlt c0.8h, v1.8h, #-15

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         cmlt c0.8h, v1.16b, #0
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         cmlt c0.8h, v1.8h, #-15
// CHECK-ERROR:                             ^

//----------------------------------------------------------------------
// Vector Compare Mask Equal to Zero (Floating Point)
//----------------------------------------------------------------------

        // Mismatched and invalid vector types, invalid imm
        fcmeq v0.2d, v1.2s, #0.0
        fcmeq v0.16b, v1.16b, #0.0
        fcmeq v0.8b, v1.4h, #1.0
        fcmeq v0.8b, v1.4h, #1

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmeq v0.2d, v1.2s, #0.0
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmeq v0.16b, v1.16b, #0.0
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmeq v0.8b, v1.4h, #1.0
// CHECK-ERROR:                             ^
// CHECK-ERROR: error:  Expected floating-point immediate
// CHECK-ERROR:        fcmeq v0.8b, v1.4h, #1
// CHECK-ERROR:                             ^
//----------------------------------------------------------------------
// Vector Compare Mask Greater Than or Equal to Zero (Floating Point)
//----------------------------------------------------------------------

        // Mismatched and invalid vector types, invalid imm
         fcmge v31.4s, v29.2s, #0.0
         fcmge v3.8b, v8.2s, #0.0
         fcmle v17.8h, v15.2d, #-1.0
         fcmle v17.8h, v15.2d, #0

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmge v31.4s, v29.2s, #0.0
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmge v3.8b, v8.2s, #0.0
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmle v17.8h, v15.2d, #-1.0
// CHECK-ERROR:                               ^
// CHECK-ERROR: error:  Expected floating-point immediate
// CHECK-ERROR:        fcmle v17.8h, v15.2d, #0
// CHECK-ERROR:                               ^
//----------------------------------------------------------------------
// Vector Compare Mask Greater Than Zero (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types, invalid imm
         fcmgt v0.2d, v31.2s, #0.0
         fcmgt v4.4s, v7.4h, #0.0
         fcmlt v29.2d, v5.2d, #255.0
         fcmlt v29.2d, v5.2d, #255

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmgt v0.2d, v31.2s, #0.0
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmgt v4.4s, v7.4h, #0.0
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: expected floating-point constant #0.0 or invalid register type
// CHECK-ERROR:        fcmlt v29.2d, v5.2d, #255.0
// CHECK-ERROR:                              ^
// CHECK-ERROR: error:  Expected floating-point immediate
// CHECK-ERROR:        fcmlt v29.2d, v5.2d, #255
// CHECK-ERROR:                              ^

//----------------------------------------------------------------------
// Vector Compare Mask Less Than or Equal To Zero (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types, invalid imm
         fcmge v31.4s, v29.2s, #0.0
         fcmge v3.8b, v8.2s, #0.0
         fcmle v17.2d, v15.2d, #15.0
         fcmle v17.2d, v15.2d, #15

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmge v31.4s, v29.2s, #0.0
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmge v3.8b, v8.2s, #0.0
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: expected floating-point constant #0.0 or invalid register type
// CHECK-ERROR:        fcmle v17.2d, v15.2d, #15.0
// CHECK-ERROR:                               ^
// CHECK-ERROR: error:  Expected floating-point immediate
// CHECK-ERROR:        fcmle v17.2d, v15.2d, #15
// CHECK-ERROR:                              ^

//----------------------------------------------------------------------
// Vector Compare Mask Less Than Zero (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types, invalid imm
         fcmgt v0.2d, v31.2s, #0.0
         fcmgt v4.4s, v7.4h, #0.0
         fcmlt v29.2d, v5.2d, #16.0
         fcmlt v29.2d, v5.2d, #2

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmgt v0.2d, v31.2s, #0.0
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fcmgt v4.4s, v7.4h, #0.0
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: expected floating-point constant #0.0 or invalid register type
// CHECK-ERROR:        fcmlt v29.2d, v5.2d, #16.0
// CHECK-ERROR:                              ^
// CHECK-ERROR: error:  Expected floating-point immediate
// CHECK-ERROR:        fcmlt v29.2d, v5.2d, #2
// CHECK-ERROR:                              ^

/-----------------------------------------------------------------------
// Vector Integer Halving Add (Signed)
// Vector Integer Halving Add (Unsigned)
// Vector Integer Halving Sub (Signed)
// Vector Integer Halving Sub (Unsigned)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types (2d)
        shadd v0.2d, v1.2d, v2.2d
        uhadd v4.2s, v5.2s, v5.4h
        shsub v11.4h, v12.8h, v13.4h
        uhsub v31.16b, v29.8b, v28.8b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        shadd v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uhadd v4.2s, v5.2s, v5.4h
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        shsub v11.4h, v12.8h, v13.4h
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uhsub v31.16b, v29.8b, v28.8b
// CHECK-ERROR:                          ^

//----------------------------------------------------------------------
// Vector Integer Rouding Halving Add (Signed)
// Vector Integer Rouding Halving Add (Unsigned)
//----------------------------------------------------------------------

        // Mismatched and invalid vector types (2d)
        srhadd v0.2s, v1.2s, v2.2d
        urhadd v0.16b, v1.16b, v2.8h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        srhadd v0.2s, v1.2s, v2.2d
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        urhadd v0.16b, v1.16b, v2.8h
// CHECK-ERROR:                                  ^

//----------------------------------------------------------------------
// Vector Integer Saturating Add (Signed)
// Vector Integer Saturating Add (Unsigned)
// Vector Integer Saturating Sub (Signed)
// Vector Integer Saturating Sub (Unsigned)
//----------------------------------------------------------------------

        // Mismatched vector types
        sqadd v0.2s, v1.2s, v2.2d
        uqadd v31.8h, v1.4h, v2.4h
        sqsub v10.8h, v1.16b, v2.16b
        uqsub v31.8b, v1.8b, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqadd v0.2s, v1.2s, v2.2d
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uqadd v31.8h, v1.4h, v2.4h
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqsub v10.8h, v1.16b, v2.16b
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uqsub v31.8b, v1.8b, v2.4s
// CHECK-ERROR:                                ^

//----------------------------------------------------------------------
// Scalar Integer Saturating Add (Signed)
// Scalar Integer Saturating Add (Unsigned)
// Scalar Integer Saturating Sub (Signed)
// Scalar Integer Saturating Sub (Unsigned)
//----------------------------------------------------------------------

      // Mismatched registers
         sqadd d0, s31, d2
         uqadd s0, s1, d2
         sqsub b0, b2, s18
         uqsub h1, h2, d2

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqadd d0, s31, d2
// CHECK-ERROR:                  ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uqadd s0, s1, d2
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqsub b0, b2, s18
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uqsub h1, h2, d2
// CHECK-ERROR:                      ^


//----------------------------------------------------------------------
// Vector Shift Left (Signed and Unsigned Integer)
//----------------------------------------------------------------------
        // Mismatched vector types
        sshl v0.4s, v15.2s, v16.2s
        ushl v1.16b, v25.16b, v6.8h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sshl v0.4s, v15.2s, v16.2s
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ushl v1.16b, v25.16b, v6.8h
// CHECK-ERROR:                                 ^

//----------------------------------------------------------------------
// Vector Saturating Shift Left (Signed and Unsigned Integer)
//----------------------------------------------------------------------
        // Mismatched vector types
        sqshl v0.2s, v15.4s, v16.2d
        uqshl v1.8b, v25.4h, v6.8h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqshl v0.2s, v15.4s, v16.2d 
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uqshl v1.8b, v25.4h, v6.8h
// CHECK-ERROR:                         ^

//----------------------------------------------------------------------
// Vector Rouding Shift Left (Signed and Unsigned Integer)
//----------------------------------------------------------------------
        // Mismatched vector types
        srshl v0.8h, v15.8h, v16.16b
        urshl v1.2d, v25.2d, v6.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        srshl v0.8h, v15.8h, v16.16b
// CHECK-ERROR:                                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        urshl v1.2d, v25.2d, v6.4s
// CHECK-ERROR:                                ^

//----------------------------------------------------------------------
// Vector Saturating Rouding Shift Left (Signed and Unsigned Integer)
//----------------------------------------------------------------------
        // Mismatched vector types
        sqrshl v0.2s, v15.8h, v16.16b
        uqrshl v1.4h, v25.4h,  v6.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqrshl v0.2s, v15.8h, v16.16b
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uqrshl v1.4h, v25.4h,  v6.2d
// CHECK-ERROR:                                  ^

//----------------------------------------------------------------------
// Scalar Integer Shift Left (Signed, Unsigned)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        sshl d0, d1, s2
        ushl b2, b0, b1

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sshl d0, d1, s2
// CHECK-ERROR:                     ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ushl b2, b0, b1
// CHECK-ERROR:             ^

//----------------------------------------------------------------------
// Scalar Integer Saturating Shift Left (Signed, Unsigned)
//----------------------------------------------------------------------

        // Mismatched vector types
        sqshl b0, b1, s0
        uqshl h0, h1, b0
        sqshl s0, s1, h0
        uqshl d0, d1, b0

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqshl b0, b1, s0
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uqshl h0, h1, b0
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqshl s0, s1, h0
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uqshl d0, d1, b0
// CHECK-ERROR:                      ^

//----------------------------------------------------------------------
// Scalar Integer Rouding Shift Left (Signed, Unsigned)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        srshl h0, h1, h2
        urshl s0, s1, s2

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        srshl h0, h1, h2
// CHECK-ERROR:              ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        urshl s0, s1, s2
// CHECK-ERROR:              ^


//----------------------------------------------------------------------
// Scalar Integer Saturating Rounding Shift Left (Signed, Unsigned)
//----------------------------------------------------------------------

        // Mismatched vector types
        sqrshl b0, b1, s0
        uqrshl h0, h1, b0
        sqrshl s0, s1, h0
        uqrshl d0, d1, b0

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqrshl b0, b1, s0
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uqrshl h0, h1, b0
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqrshl s0, s1, h0
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uqrshl d0, d1, b0
// CHECK-ERROR:                       ^


//----------------------------------------------------------------------
// Vector Maximum (Signed, Unsigned)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        smax v0.2d, v1.2d, v2.2d
        umax v0.4h, v1.4h, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smax v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umax v0.4h, v1.4h, v2.2s
// CHECK-ERROR:                              ^

//----------------------------------------------------------------------
// Vector Minimum (Signed, Unsigned)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        smin v0.2d, v1.2d, v2.2d
        umin v0.2s, v1.2s, v2.8b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smin v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umin v0.2s, v1.2s, v2.8b
// CHECK-ERROR:                             ^


//----------------------------------------------------------------------
// Vector Maximum (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        fmax v0.2s, v1.2s, v2.4s
        fmax v0.8b, v1.8b, v2.8b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fmax v0.2s, v1.2s, v2.4s
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fmax v0.8b, v1.8b, v2.8b
// CHECK-ERROR:                ^
//----------------------------------------------------------------------
// Vector Minimum (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        fmin v0.4s, v1.4s, v2.2d
        fmin v0.8h, v1.8h, v2.8h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fmin v0.4s, v1.4s, v2.2d
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fmin v0.8h, v1.8h, v2.8h
// CHECK-ERROR:                ^

//----------------------------------------------------------------------
// Vector maxNum (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        fmaxnm v0.2s, v1.2s, v2.2d
        fmaxnm v0.4h, v1.8h, v2.4h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fmaxnm v0.2s, v1.2s, v2.2d
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fmaxnm v0.4h, v1.8h, v2.4h
// CHECK-ERROR:                  ^

//----------------------------------------------------------------------
// Vector minNum (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        fminnm v0.4s, v1.2s, v2.4s
        fminnm v0.16b, v0.16b, v0.16b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fminnm v0.4s, v1.2s, v2.4s
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fminnm v0.16b, v0.16b, v0.16b
// CHECK-ERROR:                  ^


//----------------------------------------------------------------------
// Vector Maximum Pairwise (Signed, Unsigned)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        smaxp v0.2d, v1.2d, v2.2d
        umaxp v0.4h, v1.4h, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smaxp v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umaxp v0.4h, v1.4h, v2.2s
// CHECK-ERROR:                               ^

//----------------------------------------------------------------------
// Vector Minimum Pairwise (Signed, Unsigned)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        sminp v0.2d, v1.2d, v2.2d
        uminp v0.2s, v1.2s, v2.8b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sminp v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uminp v0.2s, v1.2s, v2.8b
// CHECK-ERROR:                               ^


//----------------------------------------------------------------------
// Vector Maximum Pairwise (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        fmaxp v0.2s, v1.2s, v2.4s
        fmaxp v0.8b, v1.8b, v2.8b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fmaxp v0.2s, v1.2s, v2.4s
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fmaxp v0.8b, v1.8b, v2.8b
// CHECK-ERROR:                 ^
//----------------------------------------------------------------------
// Vector Minimum Pairwise (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        fminp v0.4s, v1.4s, v2.2d
        fminp v0.8h, v1.8h, v2.8h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fminp v0.4s, v1.4s, v2.2d
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fminp v0.8h, v1.8h, v2.8h
// CHECK-ERROR:                 ^

//----------------------------------------------------------------------
// Vector maxNum Pairwise (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        fmaxnmp v0.2s, v1.2s, v2.2d
        fmaxnmp v0.4h, v1.8h, v2.4h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fmaxnmp v0.2s, v1.2s, v2.2d
// CHECK-ERROR:                                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fmaxnmp v0.4h, v1.8h, v2.4h
// CHECK-ERROR:                   ^

//----------------------------------------------------------------------
// Vector minNum Pairwise (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        fminnmp v0.4s, v1.2s, v2.4s
        fminnmp v0.16b, v0.16b, v0.16b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fminnmp v0.4s, v1.2s, v2.4s
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        fminnmp v0.16b, v0.16b, v0.16b
// CHECK-ERROR:                   ^


//----------------------------------------------------------------------
// Vector Add Pairwise (Integer)
//----------------------------------------------------------------------

        // Mismatched vector types
        addp v0.16b, v1.8b, v2.8b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         addp v0.16b, v1.8b, v2.8b
// CHECK-ERROR:                         ^

//----------------------------------------------------------------------
// Vector Add Pairwise (Floating Point)
//----------------------------------------------------------------------
        // Mismatched and invalid vector types
        faddp v0.16b, v1.8b, v2.8b
        faddp v0.2d, v1.2d, v2.8h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         faddp v0.16b, v1.8b, v2.8b
// CHECK-ERROR:                  ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         faddp v0.2d, v1.2d, v2.8h
// CHECK-ERROR:                                ^


//----------------------------------------------------------------------
// Vector Saturating Doubling Multiply High
//----------------------------------------------------------------------
         // Mismatched and invalid vector types
         sqdmulh v2.4h, v25.8h, v3.4h
         sqdmulh v12.2d, v5.2d, v13.2d
         sqdmulh v3.8b, v1.8b, v30.8b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqdmulh v2.4h, v25.8h, v3.4h
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqdmulh v12.2d, v5.2d, v13.2d
// CHECK-ERROR:                     ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqdmulh v3.8b, v1.8b, v30.8b
// CHECK-ERROR:                    ^

//----------------------------------------------------------------------
// Vector Saturating Rouding Doubling Multiply High
//----------------------------------------------------------------------
         // Mismatched and invalid vector types
         sqrdmulh v2.2s, v25.4s, v3.4s
         sqrdmulh v12.16b, v5.16b, v13.16b
         sqrdmulh v3.4h, v1.4h, v30.2d


// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqrdmulh v2.2s, v25.4s, v3.4s
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqrdmulh v12.16b, v5.16b, v13.16b
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqrdmulh v3.4h, v1.4h, v30.2d
// CHECK-ERROR:                                    ^

//----------------------------------------------------------------------
// Vector Multiply Extended
//----------------------------------------------------------------------
         // Mismatched and invalid vector types
      fmulx v21.2s, v5.2s, v13.2d
      fmulx v1.4h, v25.4h, v3.4h

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fmulx v21.2s, v5.2s, v13.2d
// CHECK-ERROR:                                  ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fmulx v1.4h, v25.4h, v3.4h
// CHECK-ERROR:                  ^

//------------------------------------------------------------------------------
// Vector Shift Left by Immediate
//------------------------------------------------------------------------------
         // Mismatched vector types and out of range
         shl v0.4s, v15,2s, #3
         shl v0.2d, v17.4s, #3
         shl v0.8b, v31.8b, #-1
         shl v0.8b, v31.8b, #8
         shl v0.4s, v21.4s, #32
         shl v0.2d, v1.2d, #64

// CHECK-ERROR: error: expected comma before next operand
// CHECK-ERROR:         shl v0.4s, v15,2s, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         shl v0.2d, v17.4s, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: expected integer in range [0, 7]
// CHECK-ERROR:         shl v0.8b, v31.8b, #-1
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [0, 7]
// CHECK-ERROR:         shl v0.8b, v31.8b, #8
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [0, 31]
// CHECK-ERROR:         shl v0.4s, v21.4s, #32
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [0, 63]
// CHECK-ERROR:         shl v0.2d, v1.2d, #64
// CHECK-ERROR:                           ^

//----------------------------------------------------------------------
// Vector Shift Left Long by Immediate
//----------------------------------------------------------------------
        // Mismatched vector types
        sshll v0.4s, v15.2s, #3
        ushll v1.16b, v25.16b, #6
        sshll2 v0.2d, v3.8s, #15
        ushll2 v1.4s, v25.4s, #7

        // Out of range 
        sshll v0.8h, v1.8b, #-1
        sshll v0.8h, v1.8b, #9
        ushll v0.4s, v1.4h, #17
        ushll v0.2d, v1.2s, #33
        sshll2 v0.8h, v1.16b, #9
        sshll2 v0.4s, v1.8h, #17
        ushll2 v0.2d, v1.4s, #33

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sshll v0.4s, v15.2s, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ushll v1.16b, v25.16b, #6
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sshll2 v0.2d, v3.8s, #15
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ushll2 v1.4s, v25.4s, #7
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: expected integer in range [0, 7]
// CHECK-ERROR:        sshll v0.8h, v1.8b, #-1
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [0, 7]
// CHECK-ERROR:        sshll v0.8h, v1.8b, #9
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [0, 15]
// CHECK-ERROR:        ushll v0.4s, v1.4h, #17
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [0, 31]
// CHECK-ERROR:        ushll v0.2d, v1.2s, #33
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [0, 7]
// CHECK-ERROR:        sshll2 v0.8h, v1.16b, #9
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: expected integer in range [0, 15]
// CHECK-ERROR:        sshll2 v0.4s, v1.8h, #17
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [0, 31]
// CHECK-ERROR:        ushll2 v0.2d, v1.4s, #33
// CHECK-ERROR:                             ^


//------------------------------------------------------------------------------
// Vector shift right by immediate
//------------------------------------------------------------------------------
         sshr v0.8b, v1.8h, #3
         sshr v0.4h, v1.4s, #3
         sshr v0.2s, v1.2d, #3
         sshr v0.16b, v1.16b, #9
         sshr v0.8h, v1.8h, #17
         sshr v0.4s, v1.4s, #33
         sshr v0.2d, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sshr v0.8b, v1.8h, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sshr v0.4h, v1.4s, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sshr v0.2s, v1.2d, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         sshr v0.16b, v1.16b, #9
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         sshr v0.8h, v1.8h, #17
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         sshr v0.4s, v1.4s, #33
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [1, 64]
// CHECK-ERROR:         sshr v0.2d, v1.2d, #65
// CHECK-ERROR:                            ^

//------------------------------------------------------------------------------
// Vector  shift right by immediate
//------------------------------------------------------------------------------
         ushr v0.8b, v1.8h, #3
         ushr v0.4h, v1.4s, #3
         ushr v0.2s, v1.2d, #3
         ushr v0.16b, v1.16b, #9
         ushr v0.8h, v1.8h, #17
         ushr v0.4s, v1.4s, #33
         ushr v0.2d, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         ushr v0.8b, v1.8h, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         ushr v0.4h, v1.4s, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         ushr v0.2s, v1.2d, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         ushr v0.16b, v1.16b, #9
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         ushr v0.8h, v1.8h, #17
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         ushr v0.4s, v1.4s, #33
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [1, 64]
// CHECK-ERROR:         ushr v0.2d, v1.2d, #65
// CHECK-ERROR:                            ^

//------------------------------------------------------------------------------
// Vector shift right and accumulate by immediate
//------------------------------------------------------------------------------
         ssra v0.8b, v1.8h, #3
         ssra v0.4h, v1.4s, #3
         ssra v0.2s, v1.2d, #3
         ssra v0.16b, v1.16b, #9
         ssra v0.8h, v1.8h, #17
         ssra v0.4s, v1.4s, #33
         ssra v0.2d, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         ssra v0.8b, v1.8h, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         ssra v0.4h, v1.4s, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         ssra v0.2s, v1.2d, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         ssra v0.16b, v1.16b, #9
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         ssra v0.8h, v1.8h, #17
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         ssra v0.4s, v1.4s, #33
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [1, 64]
// CHECK-ERROR:         ssra v0.2d, v1.2d, #65
// CHECK-ERROR:                            ^

//------------------------------------------------------------------------------
// Vector  shift right and accumulate by immediate
//------------------------------------------------------------------------------
         usra v0.8b, v1.8h, #3
         usra v0.4h, v1.4s, #3
         usra v0.2s, v1.2d, #3
         usra v0.16b, v1.16b, #9
         usra v0.8h, v1.8h, #17
         usra v0.4s, v1.4s, #33
         usra v0.2d, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         usra v0.8b, v1.8h, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         usra v0.4h, v1.4s, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         usra v0.2s, v1.2d, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         usra v0.16b, v1.16b, #9
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         usra v0.8h, v1.8h, #17
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         usra v0.4s, v1.4s, #33
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [1, 64]
// CHECK-ERROR:         usra v0.2d, v1.2d, #65
// CHECK-ERROR:                            ^

//------------------------------------------------------------------------------
// Vector rounding shift right by immediate
//------------------------------------------------------------------------------
         srshr v0.8b, v1.8h, #3
         srshr v0.4h, v1.4s, #3
         srshr v0.2s, v1.2d, #3
         srshr v0.16b, v1.16b, #9
         srshr v0.8h, v1.8h, #17
         srshr v0.4s, v1.4s, #33
         srshr v0.2d, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         srshr v0.8b, v1.8h, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         srshr v0.4h, v1.4s, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         srshr v0.2s, v1.2d, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         srshr v0.16b, v1.16b, #9
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         srshr v0.8h, v1.8h, #17
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         srshr v0.4s, v1.4s, #33
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 64]
// CHECK-ERROR:         srshr v0.2d, v1.2d, #65
// CHECK-ERROR:                             ^

//------------------------------------------------------------------------------
// Vecotr rounding shift right by immediate
//------------------------------------------------------------------------------
         urshr v0.8b, v1.8h, #3
         urshr v0.4h, v1.4s, #3
         urshr v0.2s, v1.2d, #3
         urshr v0.16b, v1.16b, #9
         urshr v0.8h, v1.8h, #17
         urshr v0.4s, v1.4s, #33
         urshr v0.2d, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         urshr v0.8b, v1.8h, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         urshr v0.4h, v1.4s, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         urshr v0.2s, v1.2d, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         urshr v0.16b, v1.16b, #9
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         urshr v0.8h, v1.8h, #17
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         urshr v0.4s, v1.4s, #33
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 64]
// CHECK-ERROR:         urshr v0.2d, v1.2d, #65
// CHECK-ERROR:                             ^

//------------------------------------------------------------------------------
// Vector rounding shift right and accumulate by immediate
//------------------------------------------------------------------------------
         srsra v0.8b, v1.8h, #3
         srsra v0.4h, v1.4s, #3
         srsra v0.2s, v1.2d, #3
         srsra v0.16b, v1.16b, #9
         srsra v0.8h, v1.8h, #17
         srsra v0.4s, v1.4s, #33
         srsra v0.2d, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         srsra v0.8b, v1.8h, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         srsra v0.4h, v1.4s, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         srsra v0.2s, v1.2d, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         srsra v0.16b, v1.16b, #9
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         srsra v0.8h, v1.8h, #17
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         srsra v0.4s, v1.4s, #33
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 64]
// CHECK-ERROR:         srsra v0.2d, v1.2d, #65
// CHECK-ERROR:                             ^

//------------------------------------------------------------------------------
// Vector rounding shift right and accumulate by immediate
//------------------------------------------------------------------------------
         ursra v0.8b, v1.8h, #3
         ursra v0.4h, v1.4s, #3
         ursra v0.2s, v1.2d, #3
         ursra v0.16b, v1.16b, #9
         ursra v0.8h, v1.8h, #17
         ursra v0.4s, v1.4s, #33
         ursra v0.2d, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         ursra v0.8b, v1.8h, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         ursra v0.4h, v1.4s, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         ursra v0.2s, v1.2d, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         ursra v0.16b, v1.16b, #9
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         ursra v0.8h, v1.8h, #17
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         ursra v0.4s, v1.4s, #33
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 64]
// CHECK-ERROR:         ursra v0.2d, v1.2d, #65
// CHECK-ERROR:                             ^

//------------------------------------------------------------------------------
// Vector shift right and insert by immediate
//------------------------------------------------------------------------------
         sri v0.8b, v1.8h, #3
         sri v0.4h, v1.4s, #3
         sri v0.2s, v1.2d, #3
         sri v0.16b, v1.16b, #9
         sri v0.8h, v1.8h, #17
         sri v0.4s, v1.4s, #33
         sri v0.2d, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sri v0.8b, v1.8h, #3
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sri v0.4h, v1.4s, #3
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sri v0.2s, v1.2d, #3
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         sri v0.16b, v1.16b, #9
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         sri v0.8h, v1.8h, #17
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         sri v0.4s, v1.4s, #33
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: expected integer in range [1, 64]
// CHECK-ERROR:         sri v0.2d, v1.2d, #65
// CHECK-ERROR:                           ^

//------------------------------------------------------------------------------
// Vector shift left and insert by immediate
//------------------------------------------------------------------------------
         sli v0.8b, v1.8h, #3
         sli v0.4h, v1.4s, #3
         sli v0.2s, v1.2d, #3
         sli v0.16b, v1.16b, #8
         sli v0.8h, v1.8h, #16
         sli v0.4s, v1.4s, #32
         sli v0.2d, v1.2d, #64

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sli v0.8b, v1.8h, #3
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sli v0.4h, v1.4s, #3
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sli v0.2s, v1.2d, #3
// CHECK-ERROR:                       ^
// CHECK-ERROR: error: expected integer in range [0, 7]
// CHECK-ERROR:         sli v0.16b, v1.16b, #8
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [0, 15]
// CHECK-ERROR:         sli v0.8h, v1.8h, #16
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: expected integer in range [0, 31]
// CHECK-ERROR:         sli v0.4s, v1.4s, #32
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: expected integer in range [0, 63]
// CHECK-ERROR:         sli v0.2d, v1.2d, #64
// CHECK-ERROR:                           ^

//------------------------------------------------------------------------------
// Vector saturating shift left unsigned by immediate
//------------------------------------------------------------------------------
         sqshlu v0.8b, v1.8h, #3
         sqshlu v0.4h, v1.4s, #3
         sqshlu v0.2s, v1.2d, #3
         sqshlu v0.16b, v1.16b, #8
         sqshlu v0.8h, v1.8h, #16
         sqshlu v0.4s, v1.4s, #32
         sqshlu v0.2d, v1.2d, #64

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshlu v0.8b, v1.8h, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshlu v0.4h, v1.4s, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshlu v0.2s, v1.2d, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: expected integer in range [0, 7]
// CHECK-ERROR:         sqshlu v0.16b, v1.16b, #8
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: expected integer in range [0, 15]
// CHECK-ERROR:         sqshlu v0.8h, v1.8h, #16
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: expected integer in range [0, 31]
// CHECK-ERROR:         sqshlu v0.4s, v1.4s, #32
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: expected integer in range [0, 63]
// CHECK-ERROR:         sqshlu v0.2d, v1.2d, #64
// CHECK-ERROR:                              ^

//------------------------------------------------------------------------------
// Vector saturating shift left by immediate
//------------------------------------------------------------------------------
         sqshl v0.8b, v1.8h, #3
         sqshl v0.4h, v1.4s, #3
         sqshl v0.2s, v1.2d, #3
         sqshl v0.16b, v1.16b, #8
         sqshl v0.8h, v1.8h, #16
         sqshl v0.4s, v1.4s, #32
         sqshl v0.2d, v1.2d, #64

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshl v0.8b, v1.8h, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshl v0.4h, v1.4s, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshl v0.2s, v1.2d, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: expected integer in range [0, 7]
// CHECK-ERROR:         sqshl v0.16b, v1.16b, #8
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: expected integer in range [0, 15]
// CHECK-ERROR:         sqshl v0.8h, v1.8h, #16
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [0, 31]
// CHECK-ERROR:         sqshl v0.4s, v1.4s, #32
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [0, 63]
// CHECK-ERROR:         sqshl v0.2d, v1.2d, #64
// CHECK-ERROR:                             ^

//------------------------------------------------------------------------------
// Vector saturating shift left by immediate
//------------------------------------------------------------------------------
         uqshl v0.8b, v1.8h, #3
         uqshl v0.4h, v1.4s, #3
         uqshl v0.2s, v1.2d, #3
         uqshl v0.16b, v1.16b, #8
         uqshl v0.8h, v1.8h, #16
         uqshl v0.4s, v1.4s, #32
         uqshl v0.2d, v1.2d, #64

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uqshl v0.8b, v1.8h, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uqshl v0.4h, v1.4s, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uqshl v0.2s, v1.2d, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: expected integer in range [0, 7]
// CHECK-ERROR:         uqshl v0.16b, v1.16b, #8
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: expected integer in range [0, 15]
// CHECK-ERROR:         uqshl v0.8h, v1.8h, #16
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [0, 31]
// CHECK-ERROR:         uqshl v0.4s, v1.4s, #32
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [0, 63]
// CHECK-ERROR:         uqshl v0.2d, v1.2d, #64
// CHECK-ERROR:                             ^

//------------------------------------------------------------------------------
// Vector shift right narrow by immediate
//------------------------------------------------------------------------------
         shrn v0.8b, v1.8b, #3
         shrn v0.4h, v1.4h, #3
         shrn v0.2s, v1.2s, #3
         shrn2 v0.16b, v1.8h, #17
         shrn2 v0.8h, v1.4s, #33
         shrn2 v0.4s, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         shrn v0.8b, v1.8b, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         shrn v0.4h, v1.4h, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         shrn v0.2s, v1.2s, #3
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         shrn2 v0.16b, v1.8h, #17
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         shrn2 v0.8h, v1.4s, #33
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         shrn2 v0.4s, v1.2d, #65
// CHECK-ERROR:                             ^

//------------------------------------------------------------------------------
// Vector saturating shift right unsigned narrow by immediate
//------------------------------------------------------------------------------
         sqshrun v0.8b, v1.8b, #3
         sqshrun v0.4h, v1.4h, #3
         sqshrun v0.2s, v1.2s, #3
         sqshrun2 v0.16b, v1.8h, #17
         sqshrun2 v0.8h, v1.4s, #33
         sqshrun2 v0.4s, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshrun v0.8b, v1.8b, #3
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshrun v0.4h, v1.4h, #3
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshrun v0.2s, v1.2s, #3
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         sqshrun2 v0.16b, v1.8h, #17
// CHECK-ERROR:                                 ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         sqshrun2 v0.8h, v1.4s, #33
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         sqshrun2 v0.4s, v1.2d, #65
// CHECK-ERROR:                                ^

//------------------------------------------------------------------------------
// Vector rounding shift right narrow by immediate
//------------------------------------------------------------------------------
         rshrn v0.8b, v1.8b, #3
         rshrn v0.4h, v1.4h, #3
         rshrn v0.2s, v1.2s, #3
         rshrn2 v0.16b, v1.8h, #17
         rshrn2 v0.8h, v1.4s, #33
         rshrn2 v0.4s, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         rshrn v0.8b, v1.8b, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         rshrn v0.4h, v1.4h, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         rshrn v0.2s, v1.2s, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         rshrn2 v0.16b, v1.8h, #17
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         rshrn2 v0.8h, v1.4s, #33
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         rshrn2 v0.4s, v1.2d, #65
// CHECK-ERROR:                              ^

//------------------------------------------------------------------------------
// Vector saturating shift right rounded unsigned narrow by immediate
//------------------------------------------------------------------------------
         sqrshrun v0.8b, v1.8b, #3
         sqrshrun v0.4h, v1.4h, #3
         sqrshrun v0.2s, v1.2s, #3
         sqrshrun2 v0.16b, v1.8h, #17
         sqrshrun2 v0.8h, v1.4s, #33
         sqrshrun2 v0.4s, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqrshrun v0.8b, v1.8b, #3
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqrshrun v0.4h, v1.4h, #3
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqrshrun v0.2s, v1.2s, #3
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         sqrshrun2 v0.16b, v1.8h, #17
// CHECK-ERROR:                                  ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         sqrshrun2 v0.8h, v1.4s, #33
// CHECK-ERROR:                                 ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         sqrshrun2 v0.4s, v1.2d, #65
// CHECK-ERROR:                                 ^

//------------------------------------------------------------------------------
// Vector saturating shift right narrow by immediate
//------------------------------------------------------------------------------
         sqshrn v0.8b, v1.8b, #3
         sqshrn v0.4h, v1.4h, #3
         sqshrn v0.2s, v1.2s, #3
         sqshrn2 v0.16b, v1.8h, #17
         sqshrn2 v0.8h, v1.4s, #33
         sqshrn2 v0.4s, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshrn v0.8b, v1.8b, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshrn v0.4h, v1.4h, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqshrn v0.2s, v1.2s, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         sqshrn2 v0.16b, v1.8h, #17
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         sqshrn2 v0.8h, v1.4s, #33
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         sqshrn2 v0.4s, v1.2d, #65
// CHECK-ERROR:                               ^

//------------------------------------------------------------------------------
// Vector saturating shift right narrow by immediate
//------------------------------------------------------------------------------
         uqshrn v0.8b, v1.8b, #3
         uqshrn v0.4h, v1.4h, #3
         uqshrn v0.2s, v1.2s, #3
         uqshrn2 v0.16b, v1.8h, #17
         uqshrn2 v0.8h, v1.4s, #33
         uqshrn2 v0.4s, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uqshrn v0.8b, v1.8b, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uqshrn v0.4h, v1.4h, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uqshrn v0.2s, v1.2s, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         uqshrn2 v0.16b, v1.8h, #17
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         uqshrn2 v0.8h, v1.4s, #33
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         uqshrn2 v0.4s, v1.2d, #65
// CHECK-ERROR:                               ^

//------------------------------------------------------------------------------
// Vector saturating shift right rounded narrow by immediate
//------------------------------------------------------------------------------
         sqrshrn v0.8b, v1.8b, #3
         sqrshrn v0.4h, v1.4h, #3
         sqrshrn v0.2s, v1.2s, #3
         sqrshrn2 v0.16b, v1.8h, #17
         sqrshrn2 v0.8h, v1.4s, #33
         sqrshrn2 v0.4s, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqrshrn v0.8b, v1.8b, #3
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqrshrn v0.4h, v1.4h, #3
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         sqrshrn v0.2s, v1.2s, #3
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         sqrshrn2 v0.16b, v1.8h, #17
// CHECK-ERROR:                                 ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         sqrshrn2 v0.8h, v1.4s, #33
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         sqrshrn2 v0.4s, v1.2d, #65
// CHECK-ERROR:                                ^

//------------------------------------------------------------------------------
// Vector saturating shift right rounded narrow by immediate
//------------------------------------------------------------------------------
         uqrshrn v0.8b, v1.8b, #3
         uqrshrn v0.4h, v1.4h, #3
         uqrshrn v0.2s, v1.2s, #3
         uqrshrn2 v0.16b, v1.8h, #17
         uqrshrn2 v0.8h, v1.4s, #33
         uqrshrn2 v0.4s, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uqrshrn v0.8b, v1.8b, #3
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uqrshrn v0.4h, v1.4h, #3
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         uqrshrn v0.2s, v1.2s, #3
// CHECK-ERROR:                           ^
// CHECK-ERROR: error: expected integer in range [1, 8]
// CHECK-ERROR:         uqrshrn2 v0.16b, v1.8h, #17
// CHECK-ERROR:                                 ^
// CHECK-ERROR: error: expected integer in range [1, 16]
// CHECK-ERROR:         uqrshrn2 v0.8h, v1.4s, #33
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         uqrshrn2 v0.4s, v1.2d, #65
// CHECK-ERROR:                                ^

//------------------------------------------------------------------------------
// Fixed-point convert to floating-point
//------------------------------------------------------------------------------
         scvtf v0.2s, v1.2d, #3
         scvtf v0.4s, v1.4h, #3
         scvtf v0.2d, v1.2s, #3
         ucvtf v0.2s, v1.2s, #33
         ucvtf v0.4s, v1.4s, #33
         ucvtf v0.2d, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         scvtf v0.2s, v1.2d, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         scvtf v0.4s, v1.4h, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         scvtf v0.2d, v1.2s, #3
// CHECK-ERROR:                         ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         ucvtf v0.2s, v1.2s, #33
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         ucvtf v0.4s, v1.4s, #33
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: expected integer in range [1, 64]
// CHECK-ERROR:         ucvtf v0.2d, v1.2d, #65
// CHECK-ERROR:                             ^

//------------------------------------------------------------------------------
// Floating-point convert to fixed-point
//------------------------------------------------------------------------------
         fcvtzs v0.2s, v1.2d, #3
         fcvtzs v0.4s, v1.4h, #3
         fcvtzs v0.2d, v1.2s, #3
         fcvtzu v0.2s, v1.2s, #33
         fcvtzu v0.4s, v1.4s, #33
         fcvtzu v0.2d, v1.2d, #65

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fcvtzs v0.2s, v1.2d, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fcvtzs v0.4s, v1.4h, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:         fcvtzs v0.2d, v1.2s, #3
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         fcvtzu v0.2s, v1.2s, #33
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: expected integer in range [1, 32]
// CHECK-ERROR:         fcvtzu v0.4s, v1.4s, #33
// CHECK-ERROR:                              ^
// CHECK-ERROR: error: expected integer in range [1, 64]
// CHECK-ERROR:         fcvtzu v0.2d, v1.2d, #65
// CHECK-ERROR:                              ^

//----------------------------------------------------------------------
// Vector operation on 3 operands with different types
//----------------------------------------------------------------------

        // Mismatched and invalid vector types
        saddl v0.8h, v1.8h, v2.8b
        saddl v0.4s, v1.4s, v2.4h
        saddl v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddl v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddl v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddl v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        saddl2 v0.4s, v1.8s, v2.8h
        saddl2 v0.8h, v1.16h, v2.16b
        saddl2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddl2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddl2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddl2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        uaddl v0.8h, v1.8h, v2.8b
        uaddl v0.4s, v1.4s, v2.4h
        uaddl v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddl v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddl v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddl v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        uaddl2 v0.8h, v1.16h, v2.16b
        uaddl2 v0.4s, v1.8s, v2.8h
        uaddl2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddl2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddl2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddl2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        ssubl v0.8h, v1.8h, v2.8b
        ssubl v0.4s, v1.4s, v2.4h
        ssubl v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubl v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubl v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubl v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        ssubl2 v0.8h, v1.16h, v2.16b
        ssubl2 v0.4s, v1.8s, v2.8h
        ssubl2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubl2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubl2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubl2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        usubl v0.8h, v1.8h, v2.8b
        usubl v0.4s, v1.4s, v2.4h
        usubl v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubl v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubl v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubl v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        usubl2 v0.8h, v1.16h, v2.16b
        usubl2 v0.4s, v1.8s, v2.8h
        usubl2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubl2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubl2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubl2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        sabal v0.8h, v1.8h, v2.8b
        sabal v0.4s, v1.4s, v2.4h
        sabal v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabal v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabal v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabal v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        sabal2 v0.8h, v1.16h, v2.16b
        sabal2 v0.4s, v1.8s, v2.8h
        sabal2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabal2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabal2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabal2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        uabal v0.8h, v1.8h, v2.8b
        uabal v0.4s, v1.4s, v2.4h
        uabal v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabal v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabal v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabal v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        uabal2 v0.8h, v1.16h, v2.16b
        uabal2 v0.4s, v1.8s, v2.8h
        uabal2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabal2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabal2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabal2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        sabdl v0.8h, v1.8h, v2.8b
        sabdl v0.4s, v1.4s, v2.4h
        sabdl v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabdl v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabdl v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabdl v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        sabdl2 v0.8h, v1.16h, v2.16b
        sabdl2 v0.4s, v1.8s, v2.8h
        sabdl2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabdl2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabdl2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sabdl2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        uabdl v0.8h, v1.8h, v2.8b
        uabdl v0.4s, v1.4s, v2.4h
        uabdl v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabdl v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabdl v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabdl v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        uabdl2 v0.8h, v1.16h, v2.16b
        uabdl2 v0.4s, v1.8s, v2.8h
        uabdl2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabdl2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabdl2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uabdl2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        smlal v0.8h, v1.8h, v2.8b
        smlal v0.4s, v1.4s, v2.4h
        smlal v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlal v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlal v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlal v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        smlal2 v0.8h, v1.16h, v2.16b
        smlal2 v0.4s, v1.8s, v2.8h
        smlal2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlal2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlal2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlal2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        umlal v0.8h, v1.8h, v2.8b
        umlal v0.4s, v1.4s, v2.4h
        umlal v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlal v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlal v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlal v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        umlal2 v0.8h, v1.16h, v2.16b
        umlal2 v0.4s, v1.8s, v2.8h
        umlal2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlal2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlal2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlal2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        smlsl v0.8h, v1.8h, v2.8b
        smlsl v0.4s, v1.4s, v2.4h
        smlsl v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlsl v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlsl v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlsl v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        smlsl2 v0.8h, v1.16h, v2.16b
        smlsl2 v0.4s, v1.8s, v2.8h
        smlsl2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlsl2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlsl2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smlsl2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        umlsl v0.8h, v1.8h, v2.8b
        umlsl v0.4s, v1.4s, v2.4h
        umlsl v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlsl v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlsl v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlsl v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        umlsl2 v0.8h, v1.16h, v2.16b
        umlsl2 v0.4s, v1.8s, v2.8h
        umlsl2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlsl2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlsl2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umlsl2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        smull v0.8h, v1.8h, v2.8b
        smull v0.4s, v1.4s, v2.4h
        smull v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smull v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smull v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smull v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        smull2 v0.8h, v1.16h, v2.16b
        smull2 v0.4s, v1.8s, v2.8h
        smull2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smull2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smull2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        smull2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

        umull v0.8h, v1.8h, v2.8b
        umull v0.4s, v1.4s, v2.4h
        umull v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umull v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umull v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umull v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                        ^

        umull2 v0.8h, v1.16h, v2.16b
        umull2 v0.4s, v1.8s, v2.8h
        umull2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umull2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umull2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                      ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        umull2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                      ^

//------------------------------------------------------------------------------
// Long - Variant 2
//------------------------------------------------------------------------------

        sqdmlal v0.4s, v1.4s, v2.4h
        sqdmlal v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlal v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlal v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                          ^

        sqdmlal2 v0.4s, v1.8s, v2.8h
        sqdmlal2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlal2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlal2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                        ^

        // Mismatched vector types
        sqdmlal v0.8h, v1.8b, v2.8b
        sqdmlal2 v0.8h, v1.16b, v2.16b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlal v0.8h, v1.8b, v2.8b
// CHECK-ERROR:                   ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlal2 v0.8h, v1.16b, v2.16b
// CHECK-ERROR:                    ^

        sqdmlsl v0.4s, v1.4s, v2.4h
        sqdmlsl v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlsl v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlsl v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                          ^

        sqdmlsl2 v0.4s, v1.8s, v2.8h
        sqdmlsl2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlsl2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlsl2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                        ^

        // Mismatched vector types
        sqdmlsl v0.8h, v1.8b, v2.8b
        sqdmlsl2 v0.8h, v1.16b, v2.16b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlsl v0.8h, v1.8b, v2.8b
// CHECK-ERROR:                   ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmlsl2 v0.8h, v1.16b, v2.16b
// CHECK-ERROR:                    ^


        sqdmull v0.4s, v1.4s, v2.4h
        sqdmull v0.2d, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmull v0.4s, v1.4s, v2.4h
// CHECK-ERROR:                          ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmull v0.2d, v1.2d, v2.2s
// CHECK-ERROR:                          ^

        sqdmull2 v0.4s, v1.8s, v2.8h
        sqdmull2 v0.2d, v1.4d, v2.4s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmull2 v0.4s, v1.8s, v2.8h
// CHECK-ERROR:                        ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmull2 v0.2d, v1.4d, v2.4s
// CHECK-ERROR:                        ^

        // Mismatched vector types
        sqdmull v0.8h, v1.8b, v2.8b
        sqdmull2 v0.8h, v1.16b, v2.16b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmull v0.8h, v1.8b, v2.8b
// CHECK-ERROR:                   ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        sqdmull2 v0.8h, v1.16b, v2.16b
// CHECK-ERROR:                    ^


//------------------------------------------------------------------------------
// Long - Variant 3
//------------------------------------------------------------------------------

        pmull v0.8h, v1.8h, v2.8b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        pmull v0.8h, v1.8h, v2.8b
// CHECK-ERROR:                        ^

        // Mismatched vector types
        pmull v0.4s, v1.4h, v2.4h
        pmull v0.2d, v1.2s, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        pmull v0.4s, v1.4h, v2.4h
// CHECK-ERROR:                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        pmull v0.2d, v1.2s, v2.2s
// CHECK-ERROR:                 ^


        pmull2 v0.8h, v1.16h, v2.16b

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        pmull2 v0.8h, v1.16h, v2.16b
// CHECK-ERROR:                      ^

        // Mismatched vector types
        pmull2 v0.4s, v1.8h v2.8h
        pmull2 v0.2d, v1.4s, v2.4s

// CHECK-ERROR: error: expected comma before next operand
// CHECK-ERROR:        pmull2 v0.4s, v1.8h v2.8h
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        pmull2 v0.2d, v1.4s, v2.4s
// CHECK-ERROR:                  ^

//------------------------------------------------------------------------------
// Widen
//------------------------------------------------------------------------------

        saddw v0.8h, v1.8h, v2.8h
        saddw v0.4s, v1.4s, v2.4s
        saddw v0.2d, v1.2d, v2.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddw v0.8h, v1.8h, v2.8h
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddw v0.4s, v1.4s, v2.4s
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddw v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                               ^

        saddw2 v0.8h, v1.8h, v2.16h
        saddw2 v0.4s, v1.4s, v2.8s
        saddw2 v0.2d, v1.2d, v2.4d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddw2 v0.8h, v1.8h, v2.16h
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddw2 v0.4s, v1.4s, v2.8s
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        saddw2 v0.2d, v1.2d, v2.4d
// CHECK-ERROR:                             ^

        uaddw v0.8h, v1.8h, v2.8h
        uaddw v0.4s, v1.4s, v2.4s
        uaddw v0.2d, v1.2d, v2.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddw v0.8h, v1.8h, v2.8h
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddw v0.4s, v1.4s, v2.4s
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddw v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                               ^

        uaddw2 v0.8h, v1.8h, v2.16h
        uaddw2 v0.4s, v1.4s, v2.8s
        uaddw2 v0.2d, v1.2d, v2.4d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddw2 v0.8h, v1.8h, v2.16h
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddw2 v0.4s, v1.4s, v2.8s
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        uaddw2 v0.2d, v1.2d, v2.4d
// CHECK-ERROR:                             ^

        ssubw v0.8h, v1.8h, v2.8h
        ssubw v0.4s, v1.4s, v2.4s
        ssubw v0.2d, v1.2d, v2.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubw v0.8h, v1.8h, v2.8h
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubw v0.4s, v1.4s, v2.4s
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubw v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                               ^

        ssubw2 v0.8h, v1.8h, v2.16h
        ssubw2 v0.4s, v1.4s, v2.8s
        ssubw2 v0.2d, v1.2d, v2.4d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubw2 v0.8h, v1.8h, v2.16h
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubw2 v0.4s, v1.4s, v2.8s
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        ssubw2 v0.2d, v1.2d, v2.4d
// CHECK-ERROR:                             ^

        usubw v0.8h, v1.8h, v2.8h
        usubw v0.4s, v1.4s, v2.4s
        usubw v0.2d, v1.2d, v2.2d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubw v0.8h, v1.8h, v2.8h
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubw v0.4s, v1.4s, v2.4s
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubw v0.2d, v1.2d, v2.2d
// CHECK-ERROR:                               ^

        usubw2 v0.8h, v1.8h, v2.16h
        usubw2 v0.4s, v1.4s, v2.8s
        usubw2 v0.2d, v1.2d, v2.4d

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubw2 v0.8h, v1.8h, v2.16h
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubw2 v0.4s, v1.4s, v2.8s
// CHECK-ERROR:                             ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        usubw2 v0.2d, v1.2d, v2.4d
// CHECK-ERROR:                             ^

//------------------------------------------------------------------------------
// Narrow
//------------------------------------------------------------------------------

        addhn v0.8b, v1.8h, v2.8d
        addhn v0.4h, v1.4s, v2.4h
        addhn v0.2s, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        addhn v0.8b, v1.8h, v2.8d
// CHECK-ERROR:                            ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        addhn v0.4h, v1.4s, v2.4h
// CHECK-ERROR:                               ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        addhn v0.2s, v1.2d, v2.2s
// CHECK-ERROR:                               ^

        addhn2 v0.16b, v1.8h, v2.8b
        addhn2 v0.8h, v1.4s, v2.4h
        addhn2 v0.4s, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        addhn2 v0.16b, v1.8h, v2.8b
// CHECK-ERROR:                                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        addhn2 v0.8h, v1.4s, v2.4h
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        addhn2 v0.4s, v1.2d, v2.2s
// CHECK-ERROR:                                ^

        raddhn v0.8b, v1.8h, v2.8b
        raddhn v0.4h, v1.4s, v2.4h
        raddhn v0.2s, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        raddhn v0.8b, v1.8h, v2.8b
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        raddhn v0.4h, v1.4s, v2.4h
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        raddhn v0.2s, v1.2d, v2.2s
// CHECK-ERROR:                                ^

        raddhn2 v0.16b, v1.8h, v2.8b
        raddhn2 v0.8h, v1.4s, v2.4h
        raddhn2 v0.4s, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        raddhn2 v0.16b, v1.8h, v2.8b
// CHECK-ERROR:                                  ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        raddhn2 v0.8h, v1.4s, v2.4h
// CHECK-ERROR:                                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        raddhn2 v0.4s, v1.2d, v2.2s
// CHECK-ERROR:                                 ^

        rsubhn v0.8b, v1.8h, v2.8b
        rsubhn v0.4h, v1.4s, v2.4h
        rsubhn v0.2s, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        rsubhn v0.8b, v1.8h, v2.8b
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        rsubhn v0.4h, v1.4s, v2.4h
// CHECK-ERROR:                                ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        rsubhn v0.2s, v1.2d, v2.2s
// CHECK-ERROR:                                ^

        rsubhn2 v0.16b, v1.8h, v2.8b
        rsubhn2 v0.8h, v1.4s, v2.4h
        rsubhn2 v0.4s, v1.2d, v2.2s

// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        rsubhn2 v0.16b, v1.8h, v2.8b
// CHECK-ERROR:                                  ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        rsubhn2 v0.8h, v1.4s, v2.4h
// CHECK-ERROR:                                 ^
// CHECK-ERROR: error: invalid operand for instruction
// CHECK-ERROR:        rsubhn2 v0.4s, v1.2d, v2.2s
// CHECK-ERROR:                                 ^

