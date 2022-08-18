//////////////////////////////////////////////////////EBNF PARA PL0//////////////////////////////////////////////////////
/*
* EBNF -> 
* .pl0 -- PL/0 compiler.
*
* program		= block "." .
* block			= [ "const" ident "=" number { "," ident "=" number } ";" ]
*		  		[ "var" ident [ array ] { "," ident [ array ] } ";" ]
*		  		{ "forward" ident ";" }
*		  		{ "procedure" ident ";" block ";" } statement .
* statement		= [ ident ":=" expression
*		 		 | "call" ident
*		 		 | "begin" statement { ";" statement } "end"
*		 		 | "if" condition "then" statement [ "else" statement ]
*		 		 | "while" condition "do" statement
*		 		 | "readInt" [ "into" ] ident
*		 		 | "readChar" [ "into" ] ident
*		 		 | "writeInt" expression
*		 		 | "writeChar" expression
*		 		 | "writeStr" ( ident | string )
*		 		 | "exit" expression ] .
* condition		= "odd" expression
*		  		| expression ( comparator ) expression .
* expression	= [ "+" | "-" | "not" ] term { ( "+" | "-" | "or" ) term } .
* term			= factor { ( "*" | "/" | "mod" | "and" ) factor } .
* factor		= ident
*		 		 | number
*		 		 | "(" expression ")" .
* comparator	= "=" | "#" | "<" | ">" | "<=" | ">=" | "<>" .
* array			= "size" number .
*
*/
///////////////////////////////////////////////////////BIBLIOTECAS///////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

/////////////////////////////////////////////////////////DEFINES/////////////////////////////////////////////////////////
#define TOK_IDENT			'I'
#define TOK_NUMBER			'N'
#define TOK_CONST			'C'
#define TOK_VAR				'V'
#define TOK_PROCEDURE		'P'
#define TOK_CALL			'c'
#define TOK_BEGIN			'B'
#define TOK_END				'E'
#define TOK_IF				'i'
#define TOK_THEN			'T'
#define TOK_ELSE			'e'
#define TOK_WHILE			'W'
#define TOK_DO				'D'
#define TOK_ODD				'O'
#define TOK_WRITEINT		'w'
#define TOK_WRITECHAR		'H'
#define TOK_WRITESTR		'S'
#define TOK_READINT			'R'
#define TOK_READCHAR		'h'
#define TOK_INTO			'n'
#define TOK_SIZE			's'
#define TOK_EXIT			'X'
#define TOK_AND				'&'
#define TOK_OR				'|'
#define TOK_NOT				'~'
#define TOK_FORWARD			'F'
#define TOK_DOT				'.'
#define TOK_EQUAL			'='
#define TOK_COMMA			','
#define TOK_SEMICOLON		';'
#define TOK_ASSIGN			':'			// assign is actually a combination of 2 character (':='), but colon is unused
#define TOK_HASH			'#'
#define TOK_LESSTHAN		'<'
#define TOK_GREATERTHAN		'>'
#define TOK_LTEQUALS		'{'
#define TOK_GTEQUALS		'}'
#define TOK_PLUS			'+'
#define TOK_MINUS			'-'
#define TOK_MULT			'*'
#define TOK_DIV				'/'
#define TOK_MODULO			'%'
#define TOK_PARENTESIS_L	'('
#define TOK_PARENTESIS_R	')'
#define TOK_BRACKET_L		'['
#define TOK_BRACKET_R		']'
#define TOK_STRING			'"'
#define LEFT				100
#define RIGTH				101
#define CALL				102

///////////////////////////////////////////////////////ESTRUCTURAS///////////////////////////////////////////////////////
typedef struct symtab {
	int depth;
	int type;
	char *name;
	struct symtab *next;
};

////////////////////////////////////////////////////////VARIABLES////////////////////////////////////////////////////////
static char *raw;			// for raw source code
static size_t line = 1;		// line counter (starts at 1)
static char *token;			// for lexing, the token that lexer reads from source code
static int type;			// number corresponding to token
static int depth = 0;		// for var ident's priority
symtab *head;				// symbol table for identifiers and numbers
bool isProcedure = false;	// to know if we are inside a procedure or just the main

