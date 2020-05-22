#include <stdio.h>
#include <ctype.h>

#define TRUE   1
#define FALSE  0

#define TAMRAM 0x4000
#define RAMMIN 0x02000000
#define RAMMAX 0x02004000
#define TAMROM 0x8000
#define ROM    0x03000000
#define ROMMIN 0x03800000
#define ROMMAX 0x03808000

#define SD     0x00100000
#define IMM    0x02000000
#define PRE    0x01000000
#define UP     0x00800000
#define SG     0x00400000
#define WB     0x00200000

#define RM      0
#define RS      8
#define RD     12
#define RN     16
#define MRN    12
#define MRD    16

#define NSET   (flags & 0x20)
#define ZSET   (flags & 0x10)
#define CSET   (flags & 0x08)
#define VSET   (flags & 0x04)
#define ISET   (flags & 0x02)
#define FSET   (flags & 0x01)

extern unsigned pos;
extern unsigned char *ptRam,*ptRom,linha[],*flg[],*cod[],*sft[],*strModo[];
extern unsigned char arquivo[];
extern unsigned long ciclos,lista,assemb,regs[];
unsigned long bsft[4] = { 0x00,0x20,0x40,0x60 };
unsigned long bcod[25] = { 0x00000000,0x00200000,0x00400000,0x00600000,
                0x00800000,0x00a00000,0x00c00000,0x00e00000,0x01000000,
                0x01200000,0x01400000,0x01600000,0x01800000,0x01a00000,
                0x01c00000,0x01e00000,0x00000090,0x00200090,0x04100000,
     0x04000000,0x08100000,0x08000000,0x0f000000,0x0a000000,0x0b000000 };
unsigned long bflg[17] = { 0x00000000,0x10000000,0x20000000,0x30000000,
                           0x40000000,0x50000000,0x60000000,0x70000000,
                           0x80000000,0x90000000,0xa0000000,0xb0000000,
                0xc0000000,0xd0000000,0xe0000000,0xf0000000,0xe0000000 };
struct INSTRUCAO { unsigned char cond,tipo,code,r1,r2,r3,r4,f1,f2,shift;
                 unsigned char i,p,u,s,w,l;
                 unsigned list;
                 unsigned long offset; };



IMP_REGS ()
{ int i,l,flags,mode;
unsigned char *pc;

flags = regs[15] >> 26;
lista = regs[15] & 0x03fffffc;
mode = regs[15] & 3;

printf ("R0 = ");
for (i=0;i<8;i++)
  printf ("%08lx ",regs[i]);

printf ("\nR8 = ");
if (mode==1)
       { i += 8;
       l = 21; }
  else l = 13;
for (;i<l;i++)
  printf ("%08lx ",regs[i]);
switch (mode)
  { case 0:     /* user */
      printf ("%08lx %08lx ",regs[13],regs[14]);
      break;
  case 1:       /* firq */
      printf ("%08lx %08lx ",regs[21],regs[22]);
      break;
  case 2:       /* irq */
      printf ("%08lx %08lx ",regs[23],regs[24]);
      break;
  case 3:       /* sup */
      printf ("%08lx %08lx ",regs[25],regs[26]);
      break; }
printf ((ciclos>99999999) ? "%8lxH\n" : "%8ld\n",ciclos);

if (NSET)
       printf ("NG ");
  else printf ("PS ");
if (ZSET)
       printf ("ZR ");
  else printf ("NZ ");
if (CSET)
       printf ("CY ");
  else printf ("NC ");
if (VSET)
       printf ("OV ");
  else printf ("NO ");
if (ISET)
       printf ("DI ");
  else printf ("EI ");
if (FSET)
       printf ("DF ");
  else printf ("EF ");
printf ("- %s ",strModo[mode]);

if (DECODIFICA (NULL))
       printf ("Acesso a posicao fora da memoria.\n");

return (0); }





