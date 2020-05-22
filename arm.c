/**********************************************************************

                   Montador de programas para o ARM
                   --------------------------------

                 Inova Tecnologia e Informatica Ltda
                   Jecel Mattos de Assumpcao Junior

                 Adaptado do ASMZ80 de agosto de 1985
                           em junho de 1988

*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <dir.h>

typedef char BOOLEAN ;

#define TRUE    1
#define FALSE   0
#define ERROR   (-1)

#define ESCAPE         0xdf
#define ORG_EXPR       0x01
#define BYTE_EXPR      0x02
#define WORD_EXPR      0x04
#define REL_EXPR       0x05
#define LINE_STR       0x06
#define CODE_SEG       0x07
#define DATA_SEG       0x08
#define ADDR_MODE      0x10
#define OP2            0x11
#define MARKER         0x1e

char text_area [ 10000 ] , * free_text , * labels [ 1000 ] ;

long unsigned values [ 1000 ] ;

unsigned errors , warnings , free_label ;

long unsigned pc , first_pc , rel_pc ;

BOOLEAN in_first_pass , make_listing ;

FILE * source , * first_draft , * output , * listing ;

struct code_info {
    char * pattern ;
    long unsigned bits ;
    int category ;
    } table [] = {
                 "bls*"		, 0xba000000 , 0 ,
                 "blt*"		, 0xda000000 , 0 ,
                 "ble*"		, 0xfa000000 , 0 ,
                 "bl*"		, 0x0b000000 , 0 ,
                 "b*"		, 0x0a000000 , 0 ,
                 "mov*"		, 0x01a00000 , 1 ,
                 "mvn*"         , 0x01e00000 , 1 ,
                 "cmp*"		, 0x01400000 , 2 ,
                 "cmn*"		, 0x01600000 , 2 ,
                 "teq*"		, 0x01200000 , 2 ,
                 "tst*"		, 0x01000000 , 2 ,
                 "and*"		, 0x00000000 , 3 ,
                 "eor*"		, 0x00200000 , 3 ,
                 "sub*"		, 0x00400000 , 3 ,
                 "rsb*"		, 0x00600000 , 3 ,
                 "add*" 	, 0x00800000 , 3 ,
                 "adc*"		, 0x00a00000 , 3 ,
                 "sbc*"		, 0x00c00000 , 3 ,
                 "rsc*"		, 0x00e00000 , 3 ,
                 "orr*"		, 0x01800000 , 3 ,
                 "bic*"		, 0x01c00000 , 3 ,
                 "mul*"		, 0x00000090 , 4 ,
                 "mla*"		, 0x00200090 , 5 ,
                 "ldr*"		, 0x04900000 , 6 ,
                 "str*"		, 0x04800000 , 6 ,
                 "ldm*"		, 0x08100000 , 7 ,
                 "stm*"		, 0x08000000 , 7 ,
                 "swi*"		, 0x0f000000 , 8 ,
                 "*"		, 0x00000000 , -1
                 } ;

char * conditions [] = {
                      "eq*" ,
                      "ne*" ,
                      "cs*" ,
                      "cc*" ,
                      "mi*" ,
                      "pl*" ,
                      "vs*" ,
                      "vc*" ,
                      "hi*" ,
                      "ls*" ,
                      "ge*" ,
                      "lt*" ,
                      "gt*" ,
                      "le*" ,
                      "al*" ,
                      "nv*" ,
                      "*"
                      } ;

char * options [] = {
                    "da*" ,
                    "ia*" ,
                    "db*" ,
                    "ib*" ,
                    "ed*" ,
                    "ea*" ,
                    "fd*" ,
                    "fa*" ,
                    "*"
                    } ;

main ( argc , argv )
    int argc ;
    char * argv [] ;
    {
    long eval ();
    int flag , i ;
    char * cp1 , * cp2 ;
    char def_drive [ MAXDRIVE ] , usr_drive [ MAXDRIVE ] ,
         def_dir [ MAXDIR ] , usr_dir [ MAXDIR ] ,
         def_file [ MAXFILE ] , usr_file [ MAXFILE ] ,
         def_ext [ MAXEXT ] , usr_ext [ MAXEXT ] ,
         fname [ MAXPATH ] , outname [ MAXPATH ] ,
         arg_buf [ 160 ] ;

    printf ( "\n\n==================================================\n" ) ;
    printf ( "  Assembler ARM - Edicao de 9 de Setembro,1988\n" ) ;
    printf ( "--------------------------------------------------\n" ) ;
    printf ( "Copyright Inova Tecnologia e Informatica Ltda 1988\n" ) ;
    printf ( "==================================================\n\n" ) ;

    strcpy ( def_drive , "" ) ;
    strcpy ( def_dir , "" ) ;
    strcpy ( def_file , "" ) ;
    strcpy ( def_ext , "" ) ;
    source = output = listing = NULL ;
    first_pc = rel_pc = 0xffffffffL ;
    errors = warnings = 0 ;
    make_listing = FALSE ;

    getcwd ( fname , MAXPATH ) ;
    flag = fnsplit ( fname , usr_drive , usr_dir , usr_file , usr_ext ) ;
    if ( flag & DRIVE && def_drive [ 0 ] == '\0' )
        strcpy ( def_drive , usr_drive ) ;
    if ( def_dir [ 0 ] == '\0' )
        {
        if ( flag & DIRECTORY )
            strcpy ( def_dir , usr_dir ) ;
        if ( flag & FILENAME )
            {
            strcat ( def_dir , usr_file ) ;
            strcat ( def_dir , "\\" ) ;
            }
        }

    arg_buf [ 0 ] = '\0' ;
    for ( i = 1 ; i < argc ; ++ i )
        strcat ( arg_buf , argv [ i ] ) ;
    cp1 = arg_buf ;
    i = 0 ;
    while ( * cp1 )
        {
        cp2 = fname ;
        while ( * cp1 && * cp1 != ',' && * cp1 != ';' && ( cp2 == fname || * cp1 != '/' ) )
            * cp2 ++ = * cp1 ++ ;
        * cp2 = '\0' ;
        if ( fname [ 0 ] == '/' )
            if ( fname [ 1 ] == 'o' || fname [ 1 ] == 'O' )
                first_pc = eval ( & fname [ 2 ] ) ;
            else if ( fname [ 1 ] == 'r' || fname [ 1 ] == 'R' )
                rel_pc = eval ( & fname [ 2 ] ) ;
              else
                {
                fprintf ( stderr , "\nARM: opcao desconhecida - /%c\n\n" , fname [ 1 ] ) ;
                exit ( -1 ) ;
                }
          else
            {
            flag = fnsplit ( fname , usr_drive , usr_dir , usr_file , usr_ext ) ;
            if ( i == 0 && flag & DRIVE )
                strcpy ( def_drive , usr_drive ) ;
            if ( i == 0 && flag & DIRECTORY )
                strcpy ( def_dir , usr_dir ) ;
            if ( i == 0 && flag & FILENAME )
                strcpy ( def_file , usr_file ) ;
            if ( ! ( flag & DRIVE ) )
                strcpy ( usr_drive , def_drive ) ;
            if ( ! ( flag & DIRECTORY ) )
                strcpy ( usr_dir , def_dir ) ;
            if ( ! ( flag & FILENAME ) )
                strcpy ( usr_file , def_file ) ;
            if ( ! ( flag & EXTENSION ) )
                if ( i == 0 ) /* nome do arquivo */
                    strcpy ( usr_ext , ".ARM" ) ;
                  else
                    if ( i == 1 ) /* nome do objeto */
                        strcpy ( usr_ext , ".BIN" ) ;
                      else
                        if ( i == 2 ) /* nome da listagem */
                            strcpy ( usr_ext , ".LST" ) ;
            if ( usr_file [ 0 ] == '\0' && i == 0 )
                {
                fprintf ( stderr , "ARM precisa do nome do Arquivo de Entrada\n" ) ;
                exit ( -1 ) ;
                }
            fnmerge ( fname , usr_drive , usr_dir , usr_file , usr_ext ) ;
            if ( i == 0 && ( source = fopen ( fname , "r" ) ) == NULL )
                {
                fprintf ( stderr , "ARM nao conseguiu abrir Arquivo de Entrada\n" ) ;
                exit ( -1 ) ;
                }
            if ( i == 1 )
                if ( ( output = fopen ( fname , "wb" ) ) == NULL )
                    {
                    fprintf ( stderr , "ARM nao conseguiu abrir Arquivo de Saida\n" ) ;
                    exit ( -1 ) ;
                    }
                  else
                    strcpy ( outname , fname ) ;
            if ( i == 2 )
                if ( ( listing = fopen ( fname , "w" ) ) == NULL )
                    {
                    fprintf ( stderr , "ARM nao conseguiu abrir Arquivo de Listagem\n" ) ;
                    exit ( -1 ) ;
                    }
                  else
                    {
                    if ( stricmp ( usr_file , "NUL" ) )
                        make_listing = TRUE ;
                    }
            }
        if ( * cp1 == ';' )
            {
            if ( i == 0 && source == NULL )
                {
                fprintf ( stderr , "ARM precisa do nome do Arquivo de Entrada\n" ) ;
                exit ( -1 ) ;
                }
            if ( i <= 1 && output == NULL )
                {
                fnmerge ( fname , def_drive , def_dir , def_file , ".BIN" ) ;
                if ( ( output = fopen ( fname , "wb" ) ) == NULL )
                    {
                    fprintf ( stderr , "ARM nao conseguiu abrir Arquivo de Saida\n" ) ;
                    exit ( -1 ) ;
                    }
                  else
                    strcpy ( outname , fname ) ;
                }
            if ( i <= 2 && listing == NULL )
                {
                fnmerge ( fname , def_drive , def_dir , "NUL" , ".LST" ) ;
                if ( ( listing = fopen ( fname , "w" ) ) == NULL )
                    {
                    fprintf ( stderr , "ARM nao conseguiu abrir Arquivo de Listagem\n" ) ;
                    exit ( -1 ) ;
                    }
                }
            i = 100 ; /* para ignorar o que vem depois do ";" */
            }
        if ( * cp1 == ',' )
            ++ i ;
        if ( * cp1 == ',' && ( cp1 [ 1 ] == '/' || cp1 [ 1 ] == '\0' ) )
            * cp1 = ' ' ;   /* parametro vazio nao e' nulo !!! */
        else if ( * cp1 && * cp1 != '/' )
            ++ cp1 ; /* pula o caracter de separacao */
        }

    if ( source == NULL )
        {
        strcpy ( def_ext , ".ARM" ) ;
        printf ( "Arquivo de Entrada [%s%s*%s]:" , def_drive , def_dir , def_ext ) ;
        gets ( fname ) ;
        flag = fnsplit ( fname , usr_drive , usr_dir , usr_file , usr_ext ) ;
        if ( ! ( flag & FILENAME ) )
            {
            fprintf ( stderr , "ARM precisa do nome do Arquivo de Entrada\n" ) ;
            exit ( -1 ) ;
            }
        if ( flag & DRIVE )
            strcpy ( def_drive , usr_drive ) ;
        if ( flag & DIRECTORY )
            strcpy ( def_dir , usr_dir ) ;
        if ( flag & FILENAME )
            strcpy ( def_file , usr_file ) ;
        if ( ! ( flag & DRIVE ) )
            strcpy ( usr_drive , def_drive ) ;
        if ( ! ( flag & DIRECTORY ) )
            strcpy ( usr_dir , def_dir ) ;
        if ( ! ( flag & EXTENSION ) )
            strcpy ( usr_ext , def_ext ) ;
        fnmerge ( fname , usr_drive , usr_dir , usr_file , usr_ext ) ;
        if ( ( source = fopen ( fname , "r" ) ) == NULL )
            {
            fprintf ( stderr , "ARM nao conseguiu abrir Arquivo de Entrada\n" ) ;
            exit ( -1 ) ;
            }
        }
    if ( output == NULL )
        {
        strcpy ( def_ext , ".BIN" ) ;
        printf ( "Arquivo de Saida [%s%s%s%s]:" , def_drive , def_dir , def_file , def_ext ) ;
        gets ( fname ) ;
        flag = fnsplit ( fname , usr_drive , usr_dir , usr_file , usr_ext ) ;
        if ( ! ( flag & DRIVE ) )
            strcpy ( usr_drive , def_drive ) ;
        if ( ! ( flag & DIRECTORY ) )
            strcpy ( usr_dir , def_dir ) ;
        if ( ! ( flag & FILENAME ) )
            strcpy ( usr_file , def_file ) ;
        if ( ! ( flag & EXTENSION ) )
            strcpy ( usr_ext , def_ext ) ;
        fnmerge ( fname , usr_drive , usr_dir , usr_file , usr_ext ) ;
        if ( ( output = fopen ( fname , "wb" ) ) == NULL )
            {
            fprintf ( stderr , "ARM nao conseguiu abrir Arquivo de Saida\n" ) ;
            exit ( -1 ) ;
            }
          else
            strcpy ( outname , fname ) ;
        }
    if ( listing == NULL )
        {
        strcpy ( def_file , "NUL" ) ;
        strcpy ( def_ext , ".LST" ) ;
        printf ( "Arquivo de Listagem [%s%s%s%s]:" , def_drive , def_dir , def_file , def_ext ) ;
        gets ( fname ) ;
        flag = fnsplit ( fname , usr_drive , usr_dir , usr_file , usr_ext ) ;
        if ( ! ( flag & DRIVE ) )
            strcpy ( usr_drive , def_drive ) ;
        if ( ! ( flag & DIRECTORY ) )
            strcpy ( usr_dir , def_dir ) ;
        if ( ! ( flag & FILENAME ) )
            strcpy ( usr_file , def_file ) ;
        if ( ! ( flag & EXTENSION ) )
            strcpy ( usr_ext , def_ext ) ;
        fnmerge ( fname , usr_drive , usr_dir , usr_file , usr_ext ) ;
        if ( ( listing = fopen ( fname , "w" ) ) == NULL )
            {
            fprintf ( stderr , "ARM nao conseguiu abrir Arquivo de Listagem\n" ) ;
            exit ( -1 ) ;
            }
          else
            if ( stricmp ( usr_file , "NUL" ) )
                make_listing = TRUE ;
        }

    if ( first_pc == 0xffffffffL && rel_pc != 0xffffffffL )
        first_pc = rel_pc ;
    if ( first_pc == 0xffffffffL )
        first_pc = 0 ;
    if ( rel_pc == 0xffffffffL )
        rel_pc = 0 ;

    sprintf ( fname , "arm.tmp" ) ;                        /* nome diferente */
    first_draft = fopen ( fname , "wb" ) ;                   /* primeira passagem vai criar o arquivo intermediario */
    in_first_pass = TRUE ;
    first_pass () ;                                         /* faz a primeira passagem */
    fclose ( first_draft ) ;
    first_draft = fopen ( fname , "rb" ) ;                   /* a segunda passagem le o arquivo intermediario */
    in_first_pass = FALSE ;
    second_pass () ;                                      /* faz a segunda passagem */
    fclose ( first_draft ) ;
    unlink ( fname ) ;                                   /* elimine o arquivo intermediario */
    printf ( "\n\n\n%d erro%c e %d aviso%c\n" , errors , errors == 1 ? ' ' : 's' ,
                    warnings , warnings == 1 ? ' ' : 's' ) ;
    fclose ( source ) ;
    fclose ( output ) ;
    fclose ( listing ) ;
    if ( errors )                   /* no caso de erros descarte a saida */
        unlink ( outname ) ;
    }