/////////////////////////////////////////////////PROTOTIPOS DE FUNCIONES/////////////////////////////////////////////////
static void error(const char*, ...);	// error handling routine. Prints the error and gives up on compiling
static void readin(char*/*raw*/);		// reads the file and calls error if needed. Puts source code in raw.
//====LEXER
static void comment();					// skips comments withing wource code.
static int ident();						// gets identifier or reserved word ant returns it token.
static int number();					// gets number and returns token, error if number is invalid.
bool isNumber(char*);					// checks if given string is a valid number
static int string();					// gets string and returns token ASD NOT IMPLEMENTED YET
static int lex();						// returns token read in source code. error if invalid

//====PARSER
static void next();						// fetchs the next token
static void expect(int/*expected*/);	// returns error if token if diff from expected 
static void parse();					// checks syntax of input code
static void block();					// process the block section in EBNF
static void statement();				// process the statement section in EBNF
static void expression();				// process the expression section in EBNF
static void condition();				// process the condition section in EBNF
static void comparator();				// process the comparator section in EBNF
static void term();						// process the term section in EBNF
static void factor();					// process the factor section in EBNF

//====CODE GENERATOR
static void out(const char*, ...);			// writes the output code
static void initSymtab();					// initialize the symbol table
static void addsymbol();					// adds symbol to symbol table
static void destroysymbols();				// destroy symbol from symbol table
static void cgConst();						// writes the code for const in c
static void cgSymbol();						// writes the code for the different symbols in c
static void cgSemicolon();					// writes the code for semicolon in c
static void cgEnd();						// writes the end of the compiler messages
static void cgVar();						// writes the code for var in c
static void cgProcedure();					// writes the code for a procedure in c
static void cgEndOfProgram();				// writes the code for the end of the program in c
static void assignmentCheck(int/*check*/);	// checks whether an assignment is valid or not (semantic)
static void cgCall();						// writes the code for call in c
static void cgReadint();					// writes the code for readInt in c
static void cgReadchar();					// writes the code for readChar in c
static void cgWriteint();					// writes the code for writeInt in c
static void cgParenR();						// writes the code for a rigth parentesis in c
static void cgWritechar();					// writes the code for writeChar in c
static void cgWritestr();					// writes the code for writeStr in c
static void cgExit();						// writes the code for exit in c
static void cgOdd();						// writes the code for odd in c

//////////////////////////////////////////////////////////Main///////////////////////////////////////////////////////////
int main(int argc, char *argv[]){

	if (argc != 2) {
		(void) fputs("Usage: pl0_compiler file.pl0\n", stderr);
		exit(1);
	}

	readin(argv[1]);

	initSymtab();

	parse();

	free(raw);

	return 0;
}

////////////////////////////////////////////////////////FUNCIONES////////////////////////////////////////////////////////
static void error(const char *format, ...){
	/*

	***** va -> variable arguments
	https://www.gnu.org/software/libc/manual/html_node/Argument-Macros.html#Argument-Macros

	va_list: The type va_list is used for argument pointer variables.

	***PROTOTYPE:
	va_start(va_list ap, last-required)	

	***DESCRIPTION:
		Initializes the argument pointer variable ap to point to the first 
		of the optional arguments of the current function.

	***PROTOTYPE:
	va_arg (va_list ap, type)

	***DESCRIPTION:
		Returns the value of the next optional argument,
		and modifies the value of ap to point to the subsequent argument.
		type must be a self-promoting type (not 'char' or 'short int' or 'float')
		that matches the type of the actual argument.

	***PROTOTYPE:
	va_end (va_list ap)

	***DESCRIPTION:
	Ends the use of ap. After a va_end call, further va_arg calls with the same ap may not work.
	
	*/

	va_list args;

	//as soon as we encounter an error, no matter what it might be,
	//we will report it to the user and then give up on the compilation.
	(void) fprintf(stderr, "pl0_compiler error in line: %lu: ", line);

	va_start(args, format);
	(void) vfprintf(stderr, format, args);
	va_end(args);

	(void) fputc('\n', stderr);

	exit(1);
}