CARGA ()
{ int arq,tipo,i,j;
unsigned long l;
unsigned char *pc;

if (!arquivo[0])
       return (0);
pc = arquivo;
while (*pc && *pc++!='.');
if (!stricmp(pc,"RAM"))
       { tipo = 0;
       pc = ptRam; }
  else { tipo = 1;
       pc = ptRom; }

if ((arq=ABRE(arquivo))<0)
       { printf ("Erro ao abrir arquivo %s.\n",arquivo);
       return (1); }
l = filelength (arq);
if ((tipo && l>0x8000) || (!tipo && l>0x4000))
       { printf ("Arquivo %s muito grande.\n",arquivo);
       return (1); }
for (i=j=0;(j=LEIA(arq,pc,512))>0;i++,pc+=512);
if (!i || j<0)
       { printf ("Erro na leitura do arquivo %s.\n",arquivo);
       FECHA (arq);
       return (1); }
if (FECHA (arq))
       { printf ("Erro ao fechar arquivo %s.\n",arquivo);
       return (1); }
if (tipo)
       { for (i=0;i<27;regs[i++]=0l);
       regs[15] = 0x0f800003;
       ciclos = 0l; }

return (0); }





GRAVA (string,bytes)
unsigned char *string;
unsigned long bytes;
{ unsigned arq,i;
unsigned char *pc;

for (pc=string;*pc && *pc++!='.';);
if ((!stricmp(pc,"ram") && bytes>0x4000) || bytes>0x8000)
       { printf ("Tamanho invalido.\n");
       return (0); }

if ((arq=CRIA(string))<0)
       { printf ("Erro na criacao do arquivo %s.\n",string);
       return (1); }
if (!stricmp(pc,"RAM"))
       pc = ptRam;
  else pc = ptRom;
for (;bytes;pc+=512)
  { i = (bytes>512) ? 512 : bytes;
  if (ESCREVA(arq,pc,i)!=i)
         { FECHA (arq);
         printf ("Erro de escrita no arquivo %s.\n",string);
         return (1); }
  bytes -= i; }

if (FECHA (arq))
       { printf ("Erro ao fechar arquivo %s.\n",string);
       return (1); }

return (0); }





ERRO (str)
unsigned char *str;
{ unsigned i;

pos += 7;
for (i=0;i<pos;i++)
  putchar (' ');
printf ("^erro --> %s\n",str);

return (0); }





BRANCO (param)
int param;
{ int i;

i = 0;
switch (param)
  { case 0:
      for (pos++;isspace(linha[pos]);pos++);
      break;
  case 1:
      for (i=++pos;isspace(linha[i]);i++);
      for (;isdigit(linha[i]);i++);
      break;
  case 2:
      for (;isspace(linha[pos]);pos++);
      break; }

return (i); }





