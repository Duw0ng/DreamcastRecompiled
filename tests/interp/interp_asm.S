/*
 * Hand-written SH-4 probes for the interpreter fallback (assembled by
 * tools/interp/build_interp_fixtures.py with the SH-4 GNU assembler).
 * Image base 0x8C040000 (after the corpus image and its data pages);
 * exception vectors at VBR = 0x8C050000.
 * Every probe is a normal function: r4 = scratch buffer, returns in r0.
 */
        .little
        .text

/* ------------------------------------------------------------------ */
/* Delayed-branch semantics.                                          */
        .global probe_delay
        .align 2
probe_delay:
        mov     #0, r0
        sett
        bt/s    1f              /* T sampled before the slot ...           */
        clrt                    /* ... even though the slot clears it      */
        mov     #99, r0         /* must be skipped                         */
1:      add     #1, r0          /* r0 = 1                                  */
        clrt
        bf/s    2f
        sett                    /* slot sets T, branch still taken         */
        mov     #99, r0
2:      movt    r1              /* T=1 from the slot                       */
        add     r1, r0          /* r0 = 2                                  */
        bra     3f
        add     #10, r0         /* slot executes: r0 = 12                  */
        mov     #99, r0
3:      mova    4f, r0          /* r0 = &4f (PC-relative, aligned)         */
        mov.l   @r0, r1         /* r1 = 0x1234                             */
        mov     #12, r0
        add     r1, r0          /* r0 = 0x1240                             */
        sts.l   pr, @-r15
        bsr     5f
        mov     r0, r2          /* slot: r2 = 0x1240 before the call       */
        lds.l   @r15+, pr
        rts
        add     r2, r0          /* slot of RTS: r0 = 0x1240 + 0x2480       */
        .align 2
4:      .long   0x1234
5:      mov     r2, r0
        rts
        shll    r0              /* r0 = 0x2480                             */

/* ------------------------------------------------------------------ */
/* GBR addressing forms (not decoded by the static recompiler).       */
        .global probe_gbr
        .align 2
probe_gbr:
        ldc     r4, gbr
        mov.l   .Lgbr_val, r0
        mov.l   r0, @(8, gbr)           /* buf[8..11]  = 0x89ABCDEF */
        mov     #-2, r0
        mov.w   r0, @(4, gbr)           /* buf[4..5]   = 0xFFFE     */
        mov     #0x5A, r0
        mov.b   r0, @(1, gbr)           /* buf[1]      = 0x5A       */
        mov     #1, r0
        or.b    #0x81, @(r0, gbr)       /* buf[1] = 0xDB            */
        and.b   #0xF3, @(r0, gbr)       /* buf[1] = 0xD3            */
        xor.b   #0x0F, @(r0, gbr)       /* buf[1] = 0xDC            */
        tst.b   #0x20, @(r0, gbr)       /* bit5 clear -> T = 1      */
        movt    r3
        mov.b   @(1, gbr), r0           /* sign-extended 0xFFFFFFDC */
        mov     r0, r1
        mov.w   @(4, gbr), r0           /* 0xFFFFFFFE               */
        add     r0, r1
        mov.l   @(8, gbr), r0
        add     r1, r0
        rts
        add     r3, r0                  /* 0x89ABCDEF - 0x24 - 2 + 1 */
        .align 2
.Lgbr_val:
        .long   0x89ABCDEF

/* ------------------------------------------------------------------ */
/* MAC.W / MAC.L, with and without S-bit saturation.                  */
/* r4 -> words: [0]=a0 [1]=a1 (16-bit pairs at +0), longs at +16.     */
/* Results stored at r4+32: MACH/MACL for four cases.                 */
        .global probe_mac
        .align 2
