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

////////////////////////////////////////////////////////VARIABLES////////////////////////////////////////////////////////
static char *raw;			// for raw source code
static size_t line = 1;		// line counter (starts at 1)
static char *token;			// for lexing, the token that lexer reads from source code
static int type;			// number corresponding to token
///////////////////////////////////////////////////////ESTRUCTURAS///////////////////////////////////////////////////////

/////////////////////////////////////////////////PROTOTIPOS DE FUNCIONES/////////////////////////////////////////////////
static void error(const char*, ...);	// error handling routine. Prints the error and gives up on compiling
static void readin(char*/*raw*/);		// reads the file and calls error if needed. Puts source code in raw.
static void comment();					// skips comments withing wource code.
static int ident();						// gets identifier or reserved word ant returns it token.
static int number();					// gets number and returns token, error if number is invalid.
bool isNumber(char*);					// checks if given string is a valid number
static int string();					// gets string and returns token ASD NOT IMPLEMENTED YET
static int lex();						// returns token read in source code. error if invalid
static void next();						// fetchs the next token
static void expect(int/*expected*/);	// returns error if token if diff from expected 
static void parse();
static void block();					// process the block section in EBNF
static void statement();				// process the statement section in EBNF
static void expression();				// process the expression section in EBNF
static void condition();				// process the condition section in EBNF
static void comparator();				// process the comparator section in EBNF
static void term();						// process the term section in EBNF
static void factor();					// process the factor section in EBNF
//////////////////////////////////////////////////////////Main///////////////////////////////////////////////////////////
int main(int argc, char *argv[]){

	if (argc != 2) {
		(void) fputs("Usage: pl0_compiler file.pl0\n", stderr);
		exit(1);
	}

	readin(argv[1]);

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

	const char *errstr;
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

	// getting token form raw source code 										ASD p++ en otra linea?
	for (i = 0; i < len; i++)	
		if (isdigit(*p)) token[j++] = *p++;

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
			return TOK_LESSTHAN;

		case '>':
			if (*++raw == '=') return TOK_GTEQUALS;
			raw++;						//ASD hace falta este??
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
	//[ "const" ident "=" number { "," ident "=" number } ";" ]
	if (type == TOK_CONST){
		expect(TOK_CONST);
		expect(TOK_IDENT);
		expect(TOK_EQUAL);
		expect(TOK_NUMBER);
		
		while(type == TOK_COMMA){
			expect(TOK_COMMA);
			expect(TOK_IDENT);
			expect(TOK_EQUAL);
			expect(TOK_NUMBER);
		}
		
		expect(TOK_SEMICOLON);
	}

	//[ "var" ident [ array ] { "," ident [ array ] } ";" ]
	// ASD NOT DOING ARRAY FOR NOW
	if (type == TOK_VAR){
		expect(TOK_VAR);
		expect(TOK_IDENT);
		
		while(type == TOK_COMMA){
			expect(TOK_COMMA);
			expect(TOK_IDENT);
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
		expect(TOK_PROCEDURE);
		expect(TOK_IDENT);
		expect(TOK_SEMICOLON);
		
		block();
		
		expect(TOK_SEMICOLON);
	}
	statement();
}

static void statement(){
	switch(type){
		//	[ ident ":=" expression
		case TOK_IDENT :
			expect(TOK_IDENT);
			expect(TOK_ASSIGN);
			expression();
			break;

		//	| "call" ident
		case TOK_CALL :
			expect(TOK_CALL);
			expect(TOK_IDENT);
			break;

		// | "begin" statement { ";" statement } "end"
		case TOK_BEGIN :
			expect(TOK_BEGIN);
			statement();
			
			while(type == TOK_SEMICOLON){
				statement();
			}

			expect(TOK_END);
			break;

		//	| "if" condition "then" statement [ "else" statement ]
		case TOK_IF :
			expect(TOK_IF);
			condition();
			expect(TOK_THEN);
			statement();
			
			if (type == TOK_ELSE){
				expect(TOK_ELSE);
				statement();
			}
			
			break;

		//	| "while" condition "do" statement
		case TOK_WHILE :
			expect(TOK_WHILE);
			condition();
			expect(TOK_DO);
			statement();
			break;

		//	| "readInt" [ "into" ] ident
		case TOK_READINT :
			expect(TOK_READINT);
			
			if (type == TOK_INTO){
				expect(TOK_INTO);
			}
			
			expect(TOK_IDENT);
			break;

		//	| "readChar" [ "into" ] ident
		case TOK_READCHAR :
			expect(TOK_READCHAR);
			
			if (type == TOK_INTO){
				expect(TOK_INTO);
			}
			
			expect(TOK_IDENT);
			break;

		//	| "writeInt" expression
		case TOK_WRITEINT :
			expect(TOK_WRITEINT);
			expression();
			break;

		//	| "writeChar" expression
		case TOK_WRITECHAR :
			expect(TOK_WRITECHAR);
			expression();
			break;

		//	| "writeStr" ( ident | string )
		case TOK_WRITESTR :
			expect(TOK_WRITESTR);

			switch(type){
				case TOK_IDENT:
				case TOK_STRING:
					next();
					break;
				default: error("Unexpected token after 'writeStr'.");
			}
			
			break;

		//	| "exit" expression ]
		case TOK_EXIT :
			expect(TOK_EXIT);
			expression();
			break;
	}
}


static void condition(){
	//	"odd" expression
	if (type == TOK_ODD){
		expect(TOK_ODD);
		expression();
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
			// for now its the same if its < or <= since it doent actually compile, it just checks for syntax
			next();
			break;
		default:
			error("Invalid conditional.");
	}
}

//	[ "+" | "-" | "not" ] term { ( "+" | "-" | "or" ) term }
static void expression(){
	if (type == TOK_PLUS || type == TOK_MINUS || type == TOK_NOT) next();

	term();

	while(type == TOK_PLUS || type == TOK_MINUS || type == TOK_NOT){
		next();
		term();
	}
}

//	factor { ( "*" | "/" | "mod" | "and" ) factor }
static void term(){
	factor();
	while(type == TOK_MULT || type == TOK_DIV || type == TOK_MODULO || type == TOK_AND){
		next();
		factor();
	}
}

static void factor(){
	switch(type){
		//	ident
		case TOK_IDENT:
		//	number
		case TOK_NUMBER:
			next();
			break;
		//	"(" expression ")"
		case TOK_PARENTESIS_L:
			expect(TOK_PARENTESIS_L);
			expression();
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
	out("PL/0 compiler: Compile success.");
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
		case TOK_BEGIN:		out("{\n");		break;
		case TOK_END:		out(";\n}\n");	break;
		case TOK_IF:		out("if(");		break;
		case TOK_THEN:
		case TOK_DO:
			out(")");
			break;
		case TOK_ODD:			out("(");		break;
		case TOK_WHILE:			out("while(");	break;
		case TOK_EQUAL:			out("==");		break;
		case TOK_COMMA:			out(",");		break;
		case TOK_ASSIGN:		out("=");		break;
		case TOK_HASH:			out("!=");		break;
		case TOK_LESSTHAN:		out("<");		break;
		case TOK_GREATERTHAN:	out(">")		break;
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
	}
}

static void cgSemicolon(){
	out(";\n");
}
///////////////////////////////////////////////////////////FIN///////////////////////////////////////////////////////////