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

////////////////////////////////////////////////////////VARIABLES////////////////////////////////////////////////////////
char *raw;					// for raw source code
static size_t line = 1;		// line counter (starts at 1)
///////////////////////////////////////////////////////ESTRUCTURAS///////////////////////////////////////////////////////

/////////////////////////////////////////////////PROTOTIPOS DE FUNCIONES/////////////////////////////////////////////////
static void error(const char*, ...);		//  error handling routine
static void readin(char*);

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

	va_list: The type va_list is used for argument pointer variables.

	va_start(va_list ap, last-required):	Initializes the argument pointer variable ap to point to the first 
												of the optional arguments of the current function.

	va_arg (va_list ap, type):	Returns the value of the next optional argument,
									and modifies the value of ap to point to the subsequent argument.
								type must be a self-promoting type (not 'char' or 'short int' or 'float')
									that matches the type of the actual argument.

	va_end (va_list ap):	Ends the use of ap. After a va_end call, further va_arg calls with the same ap may not work.
	
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
		The fstat(int fildes, struct stat *buf) function shall obtain information about an open file 
		associated with the file descriptor fildes, and shall write it to the area pointed to by buf.

	Upon successful completion, 0 shall be returned. Otherwise, -1 shall be returned and errno set to indicate the error.

	*****Stat Structure:
	https://pubs.opengroup.org/onlinepubs/009696699/basedefs/sys/stat.h.html
	
	Where information is placed concerning the file

		stat structure: off_t	st_size		For regular files, the file size in bytes. 
											For symbolic links, the length in bytes of the 
												pathname contained in the symbolic link.
	*/

	if (strrchr(file, '.') == NULL) 					error("File must end in '.pl0'");

	if (!!strcmp(strrchr(file, '.'), ".pl0"))			error("File must end in '.pl0'");

	if ((fildes = open(file, O_RDONLY)) == -1)			error("Couldn't open %s", file);

	if (fstat(fildes, &st) == -1)						error("Couldn't get file size");

	if ((raw = malloc(st.st_size + 1)) == NULL)			error("Malloc failed");

	if (read(fildes, raw, st.st_size) != st.st_size)	error("Couldn't read %s", file);
	
	raw[st.st_size] = '\0';

	(void) close(fildes);
}
///////////////////////////////////////////////////////////FIN///////////////////////////////////////////////////////////