static void readin(char *file){
	int fildes, end;
	struct stat st;

	//Explains all functions
	//https://www.geeksforgeeks.org/input-output-system-calls-c-create-open-close-read-write/#:~:text=What%20is%20the%20File%20Descriptor,pointers%20to%20file%20table%20entries.

	/*
	*****fstat function:
	https://pubs.opengroup.org/onlinepubs/009696699/functions/fstat.html

	***PROTOTYPE:
		fstat(int fildes, struct stat *buf) 

	***DESCRIPTION:
		The function shall obtain information about an open file 
		associated with the file descriptor fildes, and shall write it to the area pointed to by buf.

	***RETURNS:
		Upon successful completion, 0 shall be returned.
		Otherwise, -1 shall be returned and errno set to indicate the error.

	*****Stat Structure:
	https://pubs.opengroup.org/onlinepubs/009696699/basedefs/sys/stat.h.html
	
	Where information is placed concerning the file

		stat structure: off_t	st_size		For regular files, the file size in bytes.
	*/

	if (strrchr(file, '.') == NULL) 					error("File must end in '.pl0'.");

	if (!!strcmp(strrchr(file, '.'), ".pl0"))			error("File must end in '.pl0'.");

	if ((fildes = open(file, O_RDONLY)) == -1)			error("Couldn't open '%s'.", file);

	if (fstat(fildes, &st) == -1)						error("Couldn't get file size.");

	if ((raw = (char*)malloc(st.st_size + 1)) == NULL)	error("Malloc failed.");

	//INTERNET OPTION (doesn't work)
	//if (read(fildes, raw, st.st_size) != st.st_size)	error("Couldn't read '%s'.", file);
	//raw[st.st_size] = '\0';


	if ((end = read(fildes, raw, st.st_size)) == -1)	error("Couldn't read '%s'.", file);

	raw[end] = '\0';

	(void) close(fildes);
}


//=======================================================================================================================
//========================================================Lexer==========================================================
//=======================================================================================================================
static void comment(){						// comments with format: {...}
	int ch;

	// skip everithing until '}'
	while ((ch = *raw++) != '}') {
		if (ch == '\0') error("Unterminated comment.");
		if (ch == '\n') line++;
	}
}

// since both, ident and reserved words can start with lowercase letters there is no way to distinguish
// so reserved words are handled in the ident function
static int ident(){
	
	char *p;
	size_t i, len;

	p = raw;

	// check if its alphanumeric
	while (isalnum(*raw) || *raw == '_')	raw++;

	// getting the length of the next token
	len = raw - p;

	raw--;

	free(token);

	if ((token = (char*)malloc(len + 1)) == NULL)	error("Token malloc for identifier failed.");

	// getting token form raw source code
	for (i = 0; i < len; i++)	token[i] = *p++;

	token[i] = '\0';

	// return corresponding token
	if (!strcmp(token, "const")) 			return TOK_CONST;
	else if (!strcmp(token, "var"))			return TOK_VAR;
	else if (!strcmp(token, "procedure"))	return TOK_PROCEDURE;
	else if (!strcmp(token, "call"))		return TOK_CALL;
	else if (!strcmp(token, "begin"))		return TOK_BEGIN;
	else if (!strcmp(token, "end"))			return TOK_END;
	else if (!strcmp(token, "if"))			return TOK_IF;
	else if (!strcmp(token, "then"))		return TOK_THEN;
	else if (!strcmp(token, "while"))		return TOK_WHILE;
	else if (!strcmp(token, "do"))			return TOK_DO;
	else if (!strcmp(token, "odd"))			return TOK_ODD;
	else if (!strcmp(token, "writeInt"))	return TOK_WRITEINT;
	else if (!strcmp(token, "writeChar"))	return TOK_WRITECHAR;
	else if (!strcmp(token, "writeStr"))	return TOK_WRITESTR;
	else if (!strcmp(token, "readInt"))		return TOK_READINT;
	else if (!strcmp(token, "readChar"))	return TOK_READCHAR;
	else if (!strcmp(token, "into"))		return TOK_INTO;
	else if (!strcmp(token, "size"))		return TOK_SIZE;
	else if (!strcmp(token, "exit"))		return TOK_EXIT;
	else if (!strcmp(token, "and"))			return TOK_AND;
	else if (!strcmp(token, "or"))			return TOK_OR;
	else if (!strcmp(token, "not"))			return TOK_NOT;
	else if (!strcmp(token, "mod"))			return TOK_MODULO;
	else if (!strcmp(token, "forward"))		return TOK_FORWARD;

	return TOK_IDENT;
}