probe_mac:
        mov     r4, r6
        add     #32, r6                 /* r6 = result area */
        /* MAC.W, S=0: sum of two 16x16 products (signed) */
        clrmac
        clrs
        mov     r4, r1
        mov     r4, r2
        add     #4, r2
        mac.w   @r1+, @r2+
        mac.w   @r1+, @r2+
        sts     mach, r0
        mov.l   r0, @r6
        sts     macl, r0
        mov.l   r0, @(4, r6)
        /* MAC.L, S=0 */
        clrmac
        mov     r4, r1
        add     #16, r1
        mov     r1, r2
        add     #8, r2
        mac.l   @r1+, @r2+
        mac.l   @r1+, @r2+
        sts     mach, r0
        mov.l   r0, @(8, r6)
        sts     macl, r0
        mov.l   r0, @(12, r6)
        /* MAC.L, S=1: saturates to 48 bits */
        clrmac
        sets
        mov     r4, r1
        add     #16, r1
        mov     r1, r2
        add     #8, r2
        mac.l   @r1+, @r2+
        mac.l   @r1+, @r2+
        sts     mach, r0
        mov.l   r0, @(16, r6)
        sts     macl, r0
        mov.l   r0, @(20, r6)
        /* MAC.W, S=1: 32-bit saturation on MACL */
        clrmac
        mov.l   .Lmacl_seed, r0
        lds     r0, macl
        mov     r4, r1
        mov     r4, r2
        add     #4, r2
        mac.w   @r1+, @r2+
        sts     mach, r0
        mov.l   r0, @(24, r6)
        sts     macl, r0
        mov.l   r0, @(28, r6)
        clrs
        rts
        mov     #77, r0
        .align 2
.Lmacl_seed:
        .long   0x7FFFFF00

/* ------------------------------------------------------------------ */
/* Interrupt taken in the middle of interpreted code.                 */
/* The caller leaves a HOLLY bank-B event pending but masked; this    */
/* probe unmasks it halfway through and must still finish correctly. */
        .global probe_irq
        .align 2
probe_irq:
        mov     #0, r1
        mov     #50, r2
1:      add     #1, r1
        dt      r2
        bf      1b
        mov.l   .Lirq_mask, r3
        mov     #1, r0
        mov.l   r0, @r3                 /* unmask -> IRQ at next block tick */
        mov     #100, r2
2:      add     #1, r1
        dt      r2
        bf      2b
        rts
        mov     r1, r0                  /* 150 */
        .align 2
.Lirq_mask:
        .long   0xA05F6934              /* IRQ9 (priority 6) mask, bank B */

/* SLEEP waits for the pending interrupt, then continues after it. */
        .global probe_sleep
        .align 2
probe_sleep:
        mov.l   .Lirq_mask2, r3
        mov     #1, r0
        mov.l   r0, @r3
        sleep
        mov     #42, r0
        rts
        nop
        .align 2
.Lirq_mask2:
        .long   0xA05F6934

/* TRAPA enters VBR+0x100, the handler records TRA, RTE resumes after. */
        .global probe_trapa
        .align 2
probe_trapa:
        mov     #5, r0
        trapa   #0x21
        add     #3, r0                  /* runs after RTE: 8 */
        rts
        nop

/* ------------------------------------------------------------------ */
/* Exception vectors.                                                 */
        .org    0x10100                 /* VBR + 0x100: general exception */
vector_general:
        mov.l   .Ltra, r1
        mov.l   @r1, r1                 /* TRA = imm << 2 */
        mov.l   .Lcounter, r2
        mov.l   r1, @(4, r2)
        rte
        nop
        .align 2
.Ltra:  .long   0xFF000020
.Lcounter:
        .long   0x8C900100

        .org    0x10600                 /* VBR + 0x600: interrupt */
vector_irq:
        mov.l   .Lack, r1
        mov     #1, r0
        mov.l   r0, @r1                 /* acknowledge bank B bit 0 */
        mov.l   .Lmask, r1
        mov     #0, r0
        mov.l   r0, @r1                 /* and mask it again */
        mov.l   .Lcounter2, r1
        mov.l   @r1, r0
        add     #1, r0
        mov.l   r0, @r1                 /* ++interrupt counter */
        rte
        nop
        .align 2
.Lack:  .long   0xA05F6904
.Lmask: .long   0xA05F6934
.Lcounter2:
        .long   0x8C900100
