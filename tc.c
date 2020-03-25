
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOR_DBG


#define BUFLEN 128
#define KEYWORD_NUM 13
#define KEYWORD_LEN 8
#define SYMBOL_NUM 42
#define ID_NUM 128
#define CONST_NUM 128

typedef enum {
    false,
    true
} bool;


enum symbol {
    nul,         ident,     intsym,     plus,      minus,
    times,       slash,     oddsym,     eql,       neq,
    lss,         leq,       gtr,        geq,       lparen,
    rparen,      comma,     semicolon,  period,    becomes,
    beginsym,    endsym,    ifsym,      thensym,   whilesym,
    writesym,    readsym,   dosym,      callsym,   constsym,
    defsym,      floatsym,  varsym,     plusplus,  minusminus,
    comment,     note,
};

/*
    comment : single line annotation lead by #
    note    : multiple rows annotation lead by ###, particularly, note monopolizes one line, and will closed by another ###

*/

#ifdef FOR_DBG

char resymbol[SYMBOL_NUM][KEYWORD_LEN * 3] = {
    "NULL",       "indent",     "int",       "plus",       "minus",      
     "times",     "slash",      "oddsym",    "eql",        "neq",
    "lss",         "leq",       "gtr",        "geq",       "lparen",
    "rparen",      "comma",     "semicolon",  "period",    "becomes",
    "beginsym",    "endsym",    "ifsym",      "thensym",   "whilesym",
    "writesym",    "readsym",   "dosym",      "callsym",   "constsym",
    "defsym",      "floatsym",  "varsym",     "plusplus",  "minusminus",
    "comment",     "note",
};

#endif


char word[KEYWORD_NUM][KEYWORD_LEN];
enum symbol wsym[KEYWORD_NUM]; 

int linenum;
char buf0[BUFLEN]; // double buffer 
char buf1[BUFLEN]; // double buffer
int cc, ll, nn;
char curr[BUFLEN]; // store current symbol
int p;
char tmp[BUFLEN * 10];
char ch;
int symb;

bool need_remedy;
int err; // 统计有多少个错误

FILE * fin;

struct _id{
    char name[BUFLEN];
}id_table[ID_NUM];
int id_table_p;

struct _const{
    int type;
    int value;
}const_table[CONST_NUM];
int const_table_p;

char getch();
void init();

void error(int n, void * detail);
enum symbol getsy();

#define CODE_INCOMPLETE -1
#define UNKNOW_SYM 1
#define NUM_OVERBOUND 2
#define FILE_OPEN_FAILED 3
#define VARIABLE_TOO_LONG 4
#define UNKNOW_TOKEN 5
#define TOOMUCH_ITEMS 6

int main()
{
    printf("sizeof(int)=%d\n----------------\n", sizeof(int));
    
	printf("file name : ");
	scanf("%s", tmp);

	fin = fopen(tmp, "r");
    if(!fin)
    {
        error(FILE_OPEN_FAILED, (void *)tmp);
    }

    init();
    while(-1 != getsy())
    {
        printf("@%d : %d - %s ", linenum, symb, resymbol[symb]);
        if(symb == ident)
        {
            printf("- %s", curr);
        }
        else if(symb == intsym)
        {
            printf("- %d", const_table[const_table_p-1].value);
        }
        else if(symb == endsym)
        {
            printf("END");
            break;
        }
        printf("\n");
    }

    return 0;
}


void init()
{
    cc = ll = nn = 0;
    linenum = 1;
    need_remedy = false;
    err = 0;
    id_table_p = const_table_p = 0;

    /* 设置保留字名字,按照字母顺序，便于折半查找 */
    strcpy(&(word[0][0]), "begin");
    strcpy(&(word[1][0]), "call");
    strcpy(&(word[2][0]), "const");
    strcpy(&(word[3][0]), "do");
    strcpy(&(word[4][0]), "def");
    strcpy(&(word[5][0]), "end");
    strcpy(&(word[6][0]), "if");
    strcpy(&(word[7][0]), "odd");
    strcpy(&(word[8][0]), "read");
    strcpy(&(word[9][0]), "then");
    strcpy(&(word[10][0]), "var");
    strcpy(&(word[11][0]), "while");
    strcpy(&(word[12][0]), "write");

    /* 设置保留字符号 */
    wsym[0] = beginsym;
    wsym[1] = callsym;
    wsym[2] = constsym;
    wsym[3] = dosym;
    wsym[4] = defsym;
    wsym[5] = endsym;
    wsym[6] = ifsym;
    wsym[7] = oddsym;
    wsym[8] = readsym;
    wsym[9] = thensym;
    wsym[10] = varsym;
    wsym[11] = whilesym;
    wsym[12] = writesym;
}

char getch()
{
    if(need_remedy)
    {
        nn = 1 - nn;
        cc = 0;
    }
    if(cc == ll)
    {
        if (feof(fin)) error(CODE_INCOMPLETE, NULL);

        if(nn)  ll = fread(buf0, 1, BUFLEN, fin), nn = 0;
        else    ll = fread(buf1, 1, BUFLEN, fin), nn = 1;
        cc = 0;
    }

    if(nn) // 使用第1个缓存区
        ch = buf1[cc++];
    else // 使用第0个缓冲区
        ch = buf0[cc++];

}