static int number(){

	char *p;
	size_t i, j = 0, len;

	p = raw;
	
	// check if its a digit
	while (isdigit(*raw) || *raw == '_')	raw++;
	
	// getting the length of the next token
	len = raw - p;

	raw--;

	free(token);

	if ((token = (char*)malloc(len + 1)) == NULL)	error("Token malloc for number failed.");

	// getting token form raw source code
	for (i = 0; i < len; i++)	
		if (isdigit(*p)) token[j++] = *p;
		p++;

	token[j] = '\0';

	/*
	***** strtonum:
	https://man.openbsd.org/strtonum.3

	***PROTOTYPE:
	strtonum(const char *nptr, long long minval, long long maxval, const char **errstr)

	***DESCRIPTION:
	The strtonum() function converts the string in nptr to a long long value.

	The value obtained is then checked against the provided minval and maxval bounds.
	If errstr is non-null, strtonum() stores an error string in *errstr indicating the failure.

	***RETURNS:
	The strtonum() function returns the result of the conversion,
		unless the value would exceed the provided bounds or is invalid.
	On error, 0 is returned, errno is set, and errstr will point to an error message.
	*errstr will be set to NULL on success
	
	*/

	// it returns a long long, but its used as a number validator, so type doesn't matter. Cast to void
	//(void) strtonum(token, 0, LONG_MAX, &errstr);
	//if (errstr != NULL)	error("Invalid number: %s.", token);

	if (!isNumber(token))	error("Invalid number: %s.", token);

	return TOK_NUMBER;
}

bool isNumber(char *n){
	
	int length = strlen(n);
	
	for (int i = 0; i < length; i++){
		if (n[i] < '0' || n[i] > '9')	return false;
	}

	return true;
}

static int lex(){
	// labeled statement for goto
	again:
	
	// Skip whitespace
	while (*raw == ' ' || *raw == '\t' || *raw == '\n')
		if (*raw++ == '\n') line++;

	//if its a letter, get identifier
	if (isalpha(*raw) || *raw == '_')	return ident();

	// if its a digit, get number
	if (isdigit(*raw))	return number();

	// else, get the symbol
	switch (*raw) {
		case '{':
			comment();
			goto again;

		case '.':
		case '=':
		case ',':
		case ';':
		case '#':
		case '+':
		case '-':
		case '*':
		case '/':
		case '%':
		case '(':
		case ')':
		case '[':
		case ']':
			return (*raw);

		case '<':
			if (*++raw == '=') return TOK_LTEQUALS;
			if (*raw++ == '>') return TOK_HASH;
			raw--;
			return TOK_LESSTHAN;

		case '>':
			if (*++raw == '=') return TOK_GTEQUALS;
			raw--;
			return TOK_GREATERTHAN;

		case ':':
			if (*++raw != '=')	error("Unknown token after ':' -> '%c'.", *raw);
			return TOK_ASSIGN;

		case '\0':
			return 0;

		default:
			error("Unknown token: '%c'.", *raw);
	}

	return 0;
}

