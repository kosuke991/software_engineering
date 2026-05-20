#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getsym.h>

#define MAX_ENTRY_LEN 32
#define SYNTAX_ERR "syntax error"
#define UNDEFINED "Undefined identifier"

#define gen_load(rd, s) { fprintf(outfile, "\tload %s, %d\n", rd, s);}
#define gen_loadr(rd, rs) { fprintf(outfile, "\tloadr %s, %s\n", rd, rs);}
#define gen_loadi(rd) { fprintf(outfile, "\tloadi %s, %d\n", rd, tok.value);}
#define gen_store(rs, d) { fprintf(outfile, "\tstore %s, %d\n", rs, d);}
#define gen_addi(rd, immed) { fprintf(outfile, "\taddi %s, %d\n", rd, immed);}
#define gen_addr(rd, rs) { fprintf(outfile, "\taddr %s, %s\n", rd, rs);}
#define gen_subr(rd, rs) { fprintf(outfile, "\tsubr %s, %s\n", rd, rs);}
#define gen_mulr(rd, rs) { fprintf(outfile, "\tmulr %s, %s\n", rd, rs);}
#define gen_divr(rd, rs) { fprintf(outfile, "\tdivr %s, %s\n", rd, rs);}
#define gen_writed(register) { \
	fprintf(outfile, "\twrited %s\n", register); \
	fprintf(outfile, "\tloadi r3, '\\n'\n"); \
	fprintf(outfile, "\twritec r3\n"); \
}
#define gen_push(rs) { fprintf(outfile, "\tpush %s\n", rs);}
#define gen_pop(rs) { fprintf(outfile, "\tpop %s\n", rs);}
#define gen_cmpr(rs, rt) { fprintf(outfile, "\tcmpr %s, %s\n", rs, rt);}
#define gen_label(label) { fprintf(outfile, "L%d:\n", label);}
#define gen_jmp(label) { fprintf(outfile, "\tjmp L%d\n", label);}
#define gen_jnz(label) { fprintf(outfile, "\tjnz L%d\n", label);}
#define gen_jz(label) { fprintf(outfile, "\tjz L%d\n", label);}
#define gen_jgt(label) { fprintf(outfile, "\tjgt L%d\n", label);}
#define gen_jge(label) { fprintf(outfile, "\tjge L%d\n", label);}
#define gen_jlt(label) { fprintf(outfile, "\tjlt L%d\n", label);}
#define gen_jle(label) { fprintf(outfile, "\tjle L%d\n", label);}

extern TOKEN tok;
extern FILE *infile;
extern FILE *outfile;

int rear_addr = 0;
int label_counter = 0;

typedef struct {
	int addr;
	char id[MAXIDLEN + 1];
} symbol_entry;
symbol_entry symbol_table[MAX_ENTRY_LEN];


void error(char *s);
void outblock(void);
void statement(void);
void expression(void);
void condition(int label);

int search_addr(void) {
	for (int i = 0; i < MAX_ENTRY_LEN; i++) {
		if (strcmp(tok.charvalue, symbol_table[i].id) == 0) return i;
	}
	error(UNDEFINED);
	return -1;
}

void compiler(void){
	init_getsym();

	getsym();

	if (tok.attr == RWORD && tok.value == PROGRAM){

		getsym();

		if (tok.attr == IDENTIFIER){

			getsym();

			if (tok.attr == SYMBOL && tok.value == SEMICOLON){

				getsym();

				outblock();

				if (tok.attr == SYMBOL && tok.value == PERIOD){
					fprintf(outfile, "\thalt\n");
					fprintf(stderr, "Parsing Done. No errors found.\n");
				} else error("At the end, a period is required.");
			}else error("After program name, a semicolon is needed.");
		}else error("Program identifier is needed.");
	}else error("At the first, program declaration is required.");
}

void error(char *s){
	fprintf(stderr, "L%d: Error '%s' at token [%s] (attr:%d, val:%d)\n", 
            tok.sline, s, tok.charvalue, tok.attr, tok.value);
	exit(1);
}

void outblock(void){
	grammer = "outblock";

	if (tok.attr == RWORD && tok.value == VAR) {
		do {
			getsym();

			if (tok.attr == IDENTIFIER) {
				strcpy(symbol_table[rear_addr].id, tok.charvalue);
				symbol_table[rear_addr].addr = rear_addr;
				rear_addr++;

				getsym();
				if (tok.attr == SYMBOL && tok.value == SEMICOLON) {
					getsym();
					gen_addi("SP", rear_addr);
					break;
				}
			}
		} while (tok.attr == SYMBOL && tok.value == COMMA);
	} else {
		error(SYNTAX_ERR);
	}

	statement();
}

