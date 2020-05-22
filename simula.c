#include <stdio.h>
#include <ctype.h>
#include <alloc.h>
#include <io.h>

#define TRUE   1
#define FALSE  0

#define TAMRAM 0x4000
#define RAMMIN 0x02000000
#define RAMMAX 0x02004000
#define TAMROM 0x8000
#define ROM    0x03000000
#define ROMMIN 0x03800000
#define ROMMAX 0x03808000
#define SINAL  0x80000000

#define NSET   (flags & 0x20)
#define ZSET   (flags & 0x10)
#define CSET   (flags & 0x08)
#define VSET   (flags & 0x04)
#define ISET   (flags & 0x02)
#define FSET   (flags & 0x01)

struct INSTRUCAO { unsigned char cond,tipo,code,r1,r2,r3,r4,f1,f2,shift;
                 unsigned char i,p,u,s,w,l;
                 unsigned list;
                 unsigned long offset; };


unsigned debug,aux,wCont,pos;
unsigned char linha[82],arquivo[80],*strModo[4] = { "MU","MF","MI","MS" };
unsigned char *ptRom,*ptRam,string[80];
unsigned char *flg[18] = { "EQ","NE","CS","CC","MI","PL","VS","VC",
                           "HI","LS","GE","LT","GT","LE","","NV","AL",NULL };
unsigned char *cod[24] = { "AND","EOR","SUB","RSB","ADD","ADC","SBC","RSC",
                           "TST","TEQ","CMP","CMN","ORR","MOV","BIC","MVN",
                           "MUL","MLA","LDR","STR","LDM","STM","SWI",NULL };
unsigned char *sft[7] = { "LSL","LSR","ASR","ROR","RRX","ASL",NULL };
unsigned long lista,assemb,dump,edit,ciclos,breakPoint,wPoint[5],regs[27];





IMP_OP2 (inst)
struct INSTRUCAO *inst;
{ if (inst->i)
         printf("%lx\n",(((unsigned long) inst->r4) << (2*inst->r3))
                          + (((unsigned long) inst->r4) >> (32-2*inst->r3)) );
    else { printf ("R%d",inst->r4);
         if (inst->f1)
                printf (",%s R%d\n",sft[inst->shift],inst->r3);
           else if (!inst->r3)
                       switch (inst->shift)
                         { case 0:  /* lsl */
                             putchar ('\n');
                             break;
                         case 1:    /* lsr */
                         case 2:    /* asr */
                             printf (",%s 20\n",sft[inst->shift]);
                             break;
                         case 3:    /* ror */
                             printf (",RRX\n");
                             break; }
                  else printf (",%s %x\n",sft[inst->shift],inst->r3); }

return (0); }