char matched_with ;

/******************  compara um string com um padrao  ***************/

match ( str , with , buf )
    char * str , * with , * buf ;
    {

    tail_recursion:            /* vamos simular recursao */

    if ( * str == 0 && * with == 0 ) return ( TRUE ) ;   /* dois strings nulos sao iguais */
    if ( * with == '.' )                                 /* um '.' e' igual a qualquer numero de brancos e tabs */
        if ( * str == ' ' || * str == '\t' )
            {
            ++ str ;                                     /* se e' branco ou tab compare sem o caracter */
            goto tail_recursion ;
            } /* return ( match ( str + 1 , with , buf ) ) ; */
          else
            {
            ++ with ;                                    /* igual a zero nulos e tabs : compare sem o '.' */
            goto tail_recursion ;
            } ; /* return ( match ( str , with + 1 , buf ) ) ; */
    if ( * with == '*' || * with == '@' || * with == '?' )  /* estes caracteres sao iguais a qualquer numero de caracteres */
        {
        matched_with = * with ;                /* lembre-se pelo que estamos substituindo */
        * buf = '\0' ;                         /* suponha que vai ser igual a zero caracteres */
        if ( ! with [ 1 ] ) /* ultimo caracter - vale qualquer coisa */
            {
            strcpy ( buf , str ) ;             /* pega o resto do string */
            return ( TRUE ) ;
            }
        if ( match ( str , with + 1 , buf + 1 ) )   /* compare sem o meta-caracter */
            return ( TRUE ) ;                  /* a substituicao por nulo deu certo */
          else
            {
            if ( * str == 0 ) return ( FALSE ) ;   /* nao e' nulo mas nao tem pelo que substituir */
            * buf ++ = * str ++ ;                  /* vamos tentar substituir pelo primeiro caracter do string */
            goto tail_recursion ;                  /* compara o resto ( note que o meta-caracter continua o primeiro do padrao ) */
            /* return ( match ( str + 1 , with , buf + 1 ) ) ; */
            } ;
        } ;
    if ( * str ++ == * with ++ ) goto tail_recursion ; /*  return ( match ( str + 1 , with + 1 , buf ) ) ; */
    return ( FALSE ) ;  /* ^-- os primeiros caracteres sao iguais : compare o resto ; senao ja' nao deu */
    }