//=======================================================================================================================
//=======================================================Parser==========================================================
//=======================================================================================================================
static void next() {
	type = lex();
	raw++;
}

static void expect(int expected){
	if (expected != type)	error("Syntax error");
	next();
}

static void parse() {
	next();					// get first token in type
	block();				// process acording to EBNF
	expect(TOK_DOT);		// end of program

	if (type != 0)	error("Extra tokens at the end of file.");

	cgEnd();
}

static void block(){

	// no nested procedures
	if (depth++ > 1)	error("Nesting depth exceeded.");
	
	//[ "const" ident "=" number { "," ident "=" number } ";" ]
	if (type == TOK_CONST){
		expect(TOK_CONST);

		if (type == TOK_IDENT){
			addsymbol(TOK_CONST);
			cgConst();
		}

		expect(TOK_IDENT);
		expect(TOK_EQUAL);

		if (type == TOK_NUMBER) {
			cgSymbol();
			cgSemicolon();
		}

		expect(TOK_NUMBER);
		
		while(type == TOK_COMMA){
			expect(TOK_COMMA);

			if (type == TOK_IDENT){
				addsymbol(TOK_CONST);
				cgConst();
			}

			expect(TOK_IDENT);
			expect(TOK_EQUAL);

			if (type == TOK_NUMBER) {
				cgSymbol();
				cgSemicolon();
			}

			expect(TOK_NUMBER);
		}
		
		expect(TOK_SEMICOLON);
	}

	//[ "var" ident [ array ] { "," ident [ array ] } ";" ]
	// ASD NOT DOING ARRAY FOR NOW
	if (type == TOK_VAR){
		expect(TOK_VAR);

		if (type == TOK_IDENT) {
			addsymbol(TOK_VAR);
			cgVar();
		}

		expect(TOK_IDENT);

		if (type == TOK_SIZE){
			expect(TOK_SIZE);
			if (type == TOK_NUMBER){
				//arraySize();	//token has the size
				//cgArray();
			}
			expect(TOK_NUMBER);
		}
		
		while(type == TOK_COMMA){
			expect(TOK_COMMA);

			if (type == TOK_IDENT) {
				addsymbol(TOK_VAR);
				cgVar();
			}

			expect(TOK_IDENT);

			if (type == TOK_SIZE){
				expect(TOK_SIZE);
				if (type == TOK_NUMBER){
					//arraySize();	//token has the size
					//cgArray();
				}
				expect(TOK_NUMBER);
			}
		}
		
		expect(TOK_SEMICOLON);
	}

	//{ "forward" ident ";" }
	while(type == TOK_FORWARD){
		expect(TOK_FORWARD);
		expect(TOK_IDENT);
		expect(TOK_SEMICOLON);
	}

	//{ "procedure" ident ";" block ";" } statement .
	// Allows nested procedures
	while(type == TOK_PROCEDURE){
		
		//we are inside a procedure
		isProcedure = true;

		expect(TOK_PROCEDURE);

		if (type == TOK_IDENT) {
			addsymbol(TOK_PROCEDURE);
			cgProcedure();
		}

		expect(TOK_IDENT);
		expect(TOK_SEMICOLON);
		
		block();
		
		expect(TOK_SEMICOLON);

		//end of procedure
		isProcedure = false;

		//destroy local variables and constants for repeated identifiers
		destroysymbols();
	}
	
	if (!isProcedure)	cgProcedure();

	statement();

	cgEndOfProgram();

	if (--depth < 0)	error("Nesting depth fell below 0.");
}