DECODIFICA (ptx)
unsigned *ptx;
{ unsigned *pi,k,*pw;
struct INSTRUCAO inst;

for (pi=(unsigned *)&inst,k=0;k<11;k++,*pi++=0);
if (lista>=RAMMIN && lista<RAMMAX)
       pi = (unsigned *) (ptRam + (lista - RAMMIN));
  else if (lista>=ROMMIN && lista<ROMMAX)
              pi = (unsigned *) (ptRom + (lista - ROMMIN));
         else { if (ptx)
                       { ptx[0] = 0x30f;
                       ptx[5] = 0x101;
                       ptx[9] = 0xffff;
                       ptx[10] = 0xff; }
                  else printf ("%07lx: ffffffff\tSWINV\tffffff\n",lista);
              return (2); }

inst.cond = pi[1] >> 12;
inst.tipo = (pi[1] >> 10) & 0x03;
inst.i = (pi[1] >> 9) & 0x01;

switch (inst.tipo)
  { case 0:
      inst.code = (pi[1] >> 5) & 0x0f;
      inst.s = (pi[1] >> 4) & 0x01;
      inst.r1 = pi[1] & 0x0f;
      inst.r2 = pi[0] >> 12;
      if (inst.i)
             { inst.r3 = (pi[0] >> 8) & 0x0f;
             inst.r4 = pi[0] & 0xff; }
        else { inst.r4 = pi[0] & 0x0f;
             inst.f1 = (pi[0] >> 4) & 0x01;
             inst.shift = (pi[0] >> 5) & 0x03;
             if (inst.f1)
                    { inst.r3 = (pi[0] >> 8) & 0x0f;
                    inst.f2 = (pi[0] >> 7) & 0x01; }
               else inst.r3 = (pi[0] >> 7) & 0x1f; }
      break;
  case 1:
      if (inst.i && (pi[0] & 0x10))
             { inst.f1 = 1;
             inst.offset = *((unsigned long *) pi) & 0x1ffffff;
             break; }
      inst.p = (pi[1] >> 8) & 0x01;
      inst.u = (pi[1] >> 7) & 0x01;
      inst.s = (pi[1] >> 6) & 0x01;
      inst.w = (pi[1] >> 5) & 0x01;
      inst.l = (pi[1] >> 4) & 0x01;
      inst.r1 = pi[1] & 0x0f;
      inst.r2 = pi[0] >> 12;
      if (inst.i)
             { inst.r4 = pi[0] & 0x0f;
             inst.shift = (pi[0] >> 5) & 0x03;
             inst.r3 = (pi[0] >> 7) & 0x1f; }
        else inst.list = pi[0] & 0xfff;
      break;
  case 2:
      if (inst.i)
             { inst.l = (pi[1] >> 8) & 0x01;
             inst.offset = *((unsigned long *) pi) & 0xffffff;
             if (inst.offset & 0x800000)
                    inst.offset |= 0xff000000; }
        else { inst.p = (pi[1] >> 8) & 0x01;
             inst.u = (pi[1] >> 7) & 0x01;
             inst.s = (pi[1] >> 6) & 0x01;
             inst.w = (pi[1] >> 5) & 0x01;
             inst.l = (pi[1] >> 4) & 0x01;
             inst.r1 = pi[1] & 0x0f;
             inst.list = pi[0]; }
      break;
  case 3:
      inst.p = (pi[1] >> 8) & 0x01;
      if (inst.i)
             if (inst.p)
                    inst.offset = *((unsigned long *) pi) & 0xffffff;
               else { inst.r1 = pi[1] & 0x0f;
                    inst.r2 = pi[0] >> 12;
                    inst.r3 = (pi[0] >> 8) & 0x0f;
                    inst.shift = (pi[0] >> 5) & 0x07;
                    inst.f1 = (pi[0] >> 4) & 0x01;
                    inst.r4 = pi[0] & 0x0f;
                    if (inst.f1)
                           { inst.code = (pi[1] >> 5) & 0x07;
                           inst.l = (pi[1] >> 4) & 0x01; }
                      else inst.code = (pi[1] >> 4) & 0x0f; }
        else { inst.u = (pi[1] >> 7) & 0x01;
             inst.s = (pi[1] >> 6) & 0x01;
             inst.w = (pi[1] >> 5) & 0x01;
             inst.l = (pi[1] >> 4) & 0x01;
             inst.r1 = pi[1] & 0x0f;
             inst.r2 = pi[0] >> 12;
             inst.r3 = (pi[0] >> 8) & 0x0f;
             inst.list = pi[0] & 0xff; }
      break; }

if (ptx)
       { for (pw=&inst,k=0;k<11;ptx[k++]=*pw++);
       goto fim; }

printf ("%07lx: %08lx\t",lista,*((unsigned long *) pi));
switch (inst.tipo)
  { case 0:
      if (!inst.i && inst.f1 && inst.f2)
             if (inst.code>1 || inst.shift)
                    if (inst.code>7)
                           { printf ("INDEF.\n");
                           return (0); }
                      else { printf ("INVAL.\n");
                           return (0); }
               else printf ("%s",(inst.code) ? "MLA" : "MUL");
        else printf ("%s",cod[inst.code]);
      printf ("%s",flg[inst.cond]);
      switch (inst.code)
        { case 0: /* and , mul */
        case 1:   /* eor , mla */
            if (inst.f1 && inst.f2)
                   { if (inst.s)
                            putchar ('S');
                   if (inst.code)
                          printf ("\tR%d,R%d,R%d,R%d\n",inst.r1,
                                                     inst.r4,inst.r3,inst.r2);
                     else { printf("\tR%d,R%d,R%d\n",inst.r1,inst.r4,inst.r3);
                          if (inst.r2)
                                 printf ("Rn=R%d devia ser registrador R0.\n",
                                                                   inst.r2); }
                   if (inst.r1==15)
                          printf ("Instrucao %s - Rd=R15.\n",
                                                 (inst.code) ? "MLA" : "MUL");
                   if (inst.r1==inst.r4)
                          printf ("Instrucao %s - Rd=Rm=R%d.\n",
                                         (inst.code) ? "MLA" : "MUL",inst.r1);
                   break; }
        case 2:   /* sub */
        case 3:   /* rsb */
        case 4:   /* add */
        case 5:   /* adc */
        case 6:   /* sbc */
        case 7:   /* rsc */
        case 12:  /* orr */
        case 14:  /* bic */
            if (inst.s)
                   putchar ('S');
            printf ("\tR%d,R%d,",inst.r2,inst.r1);
            IMP_OP2 (&inst);
            break;
        case 8:   /* tst */
        case 9:   /* teq */
        case 10:  /* cmp */
        case 11:  /* cmn */
            if (inst.r2)
                   putchar ('P');
            printf ("\tR%d,",inst.r1);
            IMP_OP2 (&inst);
            if (inst.r2 && inst.r2!=15)
                  printf("Rd=R%d devia ser registrador R0 ou R15.\n",inst.r2);
            if (!inst.s)
                   printf ("Instrucao %s devia ter bit 's' ligado.\n",
                                                              cod[inst.code]);
            break;
        case 13:  /* mov */
        case 15:  /* mvn */
            if (inst.s)
                   putchar ('S');
            printf ("\tR%d,",inst.r2);
            IMP_OP2 (&inst);
            if (inst.r1)
                   printf ("Instrucao %s, Rn=R%d devia ser R0.\n",
                                                      cod[inst.code],inst.r1);
            break; }
      break;
  case 1:
      if (inst.i && inst.f1)
             { printf ("INDEF.\n");
             return (0); }
      printf ("%s%s",(inst.l) ? "LDR" : "STR",flg[inst.cond]);
      if (inst.s)
             putchar ('B');
      if (!inst.p && inst.w)
             putchar ('T');
      printf ("\tR%d,",inst.r2);
      if (inst.p)
             { printf ("[R%d",inst.r1);
             if (inst.i)
                    { putchar (',');
                    if (!inst.u)
                           putchar ('-');
                    printf ("R%d",inst.r4);
                    if (!inst.r3)
                           switch (inst.shift)
                             { case 0:  /* lsl */
                                 putchar (']');
                                 break;
                             case 1:    /* lsr */
                             case 2:    /* asr */
                                 printf (",%s 20]",sft[inst.shift]);
                                 break;
                             case 3:    /* ror */
                                 printf (",RRX]");
                                 break; }
                      else printf (",%s %x]",sft[inst.shift],inst.r3);
                    if (inst.w)
                           printf ("!\n");
                      else putchar ('\n'); }
               else { if (inst.list)
                             { if (!inst.u)
                                      inst.list = -inst.list;
                             printf (",%x]",inst.list); }
                        else putchar (']');
                    if (inst.w)
                           printf ("!\n");
                      else putchar ('\n'); } }
        else { printf ("[R%d],",inst.r1);
             if (inst.i)
                    { if (!inst.u)
                              putchar ('-');
                    printf ("R%d",inst.r4);
                    if (!inst.r3)
                           switch (inst.shift)
                             { case 0:  /* lsl */
                                 putchar ('\n');
                                 break;
                             case 1:    /* lsr */
                             case 2:    /* asr */
                                 printf (",%s 20\n",sft[inst.shift]);
                                 break;
                             case 3:    /* ror */
                                 printf (",RRX\n");
                                 break; }
                      else printf (",%s %x\n",sft[inst.shift],inst.r3); }
               else { if (!inst.u)
                             inst.list = -inst.list;
                    printf ("%x\n",inst.list); } }
      break;
  case 2:
      if (inst.i)
             printf ("B%s%s\t%lx\n",(inst.l) ? "L" : "",flg[inst.cond],
                             (lista  + 8l + (inst.offset << 2)) & 0x03ffffff);
        else { printf ("%s%s%s%s\tR%d",(inst.l) ? "LDM" : "STM",flg[inst.cond]
                          ,(inst.u) ? "I" : "D",(inst.p) ? "B" : "A",inst.r1);
             if (inst.w)
                    printf ("!,{");
               else printf (",{");
             for (k=0x8000;k;k>>=1)
               printf ("%s",(inst.list & k) ? "1" : "0");
             if (inst.s)
                    printf ("}^\n");
               else printf ("}\n"); }
      break;
  case 3:
      if (!inst.i || !inst.p)
             printf ("COP.\n");
        else printf ("SWI%s\t%lx\n",flg[inst.cond],inst.offset);
      break; }

fim:
return (0); }