char * eval_at , * syntax_msg ;

#define DEBLANK  while ( * eval_at == ' ' || * eval_at == '\t' ) ++ eval_at


/**********************  gera as mensagens de erro  *****************/

syntax ( format , message , str2 )
    char * format , * message , * str2 ;
    {
    char form [ 100 ] ;

    ++ errors ;           /* conte os erros; se for aviso quem chama tem que corrigir */
    if ( make_listing == FALSE )
        return ;          /* o resto so' e' feito se houver listagem */
    if ( in_first_pass )  /* na primeira passagem as mensagem sao linhas inseridas no arquivo intermediario */
        {
        fprintf ( first_draft , strcat ( strcpy ( form , "%c%c" ) , format ) ,
                               ESCAPE , LINE_STR , message , str2 ) ;
        putc ( 7 , first_draft ) ;
        putc ( 0 , first_draft ) ;
        }
      else     /* na segunda passagem as mensagens sao inseridas diretamente na listagem */
        {
        fprintf ( listing , strcat ( strcpy ( form , "%07lx              " ) , format ) ,
                            pc , message , str2 ) ;
        putc ( 7 , listing ) ;
        putc ( '\n' , listing ) ;
        } ;
    }


/*  converte o string indicado em um numero de acordo com a base dada  */

long conv ( from , to , base )
    char * from , * to ;
    int base ;
    {
    int digit ;
    long int number ;
    register char * cp ;

    number = 0 ;                    /* comeca com zero. So' inteiros positivos */
    for ( cp = from ; cp <= to ; ++ cp )   /* o string vai so' de from ate' to */
        if ( ( digit = * cp - ( * cp <= '9' ? '0' : ( * cp >= 'a' ? 'a' : 'A' ) - 10 ) ) < base )
            number = number * base + digit ;
          else
            {
            syntax ( "*** erro *** : %s<<<== numero invalido" , syntax_msg ) ;
            return ( ERROR ) ;
            }
    return ( number ) ;                    /* vai calculando ( note que nao e' acusado erro para digitos maiores que a base ) */
    }