ASSEMBLER ()
{ unsigned long *pl,temp,aux;
unsigned char cnd[4],instr[4];
unsigned tipo,i,j,k,ok;

if (assemb>=RAMMIN && assemb<RAMMAX)
       { aux = assemb - RAMMIN;
       pl = ptRam + aux; }
  else { aux = assemb - ROMMIN;
       pl = ptRom + aux; }
instr[3] = '\0';
cnd[2] = '\0';
while (TRUE)
  { printf ("%07lx: ",assemb);
  linha[0] = 70;
  cgets (linha);
  for (i=2;linha[i];i++)
    linha[i] = toupper (linha[i]);
  for (pos=2;isspace(linha[pos]);pos++);
  putchar ('\n');
  if (!linha[pos])
         break;
  if (linha[pos]=='B')
         if (linha[pos+1]=='L')
                if (linha[pos+2]=='S' || linha[pos+2]=='T' ||
                    linha[pos+2]=='E')
                       { temp = bcod[23];
                       tipo = 23;
                       pos++; }
                  else { temp = bcod[24];
                       tipo = 24;
                       pos += 2; }
           else { temp = bcod[23];
                tipo = 23;
                pos++; }
    else { for (i=0;i<3;i++,pos++)
             { instr[i] = linha[pos];
             if (!linha[pos])
                    break; }
         for (i=0;cod[i] && strcmp(cod[i],instr);i++);
         if (!cod[i])
                { ERRO ("instrucao inexistente.");
                continue; }
         temp = bcod[i];
         tipo = i; }
  switch (linha[pos])
    { case 'E':
        if (linha[pos+1]=='Q')
               { cnd[0] = 'E';
               cnd[1] = 'Q';
               pos += 2; }
          else cnd[0] = '\0';
        break;
    case 'P':
        if (linha[pos+1]=='L')
               { cnd[0] = 'P';
               cnd[1] = 'L';
               pos += 2; }
          else cnd[0] = '\0';
        break;
    case 'N':
    case 'C':
    case 'M':
    case 'V':
    case 'H':
    case 'L':
    case 'G':
    case 'A':
        cnd[0] = linha[pos++];
        if (linha[pos])
               if (!isspace(linha[pos]))
                      cnd[1] = linha[pos++];
                 else cnd[1] = '\0';
        break;
    default:
        cnd[0] = '\0';
        break; }
  for (i=0;flg[i] && strcmp(flg[i],cnd);i++);
  if (!flg[i])
         { ERRO ("condicao inexistente.");
         continue; }
  temp += bflg[i];
  switch (tipo)
    { case 0: /* AND */
    case 1:   /* EOR */
    case 2:   /* SUB */
    case 3:   /* RSB */
    case 4:   /* ADD */
    case 5:   /* ADC */
    case 6:   /* SBC */
    case 7:   /* RSC */
    case 12:  /* ORR */
    case 14:  /* BIC */
        if (linha[pos]=='S')
               { temp += SD;
               pos++; }
        if (!isspace(linha[pos]))
               { ERRO ("instrucao invalida.");
               continue; }
          else BRANCO (0);
        if (linha[pos]!='R')
               { ERRO ("registrador RD esperado.");
               continue; }
        i = BRANCO (1);
        if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
               { pos = i;
               ERRO ("numero de registrador invalido.");
               continue; }
        temp += j << RD;
        pos = i;
        BRANCO (2);
        if (linha[pos]!=',')
               { ERRO ("caracter ',' esperado.");
               continue; }
        BRANCO (0);
        if (linha[pos]!='R')
               { ERRO ("registrador RN esperado.");
               continue; }
        i = BRANCO (1);
        if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
               { pos = i;
               ERRO ("numero de registrador invalido.");
               continue; }
        temp += ((long) j) << RN;
        goto OP_2;
    case 8:   /* TST */
    case 9:   /* TEQ */
    case 10:  /* CMP */
    case 11:  /* CMN */
        temp += SD;
        if (linha[pos]=='P')
               { temp += 15 << RD;
               pos++; }
        if (!isspace(linha[pos]))
               { ERRO ("instrucao invalida.");
               continue; }
          else BRANCO (0);
        if (linha[pos]!='R')
               { ERRO ("registrador Rn esperado.");
               continue; }
        i = BRANCO (1);
        if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
               { pos = i;
               ERRO ("numero de registrador invalido.");
               continue; }
        temp += ((long) j) << RN;
        goto OP_2;
    case 13:  /* MOV */
    case 15:  /* MVN */
        if (linha[pos]=='S')
               { temp += SD;
               pos++; }
        if (!isspace(linha[pos]))
               { ERRO ("instrucao invalida.");
               continue; }
          else BRANCO (0);
        if (linha[pos]!='R')
               { ERRO ("registrador Rd esperado.");
               continue; }
        i = BRANCO (1);
        if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
               { pos = i;
               ERRO ("numero de registrador invalido.");
               continue; }
        temp += j << RD;
        OP_2:
        pos = i;
        BRANCO (2);
        if (linha[pos]!=',')
               { ERRO ("caracter ',' esperado.");
               continue; }
        BRANCO (0);
        if (isxdigit(linha[pos]))
               { temp += IMM;
               for (i=pos+1;isxdigit(linha[i]);i++);
               aux = 0l;
               sscanf (linha+pos,"%lx",&aux);
               for (k=0;k<16 && aux>0xff;k++)
                 aux = (aux >> 2) + (aux << 30);
               if (k==16)
                      { pos = i;
                      ERRO ("constante invalida.");
                      continue; }
               temp += aux + (k << 8);
               pos = i;
               BRANCO (2); }
          else { if (linha[pos]!='R')
                        { ERRO ("registrador Rm esperado.");
                        continue; }
               i = BRANCO (1);
               if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
                      { pos = i;
                      ERRO ("numero de registrador invalido.");
                      continue; }
               temp += j << RM;
               pos = i;
               BRANCO (2);
               if (linha[pos]==',')
                      { BRANCO (0);
                      for (i=0;i<3;i++,pos++)
                        { instr[i] = linha[pos];
                        if (!linha[pos])
                               break; }
                      for (k=0;sft[k] && strcmp(sft[k],instr);k++);
                      if (!sft[k] || !isspace(linha[pos]))
                             { ERRO ("shift inexistente.");
                             continue; }
                      if (k==4)
                             { temp += 0x60;
                             BRANCO (2);
                             if (linha[pos])
                                    { ERRO ("caracter invalido.");
                                    continue; }
                             break; }
                      if (k==5)
                             k = 0;
                      for (pos++;isspace(linha[pos]);pos++);
                      if (isxdigit(linha[pos]))
                             { for (i=pos+1;isxdigit(linha[i]);i++);
                             sscanf (linha+pos,"%x",&j);
                             if (j>32)
                                    { pos = i;
                                    ERRO ("numero hexadecimal invalido.");
                                    continue; }
                             if (j)
                                    temp += (k << 5) + (j << 7);
                             pos = i;
                             BRANCO (2); }
                        else { temp += (k << 5) + 0x10;
                             if (linha[pos]!='R')
                                    { ERRO ("registrador Rs esperado.");
                                    continue; }
                             i = BRANCO (1);
                             if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
                                    { pos = i;
                                    ERRO ("numero de registrador invalido.");
                                    continue; }
                             temp += j << RS;
                             pos = i;
                             BRANCO (2); } } }
        if (linha[pos])
               { ERRO ("caracter invalido.");
               continue; }
        break;
    case 16:  /* MUL */
    case 17:  /* MLA */
        if (linha[pos]=='S')
               { temp += SD;
               pos++; }
        if (!isspace(linha[pos]))
               { ERRO ("instrucao invalida.");
               continue; }
          else BRANCO (0);
        ok = TRUE;
        for (k=0;;k++)
          { if (linha[pos]!='R')
                   { ok = FALSE;
                   ERRO ("registrador Rx esperado.");
                   break; }
          i = BRANCO (1);
          if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
                 { ok = FALSE;
                 pos = i;
                 ERRO ("numero de registrador invalido.");
                 break; }
          switch (k)
            { case 0:
                temp += ((long) j) << MRD;
                break;
            case 1:
                temp += j << RM;
                break;
            case 2:
                temp += j << RS;
                break;
            case 3:
                temp += j << MRN;
                break; }
          pos = i;
          BRANCO (2);
          if ((tipo==16 && k==2) || k==3)
                 break;
          if (linha[pos]!=',')
                 { ok = FALSE;
                 ERRO ("caracter ',' esperado.");
                 break; }
          BRANCO (0); }
        if (!ok)
               continue;
        if (linha[pos])
               { ERRO ("caracter invalido.");
               continue; }
        break;
    case 18:  /* LDR */
    case 19:  /* STR */
        if (linha[pos]=='B')
               { temp += SG;
               pos++; }
        if (linha[pos]=='T')
               { temp += WB;
               k = TRUE;
               pos++; }
          else k = FALSE;
        if (!isspace(linha[pos]))
               { ERRO ("instrucao invalida.");
               continue; }
          else BRANCO (0);
        if (linha[pos]!='R')
               { ERRO ("registrador Rd esperado.");
               continue; }
        i = BRANCO (1);
        if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
               { pos = i;
               ERRO ("numero de registrador invalido.");
               continue; }
        temp += j << RD;
        pos = i;
        BRANCO (2);
        if (linha[pos]!=',')
               { ERRO ("caracter ',' esperado.");
               continue; }
        BRANCO (0);
        if (linha[pos]!='[')
               { ERRO ("caracter '[' esperado.");
               continue; }
        BRANCO (0);
        if (linha[pos]!='R')
               { ERRO ("registrador Rn esperado.");
               continue; }
        i = BRANCO (1);
        if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
               { pos = i;
               ERRO ("numero de registrador invalido.");
               continue; }
        temp += ((long) j) << RN;
        pos = i;
        BRANCO (2);
        if (linha[pos]==']')
               { for (i=pos+1;isspace(linha[i]);i++);
               if (linha[i]=='!')
                      for (j=i+1;isspace(linha[j]);j++);
                 else j = i;
               if (!linha[j])
                      { k += 2;
                      goto label_1; }
               pos = i; }
          else k += 2;
        if (linha[pos]!=',')
               { ERRO ("caracter ',' esperado.");
               continue; }
        BRANCO (0);
        if (linha[pos]=='-')
               BRANCO (0);
          else { if (linha[pos]=='+')
                        BRANCO (0);
               temp += UP; }
        if (isxdigit(linha[pos]))
               { for (i=pos+1;isxdigit(linha[i]);i++);
               aux = 0l;
               sscanf (linha+pos,"%lx",&aux);
               aux &= 0xfff;
               temp += aux;
               pos = i;
               BRANCO (2); }
          else { temp += IMM;
               if (linha[pos]!='R')
                      { ERRO ("registrador Rm esperado.");
                      continue; }
               i = BRANCO (1);
               if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
                      { pos = i;
                      ERRO ("numero de registrador invalido.");
                      continue; }
               temp += j << RM;
               pos = i;
               BRANCO (2);
               if (linha[pos]==',')
                      { BRANCO (0);
                      for (i=0;i<3;i++,pos++)
                        { instr[i] = linha[pos];
                        if (!linha[pos])
                               break; }
                      for (k=0;sft[k] && strcmp(sft[k],instr);k++);
                      if (!sft[k] || !isspace(linha[pos]))
                             { ERRO ("shift inexistente.");
                             continue; }
                      if (k==4)
                             { temp += 0x60;
                             BRANCO (2);
                             goto label_1; }
                      if (k==5)
                             k = 0;
                      for (i=pos+1;isspace(linha[i]);i++);
                      for (;isxdigit(linha[i]);i++);
                      if (sscanf (linha+pos,"%x",&j)!=1 || j>32)
                             { pos = i;
                             ERRO ("numero hexadecimal invalido.");
                             continue; }
                      if (j)
                             temp += (k << 5) + (j << 7);
                      pos = i;
                      BRANCO (2); } }
        label_1:
        if (k>1)
               { if (linha[pos]!=']')
                        { ERRO ("caracter ']' esperado");
                        continue; }
               if (k==3)
                      { ERRO ("opcao 'T' em pre-incremento.");
                      continue; }
               temp += PRE;
               BRANCO (0);
               if (linha[pos]=='!')
                      { temp += WB;
                      BRANCO (0); } }
        if (linha[pos])
               { ERRO ("caracter invalido.");
               continue; }
        break;
    case 20:  /* LDM */
    case 21:  /* STM */
        switch (linha[pos])
          { case 'E':
              pos++;
              if (tipo==20)        /* LDM */
                     temp += PRE;
              switch (linha[pos])
                { case 'A':
                    if (tipo==21)        /* STM */
                           temp += UP;
                    break;
                case 'D':
                    if (tipo==20)        /* LDM */
                           temp += UP;
                    break;
                default:
                    ERRO ("caracter A/D esperado.");
                    continue; }
              pos++;
              break;
          case 'F':
              pos++;
              if (tipo==21)        /* STM */
                     temp += PRE;
              switch (linha[pos])
                { case 'A':
                    if (tipo==21)        /* STM */
                           temp += UP;
                    break;
                case 'D':
                    if (tipo==20)        /* LTM */
                           temp += UP;
                    break;
                default:
                    ERRO ("caracter A/D esperado.");
                    continue; }
              pos++;
              break;
          case 'I':
              temp += UP;
          case 'D':
              pos++;
              switch (linha[pos])
                { case 'B':
                    temp += PRE;
                case 'A':
                    pos++;
                    break;
                default:
                    ERRO ("caracter B/A esperado.");
                    continue; }
              break;
          default:
              ERRO ("modo IA/IB/DA/DB/FD/ED/FA/EA esperado.");
              continue; }

        if (!isspace(linha[pos]))
               { ERRO ("instrucao invalida.");
               continue; }
          else BRANCO (0);
        if (linha[pos]!='R')
               { ERRO ("registrador Rn esperado.");
               continue; }
        i = BRANCO (1);
        if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
               { pos = i;
               ERRO ("numero de registrador invalido.");
               continue; }
        temp += ((long) j) << RN;
        pos = i;
        BRANCO (2);
        if (linha[pos]=='!')
               { temp += WB;
               BRANCO (0); }
        if (linha[pos]!=',')
               { ERRO ("caracter ',' esperado.");
               continue; }
        BRANCO (0);
        if (linha[pos]!='{')
               { ERRO ("caracter '{' esperado.");
               continue; }
        BRANCO (0);
        j = 0xffff;
        ok = TRUE;
        do
          { if (!linha[pos])
                   { ERRO ("lista incompleta.");
                   ok = FALSE;
                   break; }
          if (linha[pos]=='}')
                 { if (j==0xffff)
                          { ERRO ("lista de registradores vazia.");
                          ok = FALSE; }
                     else if (j==0xfffe)
                                 { ERRO ("lista incompleta.");
                                 ok = FALSE; }
                            else pos++;
                 break; }
          if (j<16)
                 { ERRO ("caracter ',' esperado.");
                 ok = FALSE;
                 break; }
          if (linha[pos]!='R')
                 { ERRO ("registrador Rx esperado.");
                 ok = FALSE;
                 break; }
          i = BRANCO (1);
          if (sscanf(linha+pos,"%d",&j)!=1 || j>15)
                 { pos = i;
                 ERRO ("numero de registrador invalido.");
                 ok = FALSE;
                 break; }
          pos = i;
          BRANCO (2);
          if (linha[pos]==',' || linha[pos]=='}')
                 { temp += 1 << j;
                 if (linha[pos]==',')
                        { pos++;
                        j = 0xfffe; }
                 BRANCO (2);
                 continue; }
          if (linha[pos]!='-')
                 { ERRO ("caracter '-' esperado.");
                 ok = FALSE;
                 break; }
          BRANCO (0);
          if (linha[pos]!='R')
                 { ERRO ("registrador Rx esperado.");
                 ok = FALSE;
                 break; }
          i = BRANCO (1);
          if (sscanf(linha+pos,"%d",&k)!=1 || k>15)
                 { pos = i;
                 ERRO ("numero de registrador invalido.");
                 ok = FALSE;
                 break; }
          pos = i;
          if (j>k)
                 { ERRO ("lista invalida.");
                 ok = FALSE;
                 break; }
          for (i=j;i<=k;i++)
            temp += 1 << i;
          BRANCO (2);
          if (linha[pos]==',')
                 { j = 0xfffe;
                 pos++; }
          BRANCO (2); }
          while (TRUE);
        if (!ok)
               continue;
        BRANCO (2);
        if (linha[pos]=='^')
               { temp += SG;
               pos++; }
        BRANCO (2);
        if (linha[pos])
               ERRO ("caracter invalido.");
        break;
    case 22:  /* SWI */
        if (linha[pos])
               if (!isspace(linha[pos]))
                      { ERRO ("instrucao invalida.");
                      continue; }
                 else BRANCO (0);
        for (i=pos;isxdigit(linha[i]);i++);
        for (;isspace(linha[i]);i++);
        if (linha[i])
               { pos = i;
               ERRO ("caracter invalido.");
               continue; }
        aux = 0l;
        sscanf (linha+pos,"%lx",&aux);
        temp += aux & 0xffffff;
        break;
    case 23:  /* B */
    case 24:  /* BL */
        if (!isspace(linha[pos]))
               { ERRO ("instrucao invalida.");
               continue; }
          else BRANCO (0);
        for (i=pos;isxdigit(linha[i]);i++);
        for (;isspace(linha[i]);i++);
        if (linha[i])
               { pos = i;
               ERRO ("caracter invalido.");
               continue; }
        sscanf (linha+pos,"%lx",&aux);
        temp += ((aux - assemb - 8l) >> 2) & 0xffffff;
        break; }
  *pl = temp;
  assemb += 4;
  if ((assemb>=RAMMAX && assemb<ROMMIN) || assemb>=ROMMAX)
         { printf ("Acesso a posicao fora da memoria.\n");
         break; }
  pl++; }

return (0); }