STR_REG (x)
int x;
{ int i,mode;
unsigned long l;

mode = regs[15] & 3;
i = (x<8 || x==15) ? x : (mode==1) ? x+8 : (x<13) ? x : (mode==2) ?
         x+10 : (mode) ? x+12 : x;
printf ("regs[%02d] = %08lx\n: ",i,regs[i]);
linha[0] = 9;
cgets (linha);
putchar ('\n');
if (linha[1])
       { sscanf (linha+2,"%lx",&l);
       if (i!=15)
              regs[i] = l;
         else { l &= 0x03fffffc;
              if (l<RAMMIN || (l>=RAMMAX && l<ROMMIN) || l>=ROMMAX)
                     { printf ("Endereco invalido.\n");
                     return (2); }
              regs[15] &= 0xfc000000;
              regs[15] += l + mode; } }

return (0); }





ACESSA_OP (inst,temp1,temp2)
struct INSTRUCAO *inst;
unsigned long *temp1,*temp2;
{ unsigned carry,s;

carry = (regs[15] & 0x20000000) ? 1 : 0;
if (inst->i)
       { *temp2 = (((unsigned long) inst->r4) << (2*inst->r3)) +
                              (((unsigned long) inst->r4) >> (32-2*inst->r3));
       carry = (*temp2 & 0x80000000) ? 1 : 0;
       *temp1 = (inst->r1==15) ? lista + 8l : regs[inst->r1]; }
  else { if (inst->f1)
                { ciclos++;
                if (inst->r3==15)
                       s = (lista + 8l) & 0xff;
                  else s = regs[inst->r3] & 0xff;
                *temp2 = (inst->r4==15) ? regs[15] + 12l : regs[inst->r4];
                *temp1 = (inst->r1==15) ? lista + 12l : regs[inst->r1]; }
           else { s = inst->r3;
                *temp2 = (inst->r4==15) ? regs[15] + 8l : regs[inst->r4];
                *temp1 = (inst->r1==15) ? lista + 8l : regs[inst->r1]; }
       switch (inst->shift)
         { case 0:  /* lsl */
             if (s)
                    { *temp2 <<= s - 1;
                    carry = (*temp2 & 0x80000000) ? 1 : 0;
                    *temp2 <<= 1; }
             break;
         case 1:    /* lsr */
             if (s)
                    { *temp2 >>= s - 1;
                    carry = *temp2 & 0x01;
                    *temp2 >>= 1; }
               else if (!inst->f1)
                           { carry = (*temp2 & 0x80000000) ? 1 : 0;
                           *temp2 = 0l; }
             break;
         case 2:    /* asr */
             if (s)
                    { *((long *) temp2) >>= s - 1;
                    carry = *temp2 & 0x01;
                    *((long *) temp2) >>= 1; }
               else if (!inst->f1)
                           { carry = (*temp2 & 0x80000000) ? 1 : 0;
                           *temp2 = (carry) ? 0xffffffff : 0l; }
             break;
         case 3:    /* ror */
             if (s)
                    { while (s>32)
                        s -= 32;
                    *temp2 = (*temp2 >> s) + (*temp2 << (32-s));
                    carry = (*temp2 & 0x80000000) ? 1 : 0; }
               else if (!inst->f1)
                           { s = *temp2 & 0x01;
                           *temp2 >>= 1;
                           if (carry)
                                  *temp2 |= 0x80000000;
                           carry = s; }
             break; } }

return (carry); }





BANCO (reg,mode)
unsigned reg,mode;
{ if (reg>7 && reg!=15)
         switch (mode)
           { case 0:
               break;
           case 1:
               reg += 8;
               break;
           case 2:
               if (reg>12)
                      reg += 10;
               break;
           case 3:
               if (reg>12)
                      reg += 12;
               break; }

return (reg); }