static void statement(){
	switch(type){
		//	[ ident ":=" expression
		case TOK_IDENT :
			assignmentCheck(LEFT);
			cgSymbol();
			expect(TOK_IDENT);

			if (type == TOK_ASSIGN)	cgSymbol();
			
			expect(TOK_ASSIGN);
			expression();
			break;

		//	| "call" ident
		case TOK_CALL :
			expect(TOK_CALL);
			if (type == TOK_IDENT) {
				assignmentCheck(CALL);
				cgCall();
			}
			expect(TOK_IDENT);
			break;

		// | "begin" statement { ";" statement } "end"
		case TOK_BEGIN :
			cgSymbol();
			expect(TOK_BEGIN);
			statement();
			
			while(type == TOK_SEMICOLON){
				cgSemicolon();
				expect(TOK_SEMICOLON);
				statement();
			}

			if (type == TOK_END)	cgSymbol();
			expect(TOK_END);
			break;

		//	| "if" condition "then" statement [ "else" statement ]
		case TOK_IF :
			cgSymbol();
			expect(TOK_IF);
			condition();
			
			if (type == TOK_THEN)	cgSymbol();
			
			expect(TOK_THEN);
			statement();
			
			if (type == TOK_ELSE){
				cgSymbol();
				expect(TOK_ELSE);
				statement();
			}
			
			break;

		//	| "while" condition "do" statement
		case TOK_WHILE :
			cgSymbol();
			expect(TOK_WHILE);
			condition();

			if (type == TOK_DO)	cgSymbol();

			expect(TOK_DO);
			statement();
			break;

		//	| "readInt" [ "into" ] ident
		case TOK_READINT :
			expect(TOK_READINT);
			
			if (type == TOK_INTO){
				expect(TOK_INTO);
			}
			
			if (type == TOK_IDENT) {
				assignmentCheck(LEFT);
				cgReadint();
			}

			expect(TOK_IDENT);
			break;

		//	| "readChar" [ "into" ] ident
		case TOK_READCHAR :
			expect(TOK_READCHAR);
			
			if (type == TOK_INTO){
				expect(TOK_INTO);
			}
			
			if (type == TOK_IDENT) {
				assignmentCheck(LEFT);
				cgReadchar();
			}

			expect(TOK_IDENT);
			break;

		//	| "writeInt" expression
		case TOK_WRITEINT :
			expect(TOK_WRITEINT);
			cgWriteint();
			expression();
			cgParenR();
			cgSemicolon();
			break;

		//	| "writeChar" expression
		case TOK_WRITECHAR :
			expect(TOK_WRITECHAR);
			cgWritechar();
			expression();
			cgParenR();
			cgSemicolon();
			break;

		//	| "writeStr" ( ident | string )
		case TOK_WRITESTR :
			expect(TOK_WRITESTR);

			switch(type){
				case TOK_IDENT:	assignmentCheck(LEFT);	cgWritestr();	expect(TOK_IDENT);	break;
				case TOK_STRING: cgWritestr();	expect(TOK_STRING);	break;

				default: error("writeStr takes an array or a string.");
			}
			
			break;

		//	| "exit" expression ]
		case TOK_EXIT :
			expect(TOK_EXIT);
			cgExit();
			expression();
			cgParenR();
			cgSemicolon();
			break;
	}
}


static void condition(){
	//	"odd" expression
	if (type == TOK_ODD){
		cgSymbol();
		expect(TOK_ODD);
		expression();
		cgOdd();
	}
	//	| expression ( comparator ) expression .
	else{
		expression();
		expect(TOK_PARENTESIS_L);
		comparator();
		expect(TOK_PARENTESIS_R);
		expression();
	}
}

//	"=" | "#" | "<" | ">" | "<=" | ">=" | "<>"
static void comparator(){
	switch (type) {
		case TOK_EQUAL:
		case TOK_HASH:
		case TOK_LESSTHAN:
		case TOK_GREATERTHAN:
		case TOK_LTEQUALS:
		case TOK_GTEQUALS:
			cgSymbol();
			next();
			break;
		default:
			error("Invalid conditional.");
	}
}

//	[ "+" | "-" | "not" ] term { ( "+" | "-" | "or" ) term }
static void expression(){
	if (type == TOK_PLUS || type == TOK_MINUS || type == TOK_NOT){
		cgSymbol();
		next();
	}

	term();

	while(type == TOK_PLUS || type == TOK_MINUS || type == TOK_OR){
		cgSymbol();
		next();
		term();
	}
}

