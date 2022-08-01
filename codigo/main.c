//////////////////////////////////////////////////////EBNF PARA PL0//////////////////////////////////////////////////////
/*
* EBNF -> 
* .pl0
*
* program		= block "." .
* block			= [ "const" ident "=" number { "," ident "=" number } ";" ]
*		  		[ "var" ident { "," ident } ";" ]
*		  		{ "procedure" ident ";" block ";" } statement .
* statement		= [ ident ":=" expression
*		  		| "call" ident
*		  		| "begin" statement { ";" statement } "end"
*		  		| "if" condition "then" statement
*		  		| "while" condition "do" statement ] .
* condition		= "odd" expression
*				| expression ( "=" | "#" | "<" | ">" ) expression .
* expression	= [ "+" | "-" ] term { ( "+" | "-" ) term } .
* term			= factor { ( "*" | "/" ) factor } .
* factor		= ident
*				| number
*				| "(" expression ")" .
* ident			= "A-Za-z_" { "A-Za-z0-9_" } .
* number		= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" .
*
*/
///////////////////////////////////////////////////////BIBLIOTECAS///////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
/////////////////////////////////////////////////////////DEFINES/////////////////////////////////////////////////////////
/*
	The reserved words are: const, var, procedure, call, begin, end, if, then, while, do, and odd.
	The symbols are '.', '=', ',', ';', ':=', '#', '<', '>', '+', '-', '*', '/', '(', and ')'.
	There are also the identifiers and numbers.
*/
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
#define TOK_WHILE			'W'
#define TOK_DO				'D'
#define TOK_ODD				'O'
#define TOK_DOT				'.'
#define TOK_EQUAL			'='
#define TOK_COMMA			','
#define TOK_SEMICOLON		';'
#define TOK_ASSIGN			':'		// assign is actually a combination of 2 character (':='), but colon is unused
#define TOK_HASH			'#'
#define TOK_LESSTHAN		'<'
#define TOK_GREATERTHAN		'>'
#define TOK_PLUS			'+'
#define TOK_MINUS			'-'
#define TOK_MULT			'*'
#define TOK_DIV				'/'
#define TOK_PARENTESIS_L	'('
#define TOK_PARENTESIS_R	')'

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
static void ident();					// gets identifier or reserved word ant returns it token.
static void numbers();					// gets number and returns token, error if number is invalid.
static void lex();						// returns token read in source code. error if invalid
//////////////////////////////////////////////////////////Main///////////////////////////////////////////////////////////
int main(int argc, char *argv[]){

	char *startp;

	if (argc != 2) {
		(void) fputs("usage: pl0c file.pl0\n", stderr);
		exit(1);
	}

	readin(argv[1]);
	startp = raw;

	parse();

	free(startp);

	return 0;
}
////////////////////////////////////////////////////////FUNCIONES////////////////////////////////////////////////////////
static void error(const char *format, ...)
{
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
	(void) fprintf(stderr, "pl0c: error: %lu: ", line);

	va_start(args, format);
	(void) vfprintf(stderr, format, args);
	va_end(args);

	(void) fputc('\n', stderr);

	exit(1);
}

static void readin(char *file)
{
	int fildes;
	struct stat st;

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
											For symbolic links, the length in bytes of the 
												pathname contained in the symbolic link.
	*/

	if (strrchr(file, '.') == NULL) 					error("File must end in '.pl0'.");

	if (!!strcmp(strrchr(file, '.'), ".pl0"))			error("File must end in '.pl0'.");

	if ((fildes = open(file, O_RDONLY)) == -1)			error("Couldn't open %s.", file);

	if (fstat(fildes, &st) == -1)						error("Couldn't get file size.");

	if ((raw = malloc(st.st_size + 1)) == NULL)			error("Malloc failed.");

	if (read(fildes, raw, st.st_size) != st.st_size)	error("Couldn't read %s.", file);
	
	raw[st.st_size] = '\0';

	(void) close(fildes);
}

static void comment()						// comments with format: {...}
{
	int ch;

	// skip everithing until '}'
	while ((ch = *raw++) != '}') {
		if (ch == '\0') error("Unterminated comment.");
		if (ch == '\n') line++;
	}
}

// since both, ident and reserved words can start with lowercase letters there is no way to distinguish
// so reserved words are handled in the ident function
static void ident(){
	
	char *p;
	size_t i, len;

	p = raw;

	// check if its alphanumeric
	while (isalnum(*raw) || *raw == '_')	raw++;

	// getting the length of the next token
	len = raw - p;

	raw--;

	free(token);

	if ((token = malloc(len + 1)) == NULL)	error("Token malloc for identifier failed.");

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

	return TOK_IDENT;
}

static void number(){

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

	if ((token = malloc(len + 1)) == NULL)	error("Token malloc for number failed.");

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

	(void) strtonum(token, 0, LONG_MAX, &errstr);

	if (errstr != NULL)	error("Invalid number: %s.", token);

	return TOK_NUMBER;
}

static int lex()
{
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
	case '<':
	case '>':
	case '+':
	case '-':
	case '*':
	case '/':
	case '(':
	case ')':
		return (*raw);
	case ':':
		if (*++raw != '=')	error("Unknown token: '%c'.", *raw);
		return TOK_ASSIGN;
	case '\0':
		return 0;
	default:
		error("Unknown token: '%c'.", *raw);
	}

	return 0;
}


///////////////////////////////////////////////////////////FIN///////////////////////////////////////////////////////////