VERIF_BANCO (inst,mode)
struct INSTRUCAO *inst;
unsigned mode;
{ inst->r1 = BANCO (inst->r1,mode);
inst->r2 = BANCO (inst->r2,mode);
if (!inst->i)
       { if (inst->f1)
                inst->r3 = BANCO (inst->r3,mode);
       inst->r4 = BANCO (inst->r4,mode); }

return (0); }





SIMULA ()
{ unsigned flags,ok,mode,i,k,v,rg;
unsigned char *pc;
unsigned long *pl,temp1,temp2,temp3;
struct INSTRUCAO inst;

ok = TRUE;
lista = regs[15] & 0x03fffffc;
do
  { if (DECODIFICA (&inst))
           break;
  flags = regs[15] >> 26;
  mode = regs[15] & 3;
  switch (inst.cond)
    { case 0:  /* EQ */
        if (!ZSET)
               inst.tipo = 0xff;
        break;
    case 1:    /* NE */
        if (ZSET)
               inst.tipo = 0xff;
        break;
    case 2:    /* CS */
        if (!CSET)
               inst.tipo = 0xff;
        break;
    case 3:    /* CC */
        if (CSET)
              inst.tipo = 0xff;
        break;
    case 4:    /* MI */
        if (!NSET)
               inst.tipo = 0xff;
        break;
    case 5:    /* PL */
        if (NSET)
               inst.tipo = 0xff;
        break;
    case 6:    /* VS */
        if (!VSET)
               inst.tipo = 0xff;
        break;
    case 7:    /* VC */
        if (VSET)
               inst.tipo = 0xff;
        break;
    case 8:    /* HI */
        if (!CSET || ZSET)
               inst.tipo = 0xff;
        break;
    case 9:    /* LS */
        if (CSET && !ZSET)
               inst.tipo = 0xff;
        break;
    case 10:   /* GE */
        if ((NSET && !VSET) || (!NSET && VSET))
               inst.tipo = 0xff;
        break;
    case 11:   /* LT */
        if ((NSET && VSET) || (!NSET && !VSET))
               inst.tipo = 0xff;
        break;
    case 12:   /* GT */
        if (ZSET || (NSET && !VSET) || (!NSET && VSET))
               inst.tipo = 0xff;
        break;
    case 13:   /* LE */
        if (!ZSET && ((NSET && VSET) || (!NSET && !VSET)))
               inst.tipo = 0xff;
        break;
    case 14:   /* AL */
        break;
    case 15:   /* NV */
        inst.tipo = 0xff;
        break; }
  switch (inst.tipo)
    { case 0:
        if (!inst.i && inst.f1 && inst.f2)
               if (inst.code>1 || inst.shift)
                      if (inst.code>7)
                             { ciclos += 4;
                             regs[26] = regs[15] + 4l;
                             regs[15] &= 0xf4000000;
                             regs[15] += 0x0b800007;
                             break; }
                        else { aux = debug = 0;
                             IMP_REGS ();
                             return (0); }
                 else { VERIF_BANCO (&inst,mode);
                      temp1 = (inst.r4==inst.r1) ? 0l :
                               (inst.r4==15) ? regs[15] + 12l : regs[inst.r4];
                      temp2 = (inst.r3==15) ? lista + 8l : regs[inst.r3];
                      if (!inst.code)              /* mul */
                             { if (inst.r1!=15)
                                      regs[inst.r1] = temp1 * temp2; }
                        else if (inst.r1!=15)      /* mla */
                                  regs[inst.r1] = temp1 * temp2 +
                                  (inst.r2==15) ? regs[15]+8l : regs[inst.r2];
                      if (inst.s)
                             { regs[15] &= 0x3fffffff;
                             if (!regs[inst.r1])
                                    regs[15] |= 0x40000000;
                             if (regs[inst.r1] & 0x80000000)
                                    regs[15] |= 0x80000000; }
                      ciclos++;
                      temp1 = 1l;
                      do
                        { ciclos++;
                        if (temp1>=temp2)
                               break;
                        temp1 = temp1 << 2;
                        temp1 += 3l; }
                        while (1);
                      regs[15] += 4;
                      break; }
        VERIF_BANCO (&inst,mode);
        ciclos++;
        k = ACESSA_OP (&inst,&temp1,&temp2);
        v = 0;
        switch (inst.code)
          { case 0:  /* and */
          case 8:    /* tst */
              temp3 = temp1 & temp2;
              break;
          case 1:    /* eor */
          case 9:    /* teq */
              temp3 = temp1 ^ temp2;
              break;
          case 2:    /* sub */
          case 10:   /* cmp */
              temp3 = temp1 - temp2;
              k = (temp3>temp1) ? 1 : 0;
              v = ( (temp3 & SINAL)!=(temp1 & SINAL) &&
                    ((temp1^temp2) & SINAL) ) ? 1 : 0;
              break;
          case 3:    /* rsb */
              temp3 = temp2 - temp1;
              k = (temp3>temp2) ? 1 : 0;
              v = ( (temp3 & SINAL)!=(temp2 & SINAL) &&
                    ((temp1^temp2) & SINAL) ) ? 1 : 0;
              break;
          case 4:    /* add */
          case 11:   /* cmn */
              temp3 = temp1 + temp2;
              k = (temp3<temp1) ? 1 : 0;
              break;
          case 5:    /* adc */
              temp3 = temp1 + temp2 + (CSET ? 1 : 0);
              k = (temp3<temp1) ? 1 : 0;
              break;
          case 6:    /* sbc */
              temp3 = temp1 - temp2 - (CSET ? 1 : 0);
              k = (temp3>temp1) ? 1 : 0;
              v = ( (temp3 & SINAL)!=(temp1 & SINAL) &&
                    ((temp1^temp2) & SINAL) ) ? 1 : 0;
              break;
          case 7:    /* rsc */
            temp3 = temp2 - temp1 - (CSET ? 1 : 0);
            k = (temp3>temp2) ? 1 : 0;
            v = ( (temp3 & SINAL)!=(temp2 & SINAL) &&
                  ((temp1^temp2) & 0x80000000) ) ? 1 : 0;
            break;
          case 12:   /* orr */
              temp3 = temp1 | temp2;
              break;
          case 13:   /* mov */
              temp3 = temp2;
              break;
          case 14:   /* bic */
              temp3 = temp1 & ~temp2;
              break;
          case 15:   /* mvn */
              temp3 = ~temp2;
              break; }
        if (inst.r2!=15)
               { if (inst.code<8 || inst.code>11)
                        regs[inst.r2] = temp3;
               if (inst.s)
                     { if ((1 << inst.code) & 0xf303)
                             regs[15] &= 0x1fffffff;
                         else { regs[15] &= 0x0fffffff;
                              if (v)
                                     regs[15] |= 0x10000000; }
                     if (k)
                            regs[15] |= 0x20000000;
                     if (temp3 & 0x80000000)
                            regs[15] |= 0x80000000;
                     if (!temp3)
                            regs[15] |= 0x40000000; }
               regs[15] += 4; }
          else if (inst.s)
                      { if (regs[15] & 3)
                               if (inst.code<8 || inst.code>11)
                                      { regs[15] = 0l;
                                      ciclos += 3; }
                                 else { regs[15] += 4;
                                      regs[15] &= 0x03fffffc;
                                      temp3 &= 0xfc000003; }
                          else if (inst.code<8 || inst.code>11)
                                      { regs[15] &= 0x0c000003;
                                      temp3 &= 0xf3fffffc;
                                      ciclos += 3; }
                                 else { regs[15] += 4;
                                      regs[15] &= 0x0fffffff;
                                      temp3 &= 0xf0000000; }
                      regs[15] += temp3; }
                 else if (inst.code<8 || inst.code>11)
                             { regs[15] &= 0xfc000003;
                             temp3 &= 0x03fffffc;
                             regs[15] += temp3;
                             ciclos += 3; }
                        else regs[15] += 4;
        break;
    case 1:
        if (inst.i && inst.f1)
               { ciclos += 4;
               regs[26] = regs[15] + 4;
               regs[15] &= 0xf4000000;
               regs[15] += 0x0b800007;
               break; }
        VERIF_BANCO (&inst,mode);
        ciclos += 4;
        temp1 = (inst.r1==15) ? lista + 8l : regs[inst.r1];
        if (inst.i)
               { temp2 = (inst.r4==15) ? regs[15] + 8l : regs[inst.r4];
               switch (inst.shift)
                 { case 0:  /* lsl */
                     temp2 <<= inst.r3;
                     break;
                 case 1:    /* lsr */
                     if (inst.r3)
                            temp2 >>= inst.r3;
                       else temp2 = 0l;
                     break;
                 case 2:    /* asr */
                     if (!inst.r3)
                            inst.r3 = 32;
                     temp2 = ((long) temp2) >> inst.r3;
                     break;
                 case 3:    /* ror */
                     if (inst.r3)
                            temp2 = (temp2>>inst.r3) + (temp2<<(32-inst.r3));
                       else { temp2 >>= 1;
                            if (CSET)
                                   temp2 |= 0x80000000; }
                     break; } }
          else temp2 = inst.list;
        if (inst.p)
               temp1 += (inst.u) ? temp2 : -temp2;
        for (k=0;k<wCont;k++)
          if (temp1==wPoint[k] || (!inst.s && (temp1 & 3==wPoint[k] & 3)))
                 { printf ("Posicao %08lx  - %s ",wPoint[k],strModo[mode]);
                 DECODIFICA (NULL);
                 ok = FALSE; }
        if (inst.l)
               if (inst.s)
                      if (temp1>=RAMMIN && temp1<RAMMAX)
                             temp3 = ptRam[temp1 - RAMMIN];
                        else if (temp1>=ROMMIN && temp1<ROMMAX)
                                    temp3 = ptRom[temp1 - ROMMIN];
                               else { ciclos += 3;
                                    regs[26] = regs[15] + 8l;
                                    regs[15] &= 0xf4000000;
                                    regs[15] += 0x0b800017;
                                    break; }
                 else { k = temp1 & 3;
                      temp1 -= k;
                      if (temp1>=RAMMIN && temp1<RAMMAX)
                             pl = &ptRam[temp1 - RAMMIN];
                        else if (temp1>=ROMMIN && temp1<ROMMAX)
                                    pl = &ptRom[temp1 - ROMMIN];
                               else { ciclos += 3;
                                    regs[26] = regs[15] + 8l;
                                    regs[15] &= 0xf4000000;
                                    regs[15] += 0x0b800017;
                                    break; }
                      temp3 = *pl;
                      temp3 = (temp3 >> (8 * k)) + (temp3 << (32 - 8 * k)); }
          else { temp3 = (inst.r2==15) ? regs[15] + 12l : regs[inst.r2];
               if (inst.s)
                      if (temp1>=RAMMIN && temp1<RAMMAX)
                             ptRam[temp1 - RAMMIN] = temp3;
                        else if (temp1>=ROMMIN && temp1<ROMMAX)
                                    ptRom[temp1 - ROMMIN] = temp3;
                               else { ciclos += 3;
                                    regs[26] = regs[15] + 8l;
                                    regs[15] &= 0xf4000000;
                                    regs[15] += 0x0b800017;
                                    break; }
                 else { k = temp1 & 3;
                      temp1 -= k;
                      if (temp1>=RAMMIN && temp1<RAMMAX)
                             pl = &ptRam[temp1 - RAMMIN];
                        else if (temp1>=ROMMIN && temp1<ROMMAX)
                                    pl = &ptRom[temp1 - ROMMIN];
                               else { ciclos += 3;
                                    regs[26] = regs[15] + 8l;
                                    regs[15] &= 0xf4000000;
                                    regs[15] += 0x0b800017;
                                    break; }
                      temp3 = (temp3 >> (8 * k)) + (temp3 << (32 - 8 * k));
                      *pl = temp3; } }
        if (!inst.p)
               temp1 += (inst.u) ? temp2 : -temp2;
        if (!inst.p || inst.w)
               if (inst.r1!=15)
                      regs[inst.r1] = temp1;
                 else if (inst.r2!=15 || !inst.l)
                             { ciclos += 3;
                             regs[15] &= 0xfc000003;
                             temp1 &= 0x03fffffc;
                             regs[15] += temp1 - 4l; }
        if (inst.l)
               if (inst.r2!=15)
                      regs[inst.r2] = temp3;
                 else { ciclos += 3;
                      regs[15] &= 0xfc000003;
                      temp3 &= 0x03fffffc;
                      regs[15] += temp3 - 4l; }
        regs[15] += 4;
        break;
    case 2:
        if (inst.i)
               { ciclos += 4;
               if (inst.l)
                     { k = (mode==3) ? 26 : (mode==2) ? 24 : (mode) ? 22 : 14;
                     regs[k] = regs[15] + 4l; }
               regs[15] &= 0xfc000003;
               regs[15] += lista + 8l + (inst.offset << 2); }
          else { rg = BANCO (inst.r1,mode);
               temp1 = (rg==15) ? regs[15] + 8l : regs[rg];
               temp1 &= 0xfffffffc;
               for (i=0,k=1;k;k<<=1)
                 if (inst.list & k)
                        i++;
               if (inst.u)
                      temp3 = temp1 + i * 4;
                 else { temp1 -= i * 4;
                      temp3 = temp1;
                      inst.p = (inst.p) ? 0 : 1; }
               ciclos += i + 3;
               if (inst.l && inst.w)
                      { k = (!(inst.list & 0x8000) && inst.s) ? inst.r1 : rg;
                      if (k!=15)
                             regs[k] = temp3;
                      inst.w = 0; }
               for (i=0;i<16;i++)
                 { if (!(inst.list & (1<<i)))
                          continue;
                 if (inst.p)
                        temp1 += 4;
                 for (k=0;k<wCont;k++)
                   if (temp1==wPoint[k] & 3)
                          { printf ("Posicao %08lx  - %s ",
                                                     wPoint[k],strModo[mode]);
                          DECODIFICA (NULL);
                          ok = FALSE; }
                 if (temp1>=RAMMIN && temp1<RAMMAX)
                        pl = &ptRam[temp1 - RAMMIN];
                   else if (temp1>=ROMMIN && temp1<ROMMAX)
                               pl = &ptRom[temp1 - ROMMIN];
                          else if (!i)
                                      { ciclos += 3;
                                      regs[26] = regs[15] + 8l;
                                      regs[15] &= 0xf4000000;
                                      regs[15] += 0x0b800013;
                                      break; }
                                 else { temp2 = -1;
                                      pl = &temp2; }
                 if (inst.l)
                        if (inst.list & 0x8000)
                               if (i!=15)
                                      regs[BANCO (i,mode)] = *pl;
                                 else { ciclos += 3;
                                      if (inst.s)
                                             if (mode)
                                                    regs[15] = *pl;
                                               else { regs[15] &= 0x0c000003;
                                                    temp2 = *pl & 0xf3fffffc;
                                                    regs[15] += temp2; }
                                        else { regs[15] &= 0xfc000003;
                                             regs[15] += *pl & 0x03fffffc; } }
                          else if (inst.s)
                                      regs[i] = *pl;
                                 else regs[BANCO(i,mode)] = *pl;
                   else if (inst.s)
                               *pl = regs[i];
                          else *pl = regs[BANCO(i,mode)];
                 if (!inst.p)
                        temp1 += 4;
                 if (inst.w)
                        { if (rg!=15)
                                 if (inst.s)
                                        regs[inst.r1] = temp3;
                                   else regs[rg] = temp3;
                        inst.w = 0; } }
               regs[15] += 4; }
        break;
    case 3:
        ciclos += 4;
        regs[26] = regs[15] + 4;
        regs[15] &= 0xf4000000;
        regs[15] += (!inst.i || !inst.p) ? 0x0b800007 : 0x0b80000b;
        break;
    case 255:
        ciclos++;
        regs[15] += 4;
        break; }
  lista = regs[15] & 0x03fffffc;
  if (debug==3 || !ok)
         break; }
  while (lista!=breakPoint);

if (!ok && debug!=3 && lista!=breakPoint)
       aux = debug;
debug = 0;
IMP_REGS ();

return (0); }