//	factor { ( "*" | "/" | "mod" | "and" ) factor }
static void term(){
	factor();
	while(type == TOK_MULT || type == TOK_DIV || type == TOK_MODULO || type == TOK_AND){
		cgSymbol();
		next();
		factor();
	}
}

static void factor(){
	switch(type){
		//	ident
		case TOK_IDENT:
			assignmentCheck(RIGTH);
		//	number
		case TOK_NUMBER:
			cgSymbol();
			next();
			break;
		//	"(" expression ")"
		case TOK_PARENTESIS_L:
			cgSymbol();
			expect(TOK_PARENTESIS_L);
			expression();
			if (type == TOK_RPAREN)	cgSymbol();
			expect(TOK_PARENTESIS_R);
			break;

		default: error("Unexpected token in factor.");
	}
}

//=======================================================================================================================
//===================================================Code Generator======================================================
//=======================================================================================================================
static void out(const char *format, ...){
	va_list ap;

	va_start(ap, format);
	(void) vfprintf(stdout, format, ap);
	va_end(ap);
}


static void cgEnd(){
	out("---PL/0 compiler: Compile success.---");
}

static void cgConst(){
	out("const long %s = ", token);
}

static void cgSymbol(){
	switch (type){
		case TOK_IDENT:
		case TOK_NUMBER:
			out("%s", token);
			break;
		case TOK_BEGIN:			out("{\n");		break;
		case TOK_END:			out(";\n}\n");	break;
		case TOK_IF:			out("if(");		break;
		case TOK_THEN:			out("){\n");	break;
		case TOK_DO:			out(")");		break;
		case TOK_ELSE:			out(";}else{");	break;
		case TOK_ODD:			out("(");		break;
		case TOK_WHILE:			out("while(");	break;
		case TOK_EQUAL:			out("==");		break;
		case TOK_COMMA:			out(",");		break;
		case TOK_ASSIGN:		out("=");		break;
		case TOK_HASH:			out("!=");		break;
		case TOK_LESSTHAN:		out("<");		break;
		case TOK_GREATERTHAN:	out(">");		break;
		case TOK_GTEQUALS:		out(">=");		break;
		case TOK_LTEQUALS:		out("<=");		break;
		case TOK_AND:			out("&");		break;
		case TOK_OR:			out("|");		break;
		case TOK_NOT:			out("~");		break;
		case TOK_PLUS:			out("+");		break;
		case TOK_MINUS:			out("-");		break;
		case TOK_MULT:			out("*");		break;
		case TOK_DIV:			out("/");		break;
		case TOK_MODULO:		out("%");		break;
		case TOK_PARENTESIS_L:	out("(");		break;
		case TOK_PARENTESIS_R:	out(")");		break;
		case TOK_BRACKET_L:		out("[");		break;
		case TOK_BRACKET_R:		out("]");		break;
		case TOK_STRING:		out("\"");		break;
	}
}

static void cgSemicolon(){
	out(";\n");
}

/*
Symbol table so that you can't repeat var (or const) names/ident's.

					before/during procedure            after procedure
+---------------+ +---------------+ +---------------+ +---------------+
| Sentinel      | | Sentinel      | | Sentinel      | | Sentinel      |
+---------------+ +---------------+ +---------------+ +---------------+
                  | Global consts | | Global consts | | Global consts |
                  +---------------+ +---------------+ +---------------+
                  | Global vars   | | Global vars   | | Global vars   |
                  +---------------+ +---------------+ +---------------+
                                    | Procedure     | | Procedure     |
                                    +---------------+ +---------------+
                                    | Local consts  |
                                    +---------------+
                                    | Local vars    |
                                    +---------------+
Struct:
struct symtab {
	int depth;
	int type;
	char *name;
	struct symtab *next;
};

*/
static void initSymtab(){
	symtab *new;

	if ((new = malloc(sizeof(symtab))) == NULL)	error("Malloc for first symtab failed.");

	// Sentinel
	new->depth = 0;
	new->type = TOK_PROCEDURE;
	new->name = "main";
	new->next = NULL;

	head = new;
}

