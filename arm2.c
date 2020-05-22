
int bytes_in_line ;
long unsigned max_pc ;

char * lp ;

/*********  inicializa tudo para o inicio de uma nova linha  ********/

newline ()
    {

    sprintf ( scratch , "%07lx " , pc ) ;     /* toda linha comeca com o valor do pc */
    lp = & scratch [ 8 ] ;
    bytes_in_line = 0 ;                      /* ainda nao achamos nenhum byte correspondente a linha atual */
    }

/**********  coloca um byte no arquivo de saida ( objeto )  *********/

putbyte2 ( byte )
    unsigned byte ;
    {

    if ( in_code ) putc ( byte , output ) ;  /* coloca bytes de codigo no arquivo objeto */
    ++ bytes_in_line ;                 /* temos mais um byte nesta linha */
    if ( make_listing )
        {
        sprintf ( lp , "%02x " , byte ) ;  /* manda para a listagem */
        lp += 3 ;
        if ( lp >= & scratch [ 22 ] )
            {                              /* se ja' tem muitos bytes nesta linha vamos continuar na proxima */
            * lp  = '\0' ;
            fprintf ( listing , "%s\n" , scratch ) ;
            pc = pc + bytes_in_line ;
            if ( in_code ) max_pc = pc > max_pc ? pc : max_pc ;
            newline () ;
            } ;
        }
    }

/******  le uma expressao do arquivo intermediario e a avalia  ******/

long get_exp ()
    {
    char * cp ;

    cp = line_buf ;
    while ( * cp ++ = getc ( first_draft ) ) ;   /* pega uma expressao e devolve o seu valor */
    return ( eval ( line_buf ) ) ;
    }

/****************  cria a listagem e arquivo objeto  ***************/