inline bool is_letter()
{
    return (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z') ? true : false;
}

inline bool is_digit()
{
    return (ch >= '0' && ch <= '9') ? true : false;
}

inline bool is_blank()
{
    return (ch==' ' || ch==10 || ch==13 || ch==9) ? true : false;
}


inline void buf_clean()
{
    memset(curr, 0, sizeof(curr)), p = 0;
}


inline void concat()
{
    if(p == BUFLEN-1)
    {
        error(VARIABLE_TOO_LONG, curr);
        return ;
    }
    curr[p++] = ch;
}

inline void retract()
{
    if(cc != 0)
        cc -- ;
    else 
    {
        nn = 1 - nn;
        cc = BUFLEN - 1;
        need_remedy = true;
    }
    
}

void insert_id()
{
    // push curr into a table
    if(id_table_p == ID_NUM - 1)
    {
        error(TOOMUCH_ITEMS, (void *)"id");
        return ;
    }
    strcpy(id_table[id_table_p++].name, curr);

}

int insert_const()
{
    // 将当前的curr转换为数值存储起来
    const_table[const_table_p++].type = symb;

    char * cp = curr;
    bool negative = false;
    int value = 0;

    if(*cp == '-')
    {
        negative = true;
        cp ++;
    }
    else if(*cp == '+')
        cp ++;


    while(*cp != 0)
    {
        value = 10 * value + *cp - '0';
        cp ++;
    }

    if(value > 0x7fff && negative || value > 0x1000 && negative == false)
    {
        error(NUM_OVERBOUND, (void * )curr);
        symb = nul;
        return 0;
    }
    
    const_table[const_table_p++].value = (negative == true ? -value: value);
    symb = intsym;
    return (negative == true ? -value: value);
}


enum symbol check_symb()
{
    int i = 0, j = KEYWORD_NUM-1, k, tmp;
    do{
        k = (i+j)/2;
        tmp = strcmp(curr, word[k]);
        if (tmp < 0)
            j = k - 1;
        else if (tmp > 0)
            i = k + 1;
        else
            return symb = wsym[k];
    } while (i <= j);
    
    return symb = ident;
}

enum symbol getsy()
{
    buf_clean();
    getch();
    
    if(is_blank())
    {
        while(is_blank())
        {
            if(ch == 10) linenum ++;
            getch();
        }
        retract();
        getch();
    }

    if(is_letter())
    {
        while(is_letter() || is_digit())
        {
            concat();
            getch();
        }
        retract();
        if(check_symb())
        {
            insert_id();
            return symb;
        }
        else
        {
            return symb;
        }

    }
    else if(is_digit())
    {
handle_digit:
        while(is_digit())
        {
            concat();
            getch();
        }
        if(is_blank())
        {
            if(ch == 10) retract();
        }
        else if(ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '#' || ch == ';') 
            retract();
        else {error(UNKNOW_SYM, (void *)curr); return symb;}

        insert_const();

        return symb; // symb is assign in insert_const
    }
    else if(ch == '+')
    {
        concat();
        getch();
        if(is_digit())
        {
            goto handle_digit;
        }
        else if(ch == '+')
        {
            return symb = plusplus;
        }
        else
        {
            retract();
            return symb = plus;
        }
    }
    else if(ch == '-')
    {
        concat();
        getch();
        if(is_digit())
        {
            goto handle_digit;
        }
        else if(ch == '-')
        {
            return symb = minusminus;
        }
        else
        {
            retract();
            return symb = minus;
        }
    }
    else if(ch == '*')
    {
        return symb = times;
    }
    else if(ch == '/')
    {
        return symb = slash;
    }
    else if(ch == ';')
    {
        return symb = semicolon;
    }
    else if(ch == ',')
    {
        return symb = comma;
    }
    else if(ch == '{')
    {
        return symb = beginsym;
    }
    else if(ch == '}')
    {
        return symb = endsym;
    }
    else if(ch == '#')
    {
        getch();
        if(ch == '#')
        {
            getch();
            if(ch == '#') // triple #
            {
                int flag = 0;
                do{
                    getch();
                    if(ch == 10) linenum++;
                    else if(ch == '#') flag ++;
                    else flag = 0;
                }while(flag != 3);
                return symb = note;
            }
            else // double #, see as single #
            {
                do{
                    getch();
                }while(ch != 10);
                linenum ++;
                return symb = comment;
            }
        }
        else if(ch == 10)
        {
            linenum ++;
            return symb = comment;
        }
        else
        {
            do{
                getch();
            }while(ch != 10);
            retract();
            return symb = comment;
        }
        
    }
    else
    {
        error(UNKNOW_TOKEN, (void *)ch);
        return symb = nul;
    }

}

void error(int n, void * detail)
{
    printf("[%d] line %d: error ", ++err, linenum);

	switch(n)
	{
		case CODE_INCOMPLETE:
			printf("program incomplete");
			break;
		case UNKNOW_SYM:
			printf("unknow symb");
			break;

        case NUM_OVERBOUND:
            printf("number overbound");
            break;
        case FILE_OPEN_FAILED:
            printf("file open failed");
            break;
        case VARIABLE_TOO_LONG:
            printf("variable too long");
            break;
        case UNKNOW_TOKEN:
            printf("unknow token %c", (char)detail);
            break;

        case TOOMUCH_ITEMS:
            printf("for %s: too much items", (char *)detail);
            break;
	}
    exit(-1);
    printf("\n");

}