/***********  responde qual o valor definido para um label  *********/

lookup ( string )
    char * string ;
    {
    register int i ;
    for ( i = 0 ; i < free_label ; ++ i )     /* para todos os labels */
        if ( strcmp ( labels [ i ] , string ) == 0 )
            return ( i ) ;                    /* se tem nome igual ao string, reponde qual e' o indice na tabela */
    return ( i ) ;        /* nao esta' na tabela : responda a primeira posicao livre */
    }

/************  avalia um termo primario de uma expressao  ***********/

long primary ()
    {
    char * auxp , temp ;
    long int vexp , indx ;

    DEBLANK ;                  /* pula eventuais brancos e tabs */
    if ( * eval_at == '\0' )
        {
        return ( ERROR ) ;     /* nao pode ocorrer */
        }
    else if ( * eval_at == '$' )     /* simboliza o valor atual do contador de instrucoes */
        {
        ++ eval_at ;
        return ( pc ) ;
        }
    else if ( * eval_at >= '0' && * eval_at <= '9' )     /* e' um numero */
        {
        for ( auxp = eval_at ; * auxp >= '0' && * auxp <= '9' ||
                               * auxp >= 'a' && * auxp <= 'f' ||
                               * auxp >= 'A' && * auxp <= 'F' ; ++ auxp ) ; /* acha o fim */
        if ( * auxp == 'h' || * auxp == 'H' )
            vexp = conv ( eval_at , auxp - 1 , 16 ) ;  /* e' um hexadecimal */
        else if ( * -- auxp == 'b' || * auxp == 'B' )
             vexp = conv ( eval_at , auxp - 1 , 2 ) ;  /* e' um binario */
        else
            vexp = conv ( eval_at , auxp , 10 ) ;      /* deve ser um decimal */
        eval_at = auxp + 1 ;
        return ( vexp ) ;
        }
    else if ( * eval_at == '_' || * eval_at >= 'a' && * eval_at <= 'z' || * eval_at >= 'A' && * eval_at <= 'Z' )
        {                                                  /* um label */
        for ( auxp = eval_at ; * auxp == '_' ||
                               * auxp >= '0' && * auxp <= '9' ||
                               * auxp >= 'a' && * auxp <= 'z' ||
                               * auxp >= 'A' && * auxp <= 'Z' ; ++ auxp ) ;  /* acha o fim */
        temp = * auxp ;
        * auxp = '\0' ;
        indx = lookup ( eval_at ) ;    /* procura na tabela */
        if ( indx == free_label )
            {                          /* aqui so' valem nomes ja' definidos */
            syntax ( "*** erro de sintaxe *** : %s<<<== nome desconhecido" , syntax_msg ) ;
            vexp = ERROR ;
            }
          else
            {
            vexp = values [ indx ] ;   /* valor com qual foi definido */
            eval_at = auxp ;
            } ;
        * auxp = temp ;
        return ( vexp ) ;
        } ;
    temp = * eval_at ;                 /* algum caracter inesperado : nao deve ocorrer */
    * eval_at = '\0' ;
    syntax ( "*** erro de sintaxe *** : %s<<<== falta uma expressao aqui" , syntax_msg ) ;
    * eval_at = temp ;
    return ( ERROR ) ;
    }

/***********  responde qual e' a prioridade de um operando  *********/

pri ( op )
    char op ;
    {                            /* quanto maior o numero, maior a prioridade */

    switch ( op )
        {
        case '~' :
        case '!' :
                     return ( 70 ) ;
        case '*' :
        case '/' :
        case '%' :
                     return ( 60 ) ;
        case '+' :
        case '-' :
                     return ( 50 ) ;
        case '>' :
        case '<' :
                     return ( 40 ) ;
        case '&' :
                     return ( 30 ) ;
        case '^' :
                     return ( 20 ) ;
        case '|' :
                     return ( 10 ) ;
        case '(' :
                     return ( 0 ) ;      /* "protege" os operadores abaixo dele na pilha */
        default  :
                     return ( -1 ) ;
        } ;
    }

long unsigned stack [ 30 ] , * sp ;
char op_stack [ 30 ] , * osp ;

/**  executa a opearacao indicada pelo topo da pilha de operadores **/

do_op ()
    {

    switch ( * osp )        /* ve se tem operandos */
        {
        case '~' :                          /* estes precisam de um operando */
        case '!' :
                    if ( sp < stack )
                        {
                        * eval_at = '\0' ;
                        syntax ( "*** erro de sintaxe *** : %s<<<== falta operando" , syntax_msg ) ;
                        return ;
                        } ;
                    break ;
        case '*' :                          /* este precisam de dois operandos */
        case '/' :
        case '%' :
        case '+' :
        case '-' :
        case '>' :
        case '<' :
        case '&' :
        case '^' :
        case '|' :
                    if ( sp <= stack )
                        {
                        * eval_at = '\0' ;
                        syntax ( "*** erro de sintaxe *** : %s<<<== falta operando" , syntax_msg ) ;
                        return ;
                        } ;
                    break ;
        case '(' :                        /* um '(' nunca deve ser executado */
                    * eval_at = '\0' ;
                    syntax ( "*** erro de sintaxe *** : %s<<<== falta um fecha parenteses" , syntax_msg ) ;
                    -- osp ;
                    return ;
        default :                        /* qualquer outro caracter nao vale */
                    * eval_at = '\0' ;
                    syntax ( "*** erro *** : %c<<<== operador invalido" , * osp -- ) ;
                    return ;
        } ;
    switch ( * osp -- )      /* executa e tira da pilha de operadores */
        {
        case '~' :
                    * sp = ~ * sp ;   /* complemento de 1 */
                    break ;
        case '!' :
                    * sp = - * sp ;   /* complemento de 2 */
                    break ;
        case '*' :
                    -- sp ;              /* tiramos dois operandos e devolvemos uma resposta */
                    * sp *= sp [ 1 ] ;   /* produto */
                    break ;
        case '/' :
                    -- sp ;
                    * sp /= sp [ 1 ] ;   /* divisao inteira */
                    break ;
        case '%' :
                    -- sp ;
                    * sp %= sp [ 1 ] ;   /* resto da divisao */
                    break ;
        case '+' :
                    -- sp ;
                    * sp += sp [ 1 ] ;  /* soma */
                    break ;
        case '-' :
                    -- sp ;
                    * sp -= sp [ 1 ] ;  /* subtracao */
                    break ;
        case '>' :
                    -- sp ;
                    * sp >>= sp [ 1 ] ;  /* deslocamento `a direita */
                    break ;
        case '<' :
                    -- sp ;
                    * sp <<= sp [ 1 ] ;  /* deslocamento `a esquerda */
                    break ;
        case '&' :
                    -- sp ;
                    * sp &= sp [ 1 ] ;  /* E bit a bit */
                    break ;
        case '^' :
                    -- sp ;
                    * sp ^= sp [ 1 ] ;  /* OU EXCLUSIVO bit a bit */
                    break ;
        case '|' :
                    -- sp ;
                    * sp |= sp [ 1 ] ;  /* OU INCLUSIVO bit a bit */
                    break ;
        } ;
    }