second_pass ()
    {
    char * cp , * fim ;
    long int aux ;
    long unsigned c ;
    long unsigned i , r , s ;
    BOOLEAN translate ;

    in_code = TRUE ;                /* comeca no codigo */
    max_pc = pc = first_pc ;        /* comaca do inicio */
    saved_pc = 0 ;
    newline () ;                    /* comeca tudo */
    while ( ( c = getc ( first_draft ) ) != EOF )    /* tem o que fazer ainda ? */
        if ( c != ESCAPE )
            putbyte2 ( c ) ;       /* byte normal */
          else
            switch ( c = getc ( first_draft ) )     /* sequencia de escape */
                {
                case ESCAPE    :    putbyte2 ( c ) ;      /* era so' um 0xdf originalmente : byte normal */
                                    break ;
                case ORG_EXPR  :    pc = get_exp () ;     /* vamos para outro lugar */
                                    if ( pc == 0xffffffffL )
                                        pc = 0 ;                /* houve algum erro */
                                    if ( in_code )              /* se for dados nao afeta a saida */
                                        if ( pc <= max_pc )     /* alem do fim do arquivo ? */
                                            fseek ( output , ( long ) pc - first_pc , 0 ) ;     /* nao : e' so' ir ate' la' */
                                          else
                                            {
                                            fseek ( output , 0L , 2 ) ;         /* sim : va' ate' o fim e ... */
                                            while ( max_pc ++ < pc )
                                                putc ( 0xff , output ) ;       /* ... preencha com 0xffs ate' chegar la' */
                                            }
                                    break ;
                case BYTE_EXPR :    c = get_exp () ;                     /* segue uma expressao */
                                    if ( ( c & 0xff00 ) != 0 && ( c & 0xff00 ) != 0xff00 )        /* nao da' para representar em 8 bits ... */
                                        {
                                        syntax ( "*** aviso *** : %04xh<<<== expressao truncada" , c ) ;               /* ... entao avise */
                                        -- errors ;
                                        ++ warnings ;      /* foi um aviso, e nao um erro */
                                        }
                                    putbyte2 ( c & 0x00ff ) ;            /* manda o byte correspondente `a expressao */
                                    break ;
                case REL_EXPR :     c = get_exp () - pc - 2 ;            /* segue uma expressao relativa */
                                    if ( ( c & 0xff00 ) != 0 && ( c & 0xff00 ) != 0xff00 )        /* nao da' para representar em 8 bits ... */
                                        {
                                        syntax ( "*** aviso *** : %04xh<<<== desvio para muito longe" , c ) ;               /* ... entao avise */
                                        -- errors ;
                                        ++ warnings ;      /* foi um aviso, e nao um erro */
                                        }
                                    putbyte2 ( c & 0x00ff ) ;           /* manda o byte correspondente `a expressao */
                                    break ;
                case WORD_EXPR :    c = get_exp () ;                    /* pega a expressao */
                                    putbyte2 ( c & 0x00ff ) ;           /* byte menos significativo primeiro */
                                    putbyte2 ( ( c >> 8 ) & 0x00ff ) ;  /* byte segundo menos significativo depois */
                                    putbyte2 ( ( c >> 16 ) & 0x00ff ) ; /* byte segundo mais significativo depois */
                                    putbyte2 ( ( c >> 24 ) & 0x00ff ) ; /* byte mais significativo depois */
                                    break ;
                case OP2       :    cp = line_buf ;
                                    while ( * cp ++ = getc ( first_draft ) ) ;   /* pega uma expressao tipo segundo operando */
                                    cp = line_buf ;
                                    while ( * cp != ',' ) ++ cp ;  /* primeira expressao */
                                    * cp ++ = '\0' ;
                                    aux = eval ( line_buf ) ;
                                    while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                                    if ( * cp == '#' )
                                        {                           /* imediato */
                                        aux |= 0x02000000L ;        /* bit I */
                                        i = eval ( ++ cp ) ;
                                        for ( s = 0 ; s < 16 ; s += 2 )
                                            if ( ! ( ( r = ( i >> ( 2 * s ) | i << ( 32 - 2 * s ) ) ) & 0xffffff00 ) )
                                                break ;
                                        if ( s == 16 )
                                            syntax ( "*** constante invalida : %s <<< ***" , cp ) ;
                                          else
                                            aux += ( s << 8 ) + r ;
                                        }
                                      else if ( * cp == 'r' && ( r = get_reg ( & cp , "Rm" ) ) != -1L )
                                        {                           /* registrador */
                                        aux |= r ;
                                        while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                                        if ( * cp == '\0' )
                                            ;               /* ja' acabou */
                                          else if ( * cp == ',' )
                                            {               /* tem um deslocamento */
                                            s = TRUE ;
                                            if ( match ( ++ cp , ".rrx." , free_text ) )
                                                {
                                                aux |= 0x060L ;
                                                s = FALSE ;      /* e' so' isto */
                                                }
                                              else if ( match ( cp , ".asl.*" , free_text ) ||
                                                        match ( cp , ".lsl.*" , free_text ) )
                                                ;  /*  LSL e' 0 */
                                              else if ( match ( cp , ".lsr.*" , free_text ) )
                                                aux |= 0x020L ;   /* LSR e' 1 */
                                              else if ( match ( cp , ".asr.*" , free_text ) )
                                                aux |= 0x040L ;   /* ASR e' 2 */
                                              else if ( match ( cp , ".ror.*" , free_text ) )
                                                aux |= 0x060L ;   /* ROR e' 3 */
                                              else
                                                {
                                                s = FALSE ;
                                                syntax ( "*** operando invalido : %s <<< tipo de deslocamento desconhecido ***" , cp ) ;
                                                }
                                            if ( s )
                                                {        /* ainda tem mais ??!!?! */
                                                strcpy ( cp , free_text ) ;
                                                if ( * cp == '#' )
                                                    {
                                                    i = eval ( ++ cp ) ;
                                                    if ( ( i & 0xffffffe0L ) == 0 )
                                                        aux |= i << 7 ;
                                                      else
                                                        syntax ( "*** operando invalido : %s <<< constante maior que 32 ***" , cp ) ;
                                                    }
                                                  else if ( * cp == 'r' && ( r = get_reg ( & cp , "Rs" ) ) != -1L )
                                                    aux |= 0x010L | ( r << 8 ) ;
                                                  else
                                                    syntax ( "*** operando invalido : %s <<< deslocamento invalido ***" , cp ) ;
                                                }
                                            }
                                          else
                                            syntax ( "*** operando invalido : %s <<< falta uma \",\" ***" , cp ) ;
                                        }
                                      else
                                        syntax ( "*** operando invalido : %s<<< ***" , cp ) ;
                                    putbyte2 ( aux & 0x00ff ) ;           /* byte menos significativo primeiro */
                                    putbyte2 ( ( aux >> 8 ) & 0x00ff ) ;  /* byte segundo menos significativo depois */
                                    putbyte2 ( ( aux >> 16 ) & 0x00ff ) ; /* byte segundo mais significativo depois */
                                    putbyte2 ( ( aux >> 24 ) & 0x00ff ) ; /* byte mais significativo depois */
                                    break ;
                case ADDR_MODE :    cp = line_buf ;
                                    while ( * cp ++ = getc ( first_draft ) ) ;   /* pega uma expressao tipo modo de enderecamento */
                                    cp = line_buf ;
                                    while ( * cp != ',' ) ++ cp ;  /* primeira expressao */
                                    * cp ++ = '\0' ;
                                    aux = eval ( line_buf ) ;
                                    strcpy ( line_buf , cp ) ;     /* vamos pegar o bit T */
                                    cp = line_buf ;
                                    while ( * cp != ',' ) ++ cp ;  /* segunda expressao */
                                    * cp ++ = '\0' ;
                                    translate = eval ( line_buf ) ;
                                    while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                                    if ( * cp == '[' )
                                        {        /* registradores explicitos */
                                        ++ cp ;
                                        r = get_reg ( & cp , "Rn" ) ;
                                        if ( r != -1L )
                                            {
                                            aux |= r << 16 ;
                                            while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                                            if ( * cp == ',' )
                                                {            /* pre' indexacao */
                                                while ( * ++ cp == ' ' || * cp == '\t' ) ;
                                                if ( * cp == '#' )
                                                    {
                                                    /* FACA IMEDIATO */
                                                    fim = ++ cp ;
                                                    while ( * fim && * fim != ']' ) ++ fim ;  /* acha o fim da expressao */
                                                    if ( * fim != ']' )
                                                        goto FECHA_CHAVE ;      /* nao achou o fim */
                                                    * fim = '\0' ;
                                                    i = eval ( cp ) ;
                                                    * fim = ']' ;       /* restaura */
                                                    cp = fim ;
                                                    if ( ( i & 0xfffff000L ) == 0 )
                                                        aux |= i ;          /* cabe em doze bits */
                                                      else if ( ( 0xfffff000L & -i ) == 0 )
                                                        {
                                                        aux |= -i ;           /* invertido, cabe em doze bits */
                                                        aux ^= 0x00800000L ;  /* apaga bit U */
                                                        }
                                                      else
                                                        syntax ( "*** deslocamento muito grande : %s<<< ***" , cp ) ;
                                                    }
                                                  else
                                                    {
                                                    /* FACA REGISTRADOR */
                                                    aux |= 0x02000000L ;        /* bit I */
                                                    if ( * cp == '+' )          /* ignore o + */
                                                        while ( * ++ cp == ' ' || * cp == '\t' ) ;
                                                    if ( * cp == '-' )
                                                        {
                                                        while ( * ++ cp == ' ' || * cp == '\t' ) ;
                                                        aux ^= 0x00800000L ;   /* apaga bit U */
                                                        }
                                                    r = get_reg ( & cp , "Rm" ) ;
                                                    if ( r != -1L )
                                                        aux |= r ;
                                                    while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                                                    if ( * cp == ',' )
                                                        {               /* tem um deslocamento */
                                                        s = TRUE ;
                                                        if ( match ( ++ cp , ".rrx." , free_text ) )
                                                            {
                                                            aux |= 0x060L ;
                                                            s = FALSE ;      /* e' so' isto */
                                                            }
                                                          else if ( match ( cp , ".asl.*" , free_text ) ||
                                                                    match ( cp , ".lsl.*" , free_text ) )
                                                            ;  /*  LSL e' 0 */
                                                          else if ( match ( cp , ".lsr.*" , free_text ) )
                                                            aux |= 0x020L ;   /* LSR e' 1 */
                                                          else if ( match ( cp , ".asr.*" , free_text ) )
                                                            aux |= 0x040L ;   /* ASR e' 2 */
                                                          else if ( match ( cp , ".ror.*" , free_text ) )
                                                            aux |= 0x060L ;   /* ROR e' 3 */
                                                          else
                                                            {
                                                            s = FALSE ;
                                                            syntax ( "*** operando invalido : %s <<< tipo de deslocamento desconhecido ***" , cp ) ;
                                                            }
                                                        if ( s )
                                                            {        /* ainda tem mais ??!!?! */
                                                            strcpy ( cp , free_text ) ;
                                                            if ( * cp == '#' )
                                                                {
                                                                fim = ++ cp ;
                                                                while ( * fim && * fim != ']' ) ++ fim ;  /* acha o fim da expressao */
                                                                if ( * fim != ']' )
                                                                    goto FECHA_CHAVE ;      /* nao achou o fim */
                                                                * fim = '\0' ;
                                                                i = eval ( cp ) ;
                                                                * fim = ']' ;       /* restaura */
                                                                cp = fim ;
                                                                if ( ( i & 0xffffffe0L ) == 0 )
                                                                    aux |= i << 7 ;
                                                                  else
                                                                    syntax ( "*** operando invalido : %s <<< constante maior que 32 ***" , cp ) ;
                                                                }
                                                              else
                                                                syntax ( "*** operando invalido : %s <<< deslocamento invalido ***" , cp ) ;
                                                            }
                                                        }
                                                    }
                               FECHA_CHAVE:
                                                if ( * cp != ']' )
                                                    syntax ( "*** erro de sintaxe : %s <<< falta um \"]\" ***" , cp ) ;
                                                while ( * ++ cp == ' ' || * cp == '\t' ) ;
                               OFFSET_ZERO:
                                                aux |= 0x01000000L ;     /* marque pre' indexado */
                                                if ( * cp == '!' )
                                                    {
                                                    aux |= 0x00200000L ; /* bit W */
                                                    while ( * ++ cp == ' ' || * cp == '\t' ) ;
                                                    }
                                                if ( translate )
                                                    syntax ( "*** erro : a opcao \"T\" nao pode ser usada com pre'indexacao ***" ) ;
                                                if ( * cp )
                                                    syntax ( "*** modo de enderecamento desconhecido : %s <<< ***" , cp ) ;
                                                }
                                              else if ( * cp == ']' )
                                                {            /* pos indexacao ( provavelmente ) */
                                                while ( * ++ cp == ' ' || * cp == '\t' ) ;
                                                if ( * cp != ',' )
                                                    goto OFFSET_ZERO ; /* era pre'indexado, afinal */
                                                /* FACA POS INDEXADO */
                                                if ( translate )
                                                    aux |= 0x00200000L ; /* bit W no pos indexado e' T */
                                                while ( * ++ cp == ' ' || * cp == '\t' ) ;
                                                if ( * cp == '#' )
                                                    {  /* imediato */
                                                    i = eval ( ++ cp ) ;
                                                    if ( ( i & 0xfffff000L ) == 0 )
                                                        aux |= i ;          /* cabe em doze bits */
                                                      else if ( ( 0xfffff000L & -i ) == 0 )
                                                        {
                                                        aux |= -i ;           /* invertido, cabe em doze bits */
                                                        aux ^= 0x00800000L ;  /* apaga bit U */
                                                        }
                                                      else
                                                        syntax ( "*** deslocamento muito grande : %s<<< ***" , cp ) ;
                                                    }
                                                  else
                                                    {  /* registrador */
                                                    aux |= 0x02000000L ;        /* bit I */
                                                    if ( * cp == '+' )          /* ignore o + */
                                                        while ( * ++ cp == ' ' || * cp == '\t' ) ;
                                                    if ( * cp == '-' )
                                                        {
                                                        while ( * ++ cp == ' ' || * cp == '\t' ) ;
                                                        aux ^= 0x00800000L ;   /* apaga bit U */
                                                        }
                                                    r = get_reg ( & cp , "Rm" ) ;
                                                    if ( r != -1L )
                                                        aux |= r ;
                                                    while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                                                    if ( * cp == '\0' )
                                                        ;               /* ja' acabou */
                                                      else if ( * cp == ',' )
                                                        {               /* tem um deslocamento */
                                                        s = TRUE ;
                                                        if ( match ( ++ cp , ".rrx." , free_text ) )
                                                            {
                                                            aux |= 0x060L ;
                                                            s = FALSE ;      /* e' so' isto */
                                                            }
                                                          else if ( match ( cp , ".asl.*" , free_text ) ||
                                                                    match ( cp , ".lsl.*" , free_text ) )
                                                            ;  /*  LSL e' 0 */
                                                          else if ( match ( cp , ".lsr.*" , free_text ) )
                                                            aux |= 0x020L ;   /* LSR e' 1 */
                                                          else if ( match ( cp , ".asr.*" , free_text ) )
                                                            aux |= 0x040L ;   /* ASR e' 2 */
                                                          else if ( match ( cp , ".ror.*" , free_text ) )
                                                            aux |= 0x060L ;   /* ROR e' 3 */
                                                          else
                                                            {
                                                            s = FALSE ;
                                                            syntax ( "*** operando invalido : %s <<< tipo de deslocamento desconhecido ***" , cp ) ;
                                                            }
                                                        if ( s )
                                                            {        /* ainda tem mais ??!!?! */
                                                            strcpy ( cp , free_text ) ;
                                                            if ( * cp == '#' )
                                                                {
                                                                i = eval ( ++ cp ) ;
                                                                if ( ( i & 0xffffffe0L ) == 0 )
                                                                    aux |= i << 7 ;
                                                                  else
                                                                    syntax ( "*** operando invalido : %s <<< constante maior que 32 ***" , cp ) ;
                                                                }
                                                              else
                                                                syntax ( "*** operando invalido : %s <<< deslocamento invalido ***" , cp ) ;
                                                            }
                                                        }
                                                      else
                                                        syntax ( "*** operando invalido : %s <<< falta uma \",\" ***" , cp ) ;
                                                    }
                                                }
                                              else
                                                syntax ( "*** \",\" ou \"]\" esperado : %s <<< ***" , cp ) ;
                                            }
                                        }
                                      else
                                        {        /* expressao qualquer */
                                        i = eval ( cp ) - pc - 8 ;  /* transforme em relativo */
                                        if ( i < 0x00001000L )
                                            aux |= 0x018f0000 + i ; /* pre'indexado, para cima, relativo a r15, deslocamento */
                                          else if ( i > 0xfffff000L )
                                            aux |= 0x010f0000 - i ; /* pre'indexado, para baixo, relativo a r15, -deslocamento */
                                          else
                                            syntax ( "*** expressao longe demais : %s<<< ***" , cp ) ;
                                        }
                                    putbyte2 ( aux & 0x00ff ) ;           /* byte menos significativo primeiro */
                                    putbyte2 ( ( aux >> 8 ) & 0x00ff ) ;  /* byte segundo menos significativo depois */
                                    putbyte2 ( ( aux >> 16 ) & 0x00ff ) ; /* byte segundo mais significativo depois */
                                    putbyte2 ( ( aux >> 24 ) & 0x00ff ) ; /* byte mais significativo depois */
                                    break ;
                case MARKER    :    pc = pc + bytes_in_line ;           /* atualiza  pc */
                                    bytes_in_line = 0 ;                 /* vamos iniciar outra linha */
                                    if ( in_code ) max_pc = pc > max_pc ? pc : max_pc ;    /* atualiza o marcador de fim de arquivo */
                                    break ;
                case LINE_STR  :    while ( lp < & scratch [ 22 ] )
                                        * lp ++ = ' ' ;                /* vai ate' a coluna 22 preenchendo com brancos */
                                    while ( * lp ++ = getc ( first_draft ) ) ;  /* pega a linha que segue */
                                    fprintf ( listing , "%s\n" , scratch ) ;    /* imprime o resultado na listagem */
                                    pc = pc + bytes_in_line ;          /* atualiza o pc */
                                    if ( in_code ) max_pc = pc > max_pc ? pc : max_pc ;    /* atualiza o marcador de fim de arquivo */
                                    newline () ;                       /* esta linha ja' se foi : comece outra */
                                    break ;
                case CODE_SEG  :    if ( ! in_code )
                                        {
                                        aux = pc ;          /* se estava no segmento de dados troque pc e saved_pc */
                                        pc = saved_pc ;
                                        saved_pc = aux ;
                                        } ;
                                    in_code  = TRUE ;  /* estamos no codigo */
                                    break ;
                case DATA_SEG  :    if ( in_code )
                                        {
                                        aux = pc ;        /* se estava no segmento de dados troque pc e saved_pc  */
                                        pc = saved_pc ;
                                        saved_pc = aux ;
                                        } ;
                                    in_code  = FALSE ;    /* estamos nos dados */
                                    break ;
                default        :    fprintf ( stderr , "\007ARM : erro no arquivo intermediario\n" ) ;  /* sequencia de escape desconhecida : nao deve ocorrer */
                                    ++ errors ;
                                    exit ( -1 ) ;
                } ;
    }