static void addsymbol(int type){
	symtab *curr, *new;

	curr = head;

	while (true) {
		if (!strcmp(curr->name, token)){
			if (curr->depth == (depth - 1))	error("Duplicate symbol: '%s'.", token);
		}

		if (curr->next == NULL)	break;

		curr = curr->next;
	}

	if ((new = malloc(sizeof(struct symtab))) == NULL)	error("Malloc for new symtab failed.");

	//new->depth = curr->depth + 1;	???? ASD
	new->depth = depth - 1;

	new->type = type;

	if ((new->name = strdup(token)) == NULL)	error("Malloc for new symtab name failed.");
	
	new->next = NULL;

	curr->next = new;
}

static void destroysymbols() {
	symtab *curr, *aux;
	curr = head;

	// my version (should work, not tested ASD)
	while(curr->next != NULL){
		if (curr->type == TOK_PROCEDURE && curr->next->type != TOK_PROCEDURE){
			aux = curr->next;
			curr->next = aux->next;
			free(aux->name);
			free(aux);
		}
		else{
			curr = curr->next;
		}
	}
/*
//================================INTERNET VERSION
again:
	curr = head;
	while (curr->next != NULL) {
		prev = curr;
		curr = curr->next;
	}

	if (curr->type != TOK_PROCEDURE) {
		free(curr->name);
		free(curr);
		prev->next = NULL;
		goto again;
	}
*/

}

static void cgVar(){
	out("long %s;\n", token);
}

static void cgProcedure(){
	if (!isProcedure){
		out("int main(int argc, char *argv[])");
	}
	else {
		out("void %s()", token);
	}
	out("{\n");
}

static void cgEndOfProgram(){
	cgSemicolon();
	if (!isProcedure)	out("return 0;\n");
	out("}");
}

static void assignmentCheck(int check){
	symtab *aux, *aux2 = NULL;

	aux = head;
	while(aux != NULL){
		if (!strcmp(token, aux->name))	aux2 = aux;
		aux = aux->next;
	}

	if (aux2 == NULL)	error("Undefined symbol: '%s'.", token);

	switch(check){
		case LEFT:
			if (aux2->type != TOK_VAR)	error("Must be a variable: '%s'.", aux2->name);

		case RIGTH:
			if (aux2->type == TOK_PROCEDURE) error("Can't be a procedure: '%s'", aux2->name);

		case CALL:
			if (aux2->type != TOK_PROCEDURE) error("Must be a procedure: '%s'", aux2->name);
	}
}

static void cgCall(){
	out("%s();\n",token);
}

static void cgReadint(){
	out("fgets(__stdin, sizeof(__stdin), stdin);\n");
	out("if(__stdin[strlen(__stdin) - 1] == '\\n')\t__stdin[strlen(__stdin) - 1] = '\\0';");
	out("%s = (long) atol(__stdin);\n", token);	//returns 0 upon failure
	out("if (%s == 0) fprintf(stderr, \"Invalid number: %%s\\n\", __stdin);\n", token);
	out("exit(1);");
}

static void cgReadchar(){
	out("%s = fgetc(stdin);\n",token);
}

static void cgWriteint(){
	//there's an expression after the writeInt so open paren
	out("fprintf(stdout, \"%%ld\", (long)(");
}

static void cgParenR(){
	//for closing the write instructions
	out(")");
}

static void cgWritechar(){
	out("fprintf(stdout, \" %%c \", (char)(");
}

static void cgWritestr(){
	//array given
	if (type == TOK_IDENT) {
		//ASD not doing arrays yet
	}
	//string given
	else {
		out("fprintf(stdout, %s);\n", token);
	}
}

static void cgExit(){
	out("exit(");
}

static void cgOdd(){
	//and with 1
	out(")%1");
}
///////////////////////////////////////////////////////////FIN///////////////////////////////////////////////////////////