/****************  avalia a expressao contida no string  ************/

long eval ( string )
    char * string ;
    {
    char * last_eval ;
    BOOLEAN was_operand ;

    eval_at = syntax_msg = string ;
    sp = & stack [ -1 ] ;               /* inicializa as pilhas */
    osp = & op_stack [ -1 ] ;
    was_operand = FALSE ;               /* tem que comecar com operando */
    for ( ; ; )
        {
        DEBLANK ;                       /* pula brancos e tabs */
        if ( * eval_at == '\0' && was_operand )      /* ja' acabou e nao tem operador sobrando */
            {
            while ( osp >= op_stack )
                do_op () ;                      /* faca todas operacoes pendentes */
            if ( sp == stack )
                return ( * sp ) ;               /* chegou a uma resposta : diga qual e' */
            } ;
        if ( * eval_at == '(' && ! was_operand )    /* abre parenteses que nao segue operando */
            {
            * ++ osp = '(' ;       /* guarde na pilha */
            ++ eval_at ;
            continue ;
            } ;
        if ( * eval_at == ')' && was_operand )     /* fecha parenteses que segue operando */
            {
            while ( osp >= op_stack && * osp != '(' )
                do_op () ;                         /* faca todas operacoes pendentes ate' o abre parenteses */
            ++ eval_at ;
            if ( * osp != '(' )
                {                                  /* se nao tinha abre parenteses entao esta' errado */
                * eval_at = '\0' ;
                syntax ( "*** erro de sintaxe *** : %s<<<== falta um abre parenteses" , syntax_msg ) ;
                return ( ERROR ) ;
                } ;
            -- osp ;                 /* pode eliminar o abre parenteses */
            continue ;
            } ;
        if ( ! was_operand && ( * eval_at == '-' || * eval_at == '~' ) )  /* operadores prefixos */
            {
            * ++ osp = * eval_at == '-' ? '!' : '~' ;   /* e' so' guardar na pilha */
            ++ eval_at ;
            continue ;
            } ;
        if ( was_operand && ( * eval_at == '*' || * eval_at == '/' || * eval_at == '%' ||
                              * eval_at == '+' || * eval_at == '-' || * eval_at == '>' ||
                              * eval_at == '<' || * eval_at == '&' || * eval_at == '^' ||
                              * eval_at == '|' ) )               /* operadores infixos */
            {
            if ( ( * eval_at == '>' || * eval_at == '<' ) && * eval_at ++ != * eval_at )
                {
                * eval_at = '\0' ;                 /* um '>' ou '<' so' pode aparecer repitido */
                syntax ( "*** erro de sintaxe *** : %s<<<== operador desconhecido" , syntax_msg ) ;
                return ( ERROR ) ;
                } ;
            while ( osp >= op_stack && pri ( * osp ) >= pri ( * eval_at ) )
               do_op () ;                  /* faca primeiro todas operacoes de maior prioridade pendentes */
            * ++ osp = * eval_at ++ ;      /* guarde o novo operador na pilha */
            was_operand = FALSE ;          /* foi um operador */
            continue ;
            } ;
        if ( ! was_operand && * eval_at != '*' && * eval_at != '/' && * eval_at != '%' &&
                              * eval_at != '+' && * eval_at != '-' && * eval_at != '>' &&
                              * eval_at != '<' && * eval_at != '&' && * eval_at != '^' &&
                              * eval_at != '|' )    /* nao e' um operador */
            {
            last_eval = eval_at ;
            * ++ sp = primary () ;        /* deve ser um primario : guarde na pilha */
            if ( last_eval == eval_at )
                return ( ERROR ) ;        /* houve algum erro */
            was_operand = TRUE ;          /* foi um operando */
            continue ;
            } ;
        * eval_at = '\0' ;   /* qualquer situacao alem destas acima e' um erro */
        syntax ( "*** erro de sintaxe *** : %s<<<== falta operando ou operador" , syntax_msg ) ;
        return ( ERROR ) ;
        } ;
    }

char line_buf [ 256 ] , scratch [ 256 ] ;

/*****************  define um label com o valor dado  ***************/

new_label ( value )
    long int value ;
    {
    char * cp ;
    int indx ;

    for ( cp = free_text ; * cp ; ++ cp )
        if ( ! ( * cp == '_' ||                     /* so' pode ter '_', maiusculas e minusculas */
                 * cp >= '0' && * cp <= '9' ||
                 * cp >= 'a' && * cp <= 'z' ||
                 * cp >= 'A' && * cp <= 'Z' ) )
            {
            if ( make_listing )
                {
                fprintf ( first_draft , "%c%c\007*** nome invalido *** : %s" ,
                                             ESCAPE , LINE_STR , free_text ) ;
                putc ( 0 , first_draft ) ;
                }
            ++ errors ;
            return ;
            } ;
    indx = lookup ( free_text ) ;                  /* ve se ja' conhecemos */
    if ( indx != free_label )
        {                                          /* se ja', avise que vamos redefinir */
        if ( make_listing )
            {
            fprintf ( first_draft , "%c%c\007*** nome redefinido *** : %s" ,
                                        ESCAPE , LINE_STR , free_text ) ;
            putc ( 0 , first_draft ) ;
            }
        ++ errors ;
        }
      else
        {
        ++ free_label ;                        /* vamos ocupar este espaco da tabela */
        labels [ indx ] = free_text ;          /* guarde onde esta' o nome */
        free_text = cp + 1 ;                   /* vamos ficar com esta area de texto */
        } ;
    values [ indx ] = value ;       /* novo valor */
    }

