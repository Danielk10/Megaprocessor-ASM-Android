        org  10;

        ds   5,3;

x       equ 3;
        db      x;

        // go through instruction set numerically
        // MOVER
        sxt     r0;
        move    r1,r0;
        move    r2,r0;
        move    r3,r0;
        move    r0,r1;
        sxt     r1;
        move    r2,r1;
        move    r3,r1;
        move    r0,r2;
        move    r1,r2;
        sxt     r2;
        move    r3,r2;
        move    r0,r3;
        move    r1,r3;
        move    r2,r3;
        sxt     r3;

        // AND
        test    r0;
        and     r1,r0;
        and     r2,r0;
        and     r3,r0;
        and     r0,r1;
        test    r1;
        and     r2,r1;
        and     r3,r1;
        and     r0,r2;
        and     r1,r2;
        test    r2;
        and     r3,r2;
        and     r0,r3;
        and     r1,r3;
        and     r2,r3;
        test    r3;