main (argc,argv)
int argc;
unsigned char *argv[];
{ unsigned i,j,k;
unsigned char *pc,*pd;
unsigned long m,n,z;

INICIA ();
if (!(ptRom=farcalloc((long) TAMROM,(long) sizeof (char))))
       { printf ("Erro na alocacao de memoria.\n");
       ACABA ();
       exit (0); }
if (!(ptRam=farcalloc((long) TAMRAM,(long) sizeof (char))))
       { printf ("Erro na alocacao de memoria.\n");
       ACABA ();
       exit (0); }

if (argc==2)
       { sprintf (arquivo,"%s",argv[1]);
       CARGA (); }
  else arquivo[0] = '\0';

for (i=0;i<27;i++)
  regs[i] = 0l;
regs[15] = 0x0f800003;
wCont = aux = debug = 0;
z = ciclos = 0l;
lista = ROMMIN;
assemb = edit = dump = RAMMIN;
breakPoint = -1l;

while (TRUE)
  { printf ("- ");
  linha[0] = 78;
  cgets (linha);
  linha[2] = tolower (linha[2]);
  if (linha[2]=='q')
         break;
  putchar ('\n');
  switch (linha[2])
    { case '\0':
        break;
    case 'a':
        if (sscanf (linha+3,"%lx",&m)==1)
               { if (m<RAMMIN || (m>=RAMMAX && m<ROMMIN) || m>=ROMMAX)
                        { printf ("   ^erro --> endereco invalido.\n");
                        break; }
               lista = assemb = m & 0xfffffffc; }
        ASSEMBLER ();
        break;
    case 'u':
        i = sscanf (linha+3,"%lx%lx",&m,&n);
        if (i==1)
               { lista = m & 0xfffffffc;
               n = lista + 76l; }
          else if (i==2)
                      lista = m & 0xfffffffc;
                 else n = lista + 76l;
        if (n<lista)
               { for (i=0;i<strlen(linha+2);i++)
                   putchar (' ');
               printf ("  ^erro --> limite invalido.\n");
               break; }
        if (lista<RAMMIN)
               { printf ("Memoria logica nao implementada.\n");
               break; }
          else if (lista<ROM)
                      { if (n>=RAMMAX)
                               { printf ("Memoria fora do limite de 256k.\n");
                               break; } }
                 else if (lista<ROMMIN || n>=ROMMAX)
                            { printf("Memoria fora do limite de ROM alta.\n");
                            break; }
        for (;lista<=n;lista+=4)
          DECODIFICA (NULL);
        break;
    case 'g':
        debug = 1;
        for (i=3;linha[i] && linha[i]!='=';i++);
        if (linha[i])
               { j = sscanf (linha+i+1,"%lx%lx",&m,&n);
               if (j>0)
                      { if (m<RAMMIN || (m>=RAMMAX && m<ROMMIN) || m>=ROMMAX)
                               { printf ("Endereco invalido.\n");
                               debug = 0;
                               break; }
                      m &= 0x03fffffc;
                      regs[15] &= 0xfc000003;
                      regs[15] += m; }
               breakPoint = (j==2) ? n : -1l; }
          else breakPoint = (sscanf(linha+3,"%lx",&m)==1) ? m : -1l;
        breakPoint &= 0xfffffffc;
        break;
    case 'p':
        debug = 2;
        breakPoint = (regs[15] & 0x03fffffc) + 4;
        break;
    case 't':
        debug = 3;
        break;
    case 'd':
        i = sscanf (linha+3,"%lx%lx",&m,&n);
        if (i==1)
               { dump = m & 0xfffffff0;
               n = dump + 127l; }
          else if (i==2)
                      dump = m & 0xfffffff0;
                 else n = dump + 127l;
        if (n<dump)
               { for (i=0;i<strlen(linha+2);i++)
                   putchar (' ');
               printf ("  ^erro --> limite invalido.\n");
               break; }
        if (dump<RAMMIN)
               { printf ("Memoria logica nao implementada.\n");
               break; }
          else if (dump<ROM)
                      { if (n>=RAMMAX)
                               { printf ("Memoria fora do limite de 256k.\n");
                               break; }
                      pc = pd = ptRam + (dump - RAMMIN); }
                 else if (dump<ROMMIN || n>=ROMMAX)
                            { printf("Memoria fora do limite de ROM alta.\n");
                            break; }
                       else pc = pd = ptRom + (dump - ROMMIN);
        for (;dump<=n;dump+=16)
          { printf ("%08lx:",dump);
          for (i=0;i++<16;pc++)
            if (i==9)
                   printf ("-%02x",*pc);
              else printf (" %02x",*pc);
          printf ("  ");
          for (i=0;i++<16;pd++)
            if (*pd<' ')
                   putchar ('.');
              else putchar (*pd);
          putchar ('\n'); }
        break;
    case 'e':
        if (sscanf (linha+3,"%lx",&m)==1)
               edit = m;
        if (edit<RAMMIN)
               { printf ("Memoria logica nao implementada.\n");
               break; }
          else if (edit<ROM)
                      { if (edit>=RAMMAX)
                               { printf ("Memoria fora do limite de 256k.\n");
                               break; }
                      m = RAMMIN;
                      n = RAMMAX;
                      pc = ptRam + (edit - RAMMIN); }
                 else if (edit<ROMMIN || edit>=ROMMAX)
                            { printf("Memoria fora do limite de ROM alta.\n");
                            break; }
                       else { pc = ptRom + (edit - ROMMIN);
                            m = ROMMIN;
                            n = ROMMAX; }
        dump = edit & 0xfffffff0;
        printf ("%07lx: %02x.",edit,*pc);
        for (i=j=k=0;k!=0x0d;)
          { LECAR ();
          k = _AL;
          switch (k)
            { case 0x0d:
                if (i)
                       { *pc = j;
                       if (edit+1!=n)
                              edit++; }
                putchar ('\n');
                break;
            case '-':
                if (i)
                       { *pc = j;
                       j = i = 0; }
                if (edit!=m)
                       { edit--;
                       pc--; }
                printf ("-\n%07lx: %02x.",edit,*pc);
                break;
            case ' ':
                if (i)
                       { *pc = j;
                       i = j = 0; }
                edit++;
                pc++;
                if (!(edit % 8))
                       { if (edit==n)
                                { edit--;
                                pc--; }
                       printf ("\n%07lx: %02x.",edit,*pc); }
                  else printf ("\t%02x.",*pc);
                break;
            case 0x08:
                if (i)
                       { i--;
                       j >>= 4;
                       printf ("\10 \10"); }
                break;
            default:
                if (i>1)
                       break;
                if (isdigit(k))
                       j = (j << 4) + k - '0';
                  else if (k>='a' && k<='f')
                              j = (j << 4) + k - 'a' + 10;
                         else if (k>='A' && k<='F')
                                     j = (j << 4) + k - 'A' + 10;
                                else break;
                i++;
                putchar (k);
                break; } }
        break;
    case 'r':
        if (linha[1]==1)
              IMP_REGS ();
         else if (sscanf (linha+3,"%d",&i)!=1 || i>15)
                    { for (pc=linha+3;isdigit(*pc) || *pc==' ';pc++)
                        putchar (' ');
                    printf("   ^erro --> numero do registrador invalido.\n");
                    break; }
               else STR_REG (i);
        break;
    case 'm':
        sscanf (linha+3,"%s",&i);
        switch (i)
          { case 'U':
          case 'u':
              regs[15] &= 0xfffffffc;
              break;
          case 'F':
          case 'f':
              regs[15] &= 0xfffffffc;
              regs[15] += 1;
              break;
          case 'I':
          case 'i':
              regs[15] &= 0xfffffffc;
              regs[15] += 2;
              break;
          case 'S':
          case 's':
              regs[15] &= 0xfffffffc;
              regs[15] += 3;
              break;
          default:
              for (i=0;i<strlen(linha+2);i++)
                putchar (' ');
              printf ("  ^erro --> modo inexistente.\n");
              break; }
        break;
    case 'f':
        sscanf (linha+3,"%s",&i);
        switch (i)
          { case 'N':
          case 'n':
              regs[15] ^= 0x80000000;
              break;
          case 'Z':
          case 'z':
              regs[15] ^= 0x40000000;
              break;
          case 'C':
          case 'c':
              regs[15] ^= 0x20000000;
              break;
          case 'O':
          case 'o':
              regs[15] ^= 0x10000000;
              break;
          case 'I':
          case 'i':
              regs[15] ^= 0x08000000;
              break;
          case 'F':
          case 'f':
              regs[15] ^= 0x04000000;
              break;
          default:
              for (i=0;i<strlen(linha+2);i++)
                putchar (' ');
              printf ("  ^erro --> funcao inexistente.\n");
              break; }
        break;
    case 'w':
        wCont = sscanf (linha+3,"%lx%lx%lx%lx%lx",
                                  wPoint,wPoint+1,wPoint+2,wPoint+3,wPoint+4);
        if (wCont>5)
               wCont = 0;
        break;
    case 'c':
        debug = aux;
        break;
    case 's':
        if (sscanf (linha+3,"%s%lx",string,&m)!=2)
               printf ("  ^erro --> parametro(s) invalido(s).\n");
          else GRAVA (string,m);
        break;
    case 'n':
        if (sscanf (linha+3,"%s",arquivo)!=1)
               { printf ("  ^erro --> string invalida.\n");
               break; }
    case 'l':
        CARGA ();
        break;
    case 'v':
        VIDEO (ptRam+0x3000);
        break;
    case 'i':
    case 'x':
        for (ciclos=0;ciclos<27;ciclos++)
          regs[ciclos] = 0l;
        regs[15] = (linha[2]=='x') ? 0x0f800003 : 0x0e000003;
    case 'z':
        ciclos = 0l;
        break;
    case 'h':
        if (sscanf(linha+3,"%lx%lx",&m,&n)!=2)
               { for (pc=linha+3;isdigit(*pc) || *pc==' ';pc++)
                   putchar (' ');
               printf ("  ^erro --> parametro(s) invalido(s).\n");
               break; }
        printf ("%lx %lx\n",m+n,m-n);
        break;
    case '?':
        printf ("assembler ........: a | a inicio\n");
        printf ("listagem .........: u | u inicio | u inicio fim\n");
        printf ("executa trecho ...: g | g fim | g=inicio fim\n");
        printf ("executa rotina ...: p\n");
        printf ("passo a passo ....: t\n");
        printf ("dump .............: d | d inicio | d inicio fim\n");
        printf ("edita ............: e | e inicio\n");
        printf ("registradores ....: r | r numero\n");
        printf ("modo .............: m (u,f,i,s)\n");
        printf ("flags ............: f (n,z,c,o,i,f)\n");
        printf ("ponto de parada ..: w pontos-de-parada (5 no maximo)\n");
        printf ("continua .........: c\n");
        printf ("grava arquivo ....: s xxxxxxxx.xxx tamanho\n");
        printf ("nome de arquivo ..: n xxxxxxxx.ram | n xxxxxxxx.xxx\n");
        printf ("carga ............: l\n");
        printf ("quit .............: q\n");
        printf ("video ............: v\n");
        printf ("reset p/ ram .....: i\n");
        printf ("reset p/ rom .....: x\n");
        printf ("zera ciclos ......: z\n");
        printf ("hexa .............: h num1 num2\n");
        break; }
  if (debug)
         { aux = 0;
         SIMULA (); } } }