/**************  coloca um byte no arquivo intermediario  ***********/

putbyte ( c )
    unsigned c ;
    {

    ++ pc ;
    if ( ( c & 0x00ff ) == ESCAPE )
        putc ( c , first_draft ) ;      /* coloque um byte normal no arquivo */
    putc ( c , first_draft ) ;          /* coloque 0DFh duas vezes para diferenciar de ESCAPE */
    }

get_seperator ( marker )
    char * * marker ;
    {
    char * cp ;

    cp = * marker ;
    while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
    if ( * cp != ',' )
        {
        if ( make_listing )
            {
            fprintf ( first_draft , "%c%c\007*** falta uma \",\" :%s<<<***" , ESCAPE , LINE_STR , cp ) ;
            putc ( 0 , first_draft ) ;
            }
        ++ errors ;
        return ( FALSE ) ;
        }
    ++ cp ;
    * marker = cp ;
    return ( TRUE ) ;
    }

long get_reg ( marker , str )
    char * * marker ;
    char * str ;
    {
    char * cp ;
    long r ;

    cp = * marker ;
    while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
    if ( * cp != 'r' )
        {
        syntax ( "*** %s - registrador invalido: %s<<< ***" , str , cp ) ;
        * marker = cp ;
        return ( -1L ) ;
        }
    ++ cp ;
    r = 0 ;
    if ( * cp < '0' || * cp > '9' )
        {
        syntax ( "*** %s - registrador invalido: %s<<< ***" , str , cp ) ;
        * marker = cp ;
        return ( -1L ) ;
        }
    r = * cp ++ - '0' ;
    if ( * cp >= '0' && * cp <= '9' )
        {
        r *= 10 ;
        r += * cp ++ - '0' ;
        if ( r > 15 )
            {
            syntax ( "*** %s - registrador invalido: %s<<< ***" , str , cp ) ;
            * marker = cp ;
            return ( -1L ) ;
            }
        }
    * marker = cp ;
    return ( r ) ;
    }

long int saved_pc ;

BOOLEAN in_code ;

/* traduz as instrucoes para linguagem de maquina e define os labels */