void statement(void){
	grammer = "statement";

	if (tok.attr == IDENTIFIER) {
		int addr = search_addr();
		getsym();

		if (tok.attr == SYMBOL && tok.value == BECOMES) {
			getsym();
			expression();
			gen_store("r1", addr);
		} else {
			error(SYNTAX_ERR);
		}

	} else if (tok.attr == RWORD && tok.value == BEGIN) {
		do {
			getsym();
			statement();
		} while (tok.attr == SYMBOL && tok.value == SEMICOLON);

		if (tok.attr == RWORD && tok.value == END) {
			getsym();
		} else {
			error(SYNTAX_ERR);
		}

	} else if (tok.attr == RWORD && tok.value == IF) {
		int first_label = label_counter++;
		int second_label = label_counter++;
		getsym();
		condition(first_label);

		if (tok.attr == RWORD && tok.value == THEN) {
			getsym();
			statement();
			
			if (tok.attr == RWORD && tok.value == ELSE) {
				gen_jmp(second_label);
				gen_label(first_label);
				getsym();
				statement();
				gen_label(second_label);
			} else {
				gen_label(first_label);
			}
		} else {
			error(SYNTAX_ERR);
		}

	} else if (tok.attr == RWORD && tok.value == WHILE) {
		int first_label = label_counter++;
		int second_label = label_counter++;
		gen_label(first_label);

		getsym();
		condition(second_label);

		if (tok.attr == RWORD && tok.value == DO) {
			getsym();
			statement();
			gen_jmp(first_label);
			gen_label(second_label);
		} else {
			error(SYNTAX_ERR);
		}

	} else if (tok.attr == RWORD && tok.value == WRITE) {
		do {
			getsym();
			
			if (tok.attr == IDENTIFIER) {
				int addr = search_addr();
				gen_load("r0", addr);
				gen_writed("r0")
				getsym();
			} else {
				error(SYNTAX_ERR);
			}
		} while (tok.attr == SYMBOL && tok.value == COMMA);

	} else {
		error(SYNTAX_ERR);
	}

	if (tok.attr == SYMBOL && tok.value == PERIOD) return;
}

void load_number(char *rd) {
	if (tok.attr == NUMBER) {
		gen_loadi(rd);

	} else if (tok.attr == IDENTIFIER) {
	int addr = search_addr();
	gen_load(rd, addr);

	} else {
		error(SYNTAX_ERR);
	}
}

/*計算結果はr1に格納される*/
void expression(void) {
	grammer = "expression";

	if (tok.attr == NUMBER) {
		gen_loadi("r1");
	} else if (tok.attr == IDENTIFIER) {
		int addr = search_addr();
		gen_load("r1", addr);
	} else {
		error(SYNTAX_ERR);
	}

	getsym();

	if (tok.attr == SYMBOL && (tok.value == PLUS || tok.value == MINUS || tok.value == TIMES)) {
		gen_push("r2"); //r2を計算に使うので退避

		switch (tok.value) {
			case PLUS	:
				getsym();
				load_number("r2");
				gen_addr("r1", "r2");
				break;
			
			case MINUS	:
				getsym();
				load_number("r2");
				gen_subr("r1", "r2");
				break;
			
			case TIMES	:
				getsym();
				load_number("r2");
				gen_mulr("r1", "r2");
				break;
		}

		getsym();
		gen_pop("r2"); //復帰

	} else if (tok.attr == RWORD && tok.value == DIV) {
		gen_push("r2");

		getsym();
		load_number("r2");

		gen_divr("r1", "r2");

		getsym();
		gen_pop("r2");
	}
}

/*条件式が偽であれば引数の番号のラベルに飛ぶ*/
void condition(int label) {
	grammer = "condition";

	expression();
	gen_loadr("r2", "r1");

	if (tok.attr == SYMBOL) {
		switch (tok.value) {
			case EQL		: 
				getsym();
				expression();

				gen_cmpr("r2", "r1");
				gen_jnz(label);
				break;
			case NOTEQL		:
				getsym();
				expression();

				gen_cmpr("r2", "r1");
				gen_jz(label);
				break;
			case LESSTHAN	:
				getsym();
				expression();

				gen_cmpr("r2", "r1");
				gen_jge(label);
				break;
			case GRTRTHAN	:
				getsym();
				expression();

				gen_cmpr("r2", "r1");
				gen_jle(label);
				break;
			case LESSEQL	:
				getsym();
				expression();

				gen_cmpr("r2", "r1");
				gen_jgt(label);
				break;
			case GRTREQL	:
				getsym();
				expression();

				gen_cmpr("r2", "r1");
				gen_jlt(label);
		}	
	}
}