first_pass ()
    {
    int aux ;
    char * cp ;
    register struct code_info * instr ;
    int test , tipo ;
    long assembled , rd , rm ;
    BOOLEAN user , translate ;

    in_code = TRUE ;              /* comeca no segmento de codigo */
    pc = first_pc ;
    saved_pc = 0 ;
    free_text = text_area ;       /* inicializacoes */
    free_label = 0 ;
    for ( ; ; )
        {
        if ( fgets ( line_buf , 255 , source ) == NULL )     /* se acabou o arquivo houve um erro : o programa nao tinha "end" */
            {
            ++ errors ;
            fprintf ( stderr , "\007ARM : fim inesperado de arquivo\n" ) ;
            return ;
            } ;
        line_buf [ 255 ] = '\n' ;    /* para o caso de linhas muito longas */
        cp = line_buf ;
        while ( * cp ++ != '\n' ) ;  /* acha o fim de linha */
        * -- cp = '\0' ;             /* transforma a linha em um string */
        strcpy ( scratch  , line_buf ) ;       /* faz uma copia que vamos manipular */
        strlwr ( scratch ) ;         /* nao gostamos de maiusculas - converta-as */
        cp = scratch ;
        while ( * cp && * cp != ';' ) cp ++ ;
         * cp = '\0' ;                         /* elimina qualquer comentario */
        cp = scratch ;
        if ( match ( cp , ".*.:*" , free_text ) )   /* a linha tem um label */
            {
            new_label ( pc ) ;                /* define com o valor atual do pc */
            while ( * cp ++ != ':' ) ;        /* pula o label */
            } ;
        if ( match ( cp , ".end." , free_text ) )    /* linha de fim de programa */
            {
            fprintf ( first_draft , "%c%c%s" , ESCAPE , LINE_STR , line_buf ) ;
            putc ( 0 , first_draft ) ;
            return ;                                 /* acabou */
            } ;
        if ( match ( cp , ".org.*." , free_text ) )  /* linha que define origem */
            {
            pc = eval ( free_text ) + rel_pc ;      /* novo valor do pc */
            fprintf ( first_draft , "%c%c0%lxh" , ESCAPE , ORG_EXPR , pc ) ;
            putc ( 0 , first_draft ) ;     /* ^-- guarda a alteracao para a segunda fase */
            }
        else if ( match ( cp , ".rmb.*." , free_text ) ) /* reserva uma area da memoria */
            {
            pc += eval ( free_text ) ;    /* pula a area reservada */
            fprintf ( first_draft , "%c%c0%lxh" , ESCAPE , ORG_EXPR , pc ) ;
            putc ( 0 , first_draft ) ;    /* ^-- igual um org  */
            }
        else if ( match ( cp , ".db.*." , free_text ) )   /* define literais de 8 bits */
            {
            strcpy ( scratch , free_text ) ;       /* pega o resto da linha */
            while ( match ( scratch , ".*.,*" , free_text ) )    /* enquanto houver elementos separados por ',' */
                {
                if ( match ( scratch , ".\"*\".,*" , free_text ) )  /* o primeiro elemento da lista e' string ? */
                    {
                    cp = free_text ;
                    while ( * cp )           /* manda os bytes do string */
                        {
                        putc ( * cp ++ , first_draft ) ;
                        ++ pc ;
                        } ;
                    }
                else
                    {
                    match ( scratch , ".*.,*" , free_text ) ;     /* pega o primeiro elemento ( nao e' string ) */
                    fprintf ( first_draft , "%c%c%s" , ESCAPE , BYTE_EXPR , free_text ) ;
                    putc ( 0 , first_draft ) ;     /* guarde a expressao literalmente para a segunda fase */
                    ++ pc ;
                    } ;
                cp = free_text ;
                while ( * cp ++ ) ;
                strcpy ( scratch , cp ) ;     /* pega o resto da linha */
                } ;
            if ( match ( scratch , ".\"*\"." , free_text ) )   /* o ultimo elemento e' string */
                {
                cp = free_text ;
                while ( * cp )          /* manda os bytes do string */
                    {
                    putc ( * cp ++ , first_draft ) ;
                    ++ pc ;
                    } ;
                }
              else      /* nao e' string */
                {
                match ( scratch , ".*." , free_text ) ;
                fprintf ( first_draft , "%c%c%s" , ESCAPE , BYTE_EXPR , free_text ) ;
                putc ( 0 , first_draft ) ;    /* mande a expressao para a segunda fase */
                ++ pc ;
                }
            }
        else if ( match ( cp , ".dw.*." , free_text ) )   /* define literalmente expressoes de 32 bits */
            {
            strcpy ( scratch , free_text ) ;
            while ( match ( scratch , ".*.,*" , free_text ) )  /* lista de elementos */
                {
                fprintf ( first_draft , "%c%c%s" , ESCAPE , WORD_EXPR , free_text ) ;
                putc ( 0 , first_draft ) ;               /* manda o primeiro elemento */
                ++ pc ;
                ++ pc ;
                cp = free_text ;
                while ( * cp ++ ) ;
                strcpy ( scratch , cp ) ;     /* pega o resto da linha */
                } ;
            match ( scratch , ".*." , free_text ) ;       /* pega o ultimo elemento da lista */
            fprintf ( first_draft , "%c%c%s" , ESCAPE , WORD_EXPR , free_text ) ;
            putc ( 0 , first_draft ) ;
            ++ pc ;
            ++ pc ;
            ++ pc ;
            ++ pc ;
            }
        else if ( match ( cp , ".code.segment." , free_text ) ) /* vamos para segmento de codigo */
            {
            if ( ! in_code )    /* se estavamos no segmento de dados */
                {
                aux = pc ;
                pc = saved_pc ;     /* troca pc com saved_pc */
                saved_pc = aux ;
                } ;
            in_code  = TRUE ;       /* estamos no codigo */
            fprintf ( first_draft , "%c%c" , ESCAPE , CODE_SEG ) ;  /* avisa a segunda fase */
            }
        else if ( match ( cp , ".data.segment." , free_text ) )  /* vamos para segmento de dados */
            {
            if ( in_code )  /* se estavamos no segmento de codigo */
                {
                aux = pc ;
                pc = saved_pc ;   /* troca pc com saved_pc */
                saved_pc = aux ;
                } ;
            in_code  = FALSE ;    /* estamos nos dados */
            fprintf ( first_draft , "%c%c" , ESCAPE , DATA_SEG ) ;
            }                     /* ^--- avisa a segunda fase */
        else if ( match ( cp , ".*.=.*." , free_text ) )  /* definicao de label */
            {
            cp = free_text ;
            while ( * cp ++ ) ;        /* acha o segundo string */
            new_label ( eval ( cp ) ) ;   /* avalia a expressao e define o label */
            }
        else if ( match ( cp , "." , free_text ) )
                 ;                     /* ignore linhas em branco */
        else                           /* deve conter uma instrucao valida */
            {
            while ( * cp == ' ' || * cp == '\t' ) ++ cp ;   /* pula os brancos e tabs iniciais */
            instr = table ;
            do
                matched_with = 0 ;       /* procura na tabela de instrucoes */
            while ( ! match ( cp , ( instr ++ ) -> pattern , free_text ) ) ;
            if ( ( -- instr ) -> category == -1 )
                {                                 /* nao esta' na tabela ( observe a ultima linha da tabela ) */
                if ( make_listing )
                    {
                    fprintf ( first_draft , "%c%c\007*** erro de sintaxe ou instrucao invalida ***" , ESCAPE , LINE_STR ) ;
                    putc ( 0 , first_draft ) ;
                    }
                ++ errors ;
                }
              else
                {
                assembled = instr -> bits ;   /* os bits basicos */
                strcpy ( cp , free_text ) ;   /* pule o pedaco lido */
                test = 0 ;
                while ( ! match ( cp , conditions [ test ++ ] , free_text ) ) ;
                if ( -- test > 0xf )
                    test = 0xe ;   /* nenhuma condicao equivale a "al" */
                assembled += ( long ) test << 28 ;
                strcpy ( cp , free_text ) ; /* outro pedaco ja' foi lido */
                switch ( instr -> category ) /* trata o resto da linha */
                    {
                    case 0 :          /* <expr rel> */
                             while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                             fprintf ( first_draft , "%c%c0%lxh+(0ffffffh&(((%s)-$-8)>>2))"
                                       , ESCAPE , WORD_EXPR , assembled , cp ) ;
                             putc ( 0 , first_draft ) ;
                             break ;
                    case 1 :          /* [S] Rd,<op2> */
                             while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                             if ( * cp == 's' )
                                 {
                                 ++ cp ;
                                 assembled += 0x00100000L ;  /* bit S */
                                 }
                             assembled += get_reg ( & cp , "Rd" ) << 12 ;
                             if ( get_seperator ( & cp ) )
                                 {
                                 fprintf ( first_draft , "%c%c0%lxh,%s" , ESCAPE , OP2 , assembled , cp ) ;
                                 putc ( 0 , first_draft ) ;
                                 break ;
                                 }
                             fprintf ( first_draft , "%c%c0%lxh" , ESCAPE , WORD_EXPR , assembled ) ;
                             putc ( 0 , first_draft ) ;
                             break ;
                    case 2 :          /* [P] Rn,<op2> */
                             assembled += 0x00100000L ; /* o bit S e' sempre ligado */
                             while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                             if ( * cp == 's' )
                                 ++ cp ;         /* ignore o S: e' sempre ligado */
                             if ( * cp == 'p' )
                                 {
                                 ++ cp ;
                                 assembled += 0x0000f000L ;  /* Rd = 15 */
                                 }
                             assembled += get_reg ( & cp , "Rn" ) << 16 ;
                             if ( get_seperator ( & cp ) )
                                 {
                                 fprintf ( first_draft , "%c%c0%lxh,%s" , ESCAPE , OP2 , assembled , cp ) ;
                                 putc ( 0 , first_draft ) ;
                                 break ;
                                 }
                             fprintf ( first_draft , "%c%c0%lxh" , ESCAPE , WORD_EXPR , assembled ) ;
                             putc ( 0 , first_draft ) ;
                             break ;
                    case 3 :          /* [S] Rd,Rn,<op2> */
                            while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                             if ( * cp == 's' )
                                 {
                                 ++ cp ;
                                 assembled += 0x00100000L ;  /* bit S */
                                 }
                             assembled += get_reg ( & cp , "Rd" ) << 12 ;
                             if ( get_seperator ( & cp ) )
                                 {
                                 assembled += get_reg ( & cp , "Rn" ) << 16 ;
                                 if ( get_seperator ( & cp ) )
                                     {
                                     fprintf ( first_draft , "%c%c0%lxh,%s" , ESCAPE , OP2 , assembled , cp ) ;
                                     putc ( 0 , first_draft ) ;
                                     break ;
                                     }
                                 }
                             fprintf ( first_draft , "%c%c0%lxh" , ESCAPE , WORD_EXPR , assembled ) ;
                             putc ( 0 , first_draft ) ;
                             break ;
                    case 4 :          /* [S] Rd,Rm,Rs */
                             while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                             if ( * cp == 's' )
                                 {
                                 ++ cp ;
                                 assembled += 0x00100000L ;  /* bit S */
                                 }
                             assembled += ( rd = get_reg ( & cp , "Rd" ) ) << 16 ;
                             if ( get_seperator ( & cp ) )
                                 {
                                 assembled += ( rm = get_reg ( & cp , "Rm" ) ) ;
                                 if ( get_seperator ( & cp ) )
                                     assembled += get_reg ( & cp , "Rs" ) << 8 ;
                                 }
                             if ( rm == rd )
                                 syntax ( "*** Rd e Rm devem ser diferentes ***" ) ;
                             if ( rd == 15 )
                                 syntax ( "*** Rd nao pode ser R15 ***" ) ;
                             fprintf ( first_draft , "%c%c0%lxh" , ESCAPE , WORD_EXPR , assembled ) ;
                             putc ( 0 , first_draft ) ;
                             break ;
                    case 5 :          /* [S] Rd,Rm,Rs,Rn */
                             while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                             if ( * cp == 's' )
                                 {
                                 ++ cp ;
                                 assembled += 0x00100000L ;  /* bit S */
                                 }
                             assembled += ( rd = get_reg ( & cp , "Rd" ) ) << 16 ;
                             if ( get_seperator ( & cp ) )
                                 {
                                 assembled += ( rm = get_reg ( & cp , "Rm" ) ) ;
                                 if ( get_seperator ( & cp ) )
                                     {
                                     assembled += get_reg ( & cp , "Rs" ) << 8 ;
                                     if ( get_seperator ( & cp ) )
                                         assembled += get_reg ( & cp , "Rn" ) << 12 ;
                                     }
                                 }
                             if ( rm == rd )
                                 syntax ( "*** Rd e Rm devem ser diferentes ***" ) ;
                             if ( rd == 15 )
                                 syntax ( "*** Rd nao pode ser R15 ***" ) ;
                             fprintf ( first_draft , "%c%c0%lxh" , ESCAPE , WORD_EXPR , assembled ) ;
                             putc ( 0 , first_draft ) ;
                             break ;
                    case 6 :          /* [B][T] Rd,<modo de ender> */
                             while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                             if ( * cp == 'b' )
                                 {
                                 ++ cp ;
                                 assembled += 0x00400000L ;  /* bit B */
                                 }
                             while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                             if ( * cp == 't' )
                                 {
                                 ++ cp ;
                                 translate = TRUE ;
                                 }
                               else
                                 translate = FALSE ;
                             assembled += get_reg ( & cp , "Rd" ) << 12 ;
                             if ( get_seperator ( & cp ) )
                                 {
                                 fprintf ( first_draft , "%c%c0%lxh,%d,%s" , ESCAPE , ADDR_MODE , assembled , translate , cp ) ;
                                 putc ( 0 , first_draft ) ;
                                 }
                               else
                                 {
                                 fprintf ( first_draft , "%c%c0%lxh" , ESCAPE , WORD_EXPR , assembled ) ;
                                 putc ( 0 , first_draft ) ;
                                 }
                             break ;
                    case 7 :          /* <tipo> Rn[!],{Rlist}[^] */
                             tipo = 0 ;
                             while ( ! match ( cp , options [ tipo ++ ] , free_text ) ) ;
                             strcpy ( cp , free_text ) ; /* outro pedaco ja' foi lido */
                             if ( -- tipo > 7 )
                                 tipo = 7 ;  /* o normal e' FA */
                             if ( tipo > 3 && assembled & 0x0100000L )
                                 tipo = ~ tipo ;  /* o LDM e' invertido */
                             assembled |= ( ( long ) tipo & 3 ) << 23 ;
                             assembled += get_reg ( & cp , "Rd" ) << 16 ;
                             while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                             if ( * cp == '!' )
                                 {
                                 ++ cp ;
                                 assembled |= 0x00200000L ;  /* bit de write-back */
                                 }
                             user = FALSE ;
                             if ( get_seperator ( & cp ) ) /* lista de registradores */
                                 if ( match ( cp , ".{.*.}." , free_text )
                                      || ( user = match ( cp , ".{.*.}.^." , free_text ) ) )
                                     {
                                     if ( user )
                                         assembled |= 0x00400000L ;   /* bit S */
                                     strcpy ( cp , free_text ) ;
                                     while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                                     while ( * cp == 'r' )
                                         {
                                         rd = get_reg ( & cp , "Rx" ) ;
                                         while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                                         if ( * cp == '-' )  /* tipo Rx-Ry */
                                             {
                                             ++ cp ;
                                             if ( ( rm = get_reg ( & cp , "Ry" ) ) != -1 )
                                               assembled |= (-1L<<rd) & (~(-1L<<(rm+1))) ;
                                             }
                                           else
                                             assembled |= 1L << rd ; /* tipo Rx */
                                         while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                                         if ( * cp == ',' )
                                             ++ cp ;        /* ignore as virgulas */
                                         while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                                         }
                                     if ( ! match ( cp , "." , free_text ) )
                                         syntax ( "*** erro na lista de registradores: %s<<< ***" , cp ) ;
                                     }
                                   else
                                     syntax ( "*** lista de registradores esperada: %s<<< *** " , cp ) ;
                             fprintf ( first_draft , "%c%c0%lxh" , ESCAPE , WORD_EXPR , assembled ) ;
                             putc ( 0 , first_draft ) ;
                             break ;
                    case 8 :          /* <expr> */
                             while ( * cp == ' ' || * cp == '\t' ) ++ cp ;
                             fprintf ( first_draft , "%c%c0%lxh+(0ffffffh&(%s))" , ESCAPE , WORD_EXPR , assembled , cp ) ;
                             putc ( 0 , first_draft ) ;
                             break ;
                    } ;
                ++ pc ;
                ++ pc ;
                ++ pc ;
                ++ pc ;
                } ;
            } ;
        if ( make_listing )
            {
            fprintf ( first_draft , "%c%c%s" , ESCAPE , LINE_STR , line_buf ) ;
            putc ( 0 , first_draft ) ;      /* ^-- manda todas as linhas para a segunda fase poder criar a listagem */
            }
          else
            fprintf ( first_draft , "%c%c" , ESCAPE , MARKER ) ; /* separe linhas, mesmo sem listagem */
        } ;
    }

#include "arm2.c"