
#define	TCH_BLANK		1
#define	TCH_OPERATOR	2
#define	TCH_DIGIT		4
#define	TCH_LETTER		8
#define	TCH_HEX			16
#define	TCH_DELIMITER	32
#define	TCH_OCT			64
#define	TCH_ID			128

const uint8_t tchtab[ 256 ] = {

    0,// 0

    0,// 1
    0,// 2
    0,// 3
    0,// 4
    0,// 5
    0,// 6
    0,// 7
    0,// 8

    TCH_BLANK,// 9
    0,// 10

    0,// 11
    0,// 12

    0,// 13

    0,// 14
    0,// 15
    0,// 16
    0,// 17
    0,// 18
    0,// 19
    0,// 20
    0,// 21
    0,// 22
    0,// 23
    0,// 24
    0,// 25
    0,// 26
    0,// 27
    0,// 28
    0,// 29
    0,// 30
    0,// 31

    TCH_BLANK,// 32

    TCH_DELIMITER,// 33 "! "
    TCH_DELIMITER,// 34 """
    TCH_DELIMITER,// 35 "# "
    TCH_DELIMITER,// 36 "$ "
    TCH_DELIMITER | TCH_OPERATOR,// 37 "% "
    TCH_DELIMITER | TCH_OPERATOR,// 38 "& "
    TCH_DELIMITER,// 39 "' "
    TCH_DELIMITER,// 40 "( "
    TCH_DELIMITER,// 41 ") "
    TCH_DELIMITER | TCH_OPERATOR,// 42 "* "
    TCH_DELIMITER | TCH_OPERATOR,// 43 "+ "
    TCH_DELIMITER,// 44 ","
    TCH_DELIMITER | TCH_OPERATOR,// 45 "- "
    TCH_DELIMITER,// 46 ". "
    TCH_DELIMITER | TCH_OPERATOR,// 47 "/ "

    TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,// 48 "0 "
    TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,// 49 "1 "
    TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,// 50 "2 "
    TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,// 51 "3 "
    TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,// 52 "4 "
    TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,// 53 "5 "
    TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,// 54 "6 "
    TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,// 55 "7 "
    TCH_ID | TCH_DIGIT | TCH_HEX,// 56 "8 "
    TCH_ID | TCH_DIGIT | TCH_HEX,// 57 "9 "

    TCH_DELIMITER,// 58 ": "
    TCH_DELIMITER,// 59 "; "

    TCH_DELIMITER | TCH_OPERATOR,// 60 "< "
    TCH_DELIMITER | TCH_OPERATOR,// 61 "= "
    TCH_DELIMITER | TCH_OPERATOR,// 62 "> "
    TCH_DELIMITER,// 63 "? "
    TCH_DELIMITER,// 64 "@ "

    TCH_ID | TCH_LETTER | TCH_HEX,// 65 "A "
    TCH_ID | TCH_LETTER | TCH_HEX,// 66 "B "
    TCH_ID | TCH_LETTER | TCH_HEX,// 67 "C "
    TCH_ID | TCH_LETTER | TCH_HEX,// 68 "D "
    TCH_ID | TCH_LETTER | TCH_HEX,// 69 "E "
    TCH_ID | TCH_LETTER | TCH_HEX,// 70 "F "

    TCH_ID | TCH_LETTER,// 71 "G "
    TCH_ID | TCH_LETTER,// 72 "H "
    TCH_ID | TCH_LETTER,// 73 "I "
    TCH_ID | TCH_LETTER,// 74 "J "
    TCH_ID | TCH_LETTER,// 75 "K "
    TCH_ID | TCH_LETTER,// 76 "L "
    TCH_ID | TCH_LETTER,// 77 "M "
    TCH_ID | TCH_LETTER,// 78 "N "
    TCH_ID | TCH_LETTER,// 79 "O "
    TCH_ID | TCH_LETTER,// 80 "P "
    TCH_ID | TCH_LETTER,// 81 "Q "
    TCH_ID | TCH_LETTER,// 82 "R "
    TCH_ID | TCH_LETTER,// 83 "S "
    TCH_ID | TCH_LETTER,// 84 "T "
    TCH_ID | TCH_LETTER,// 85 "U "
    TCH_ID | TCH_LETTER,// 86 "V "
    TCH_ID | TCH_LETTER,// 87 "W "
    TCH_ID | TCH_LETTER,// 88 "X "
    TCH_ID | TCH_LETTER,// 89 "Y "
    TCH_ID | TCH_LETTER,// 90 "Z "

    TCH_DELIMITER,// 91 "[ "
    TCH_DELIMITER,// 92 "\ "
    TCH_DELIMITER,// 93 "] "
    TCH_DELIMITER | TCH_OPERATOR,// 94 "^ "

    TCH_ID | TCH_LETTER,// 95 "_ "

    TCH_DELIMITER,// 96 "` "

    TCH_ID | TCH_LETTER | TCH_HEX,// 97 "a "
    TCH_ID | TCH_LETTER | TCH_HEX,// 98 "b "
    TCH_ID | TCH_LETTER | TCH_HEX,// 99 "c "
    TCH_ID | TCH_LETTER | TCH_HEX,// 100 "d "
    TCH_ID | TCH_LETTER | TCH_HEX,// 101 "e "
    TCH_ID | TCH_LETTER | TCH_HEX,// 102 "f "

    TCH_ID | TCH_LETTER,// 103 "g "
    TCH_ID | TCH_LETTER,// 104 "h "
    TCH_ID | TCH_LETTER,// 105 "i "
    TCH_ID | TCH_LETTER,// 106 "j "
    TCH_ID | TCH_LETTER,// 107 "k "
    TCH_ID | TCH_LETTER,// 108 "l "
    TCH_ID | TCH_LETTER,// 109 "m "
    TCH_ID | TCH_LETTER,// 110 "n "
    TCH_ID | TCH_LETTER,// 111 "o "
    TCH_ID | TCH_LETTER,// 112 "p "
    TCH_ID | TCH_LETTER,// 113 "q "
    TCH_ID | TCH_LETTER,// 114 "r "
    TCH_ID | TCH_LETTER,// 115 "s "
    TCH_ID | TCH_LETTER,// 116 "t "
    TCH_ID | TCH_LETTER,// 117 "u "
    TCH_ID | TCH_LETTER,// 118 "v "
    TCH_ID | TCH_LETTER,// 119 "w "
    TCH_ID | TCH_LETTER,// 120 "x "
    TCH_ID | TCH_LETTER,// 121 "y "
    TCH_ID | TCH_LETTER,// 122 "z "

    TCH_DELIMITER,// 123 "{ "
    TCH_DELIMITER | TCH_OPERATOR,// 124 "| "
    TCH_DELIMITER,// 125 "} "
    TCH_DELIMITER | TCH_OPERATOR,// 126 "~ "

    0,// 127
    0,// 128
    0,// 129
    0,// 130
    0,// 131
    0,// 132
    0,// 133
    0,// 134
    0,// 135
    0,// 136
    0,// 137
    0,// 138
    0,// 139
    0,// 140
    0,// 141
    0,// 142
    0,// 143
    0,// 144
    0,// 145
    0,// 146
    0,// 147
    0,// 148
    0,// 149
    0,// 150
    0,// 151
    0,// 152
    0,// 153
    0,// 154
    0,// 155
    0,// 156
    0,// 157
    0,// 158
    0,// 159
    0,// 160
    0,// 161
    0,// 162
    0,// 163
    0,// 164
    0,// 165
    0,// 166
    0,// 167
    0,// 168
    0,// 169
    0,// 170
    0,// 171
    0,// 172
    0,// 173
    0,// 174
    0,// 175
    0,// 176
    0,// 177
    0,// 178
    0,// 179
    0,// 180
    0,// 181
    0,// 182
    0,// 183
    0,// 184
    0,// 185
    0,// 186
    0,// 187
    0,// 188
    0,// 189
    0,// 190
    0,// 191
    0,// 192
    0,// 193
    0,// 194
    0,// 195
    0,// 196
    0,// 197
    0,// 198
    0,// 199
    0,// 200
    0,// 201
    0,// 202
    0,// 203
    0,// 204
    0,// 205
    0,// 206
    0,// 207
    0,// 208
    0,// 209
    0,// 210
    0,// 211
    0,// 212
    0,// 213
    0,// 214
    0,// 215
    0,// 216
    0,// 217
    0,// 218
    0,// 219
    0,// 220
    0,// 221
    0,// 222
    0,// 223
    0,// 224
    0,// 225
    0,// 226
    0,// 227
    0,// 228
    0,// 229
    0,// 230
    0,// 231
    0,// 232
    0,// 233
    0,// 234
    0,// 235
    0,// 236
    0,// 237
    0,// 238
    0,// 239
    0,// 240
    0,// 241
    0,// 242
    0,// 243
    0,// 244
    0,// 245
    0,// 246
    0,// 247
    0,// 248
    0,// 249
    0,// 250
    0,// 251
    0,// 252
    0,// 253
    0,// 254
    0  // 255
};


#define _TOKEN_TYPE( x ) ( x & 0xFFFF0000 )
#define _TOKEN_OP( x ) ( x & 0x0000FFFF )

#define	_TOKEN_MAX_ID_LEN		63

#define	_TOKEN_TYPE_EQ_OPERATOR			(0 << 16)
#define	_TOKEN_TYPE_BINARY_OPERATOR		(1 << 16)
#define	_TOKEN_TYPE_UNARY_OP_PREFIX		(2 << 16)
#define	_TOKEN_TYPE_UNARY_OP_POSTFIX	(3 << 16)
#define	_TOKEN_TYPE_FUNCTION_CLOSE		(4 << 16)
#define	_TOKEN_TYPE_FUNCTION_OPEN		(5 << 16)
#define	_TOKEN_TYPE_VIRGULE				(6 << 16)
#define	_TOKEN_TYPE_BRACE_OPEN			(7 << 16)
#define	_TOKEN_TYPE_BRACE_CLOSE			(8 << 16)
#define	_TOKEN_TYPE_SEPARATOR			(9 << 16)
#define	_TOKEN_TYPE_BRACKET_OPEN		(10 << 16)
#define	_TOKEN_TYPE_BRACKET_CLOSE		(11 << 16)
#define	_TOKEN_TYPE_COMMAND				(12 << 16)
#define	_TOKEN_TYPE_STRING				(13 << 16)
#define	_TOKEN_TYPE_VAR					(14 << 16)
#define	_TOKEN_TYPE_IMM					(15 << 16)
#define	_TOKEN_TYPE_TERMINATOR			(16 << 16)

#define	TOKEN_PERM_EQ_OPERATOR		1
#define	TOKEN_PERM_BINARY_OPERATOR	2

#define	TOKEN_PERM_UNARY_OP_INCDEC_PREFIX	4
#define	TOKEN_PERM_UNARY_OP_INCDEC_POSTFIX	8
#define	TOKEN_PERM_UNARY_OP_ARITH_PREFIX	65536
#define	TOKEN_PERM_UNARY_OP_PREFIX	(TOKEN_PERM_UNARY_OP_ARITH_PREFIX | TOKEN_PERM_UNARY_OP_INCDEC_PREFIX)
#define	TOKEN_PERM_UNARY_OP_POSTFIX	TOKEN_PERM_UNARY_OP_INCDEC_POSTFIX

#define	TOKEN_PERM_FUNCTION_OPEN	16
#define	TOKEN_PERM_FUNCTION_CLOSE	32
#define	TOKEN_PERM_VAR				64
#define	TOKEN_PERM_IMM				128
#define	TOKEN_PERM_VIRGULE			256
#define	TOKEN_PERM_BRACE_OPEN		512
#define	TOKEN_PERM_BRACE_CLOSE		1024
#define	TOKEN_PERM_SEPARATOR		2048
#define	TOKEN_PERM_COMMAND			4096
#define	TOKEN_PERM_STRING			8192
#define	TOKEN_PERM_BRACKET_OPEN		16384
#define	TOKEN_PERM_BRACKET_CLOSE	32768

#define	_TOKEN_ID_UNARY_PREFIX_PLUS		0
#define	_TOKEN_ID_UNARY_PREFIX_MINUS	1		/// level
#define	_TOKEN_ID_UNARY_NOT				2
#define	_TOKEN_ID_UNARY_PREFIX_PP		3
#define	_TOKEN_ID_UNARY_PREFIX_MM		4

#define	_TOKEN_ID_MUL					1	///////////////////////////////////
#define	_TOKEN_ID_DIV					2
#define	_TOKEN_ID_MOD					3
#define	_TOKEN_ID_ADD					4	///////////////////////////////////
#define	_TOKEN_ID_SUB					5
#define	_TOKEN_ID_SHR					6
#define	_TOKEN_ID_SHL					7
#define	_TOKEN_ID_OR					8	////////////////////////////////////
#define	_TOKEN_ID_AND					9
#define	_TOKEN_ID_XOR					10

#define	TOK_ERR_NO_ERROR				0
#define	TOK_ERR_INVALID_TOKEN			1
#define	TOK_ERR_EXPECTED_COMMAND		2
#define	TOK_ERR_UNKNOWN_ID				3
#define	TOK_ERR_UNCLOSED_PARENTHESIS	4
#define	TOK_ERR_UNCLOSED_DOUBLE_QUOTE	5
#define	TOK_ERR_SYNTAX_ERROR			6
#define TOK_ERR_HEX_SYNTAX_ERROR		7
#define TOK_ERR_IMM_OVERFLOW			8
#define TOK_ERR_EXPECTED_FLOAT			9
#define TOK_ERR_NESTING_OVERFLOW		10
#define TOK_ERR_UNCLOSED_BRACE			11
#define TOK_ERR_NOT_INSIDE_COMM_OR_FUNC	12
#define TOK_ERR_UNEXPECTED_CLOSE_BRACE	13
#define TOK_ERR_UNEXPECTED_END_OF_EXPR	14
#define TOK_ERR_TOO_MANY_ARGS			15
#define TOK_ERR_NEED_MORE_ARGS			16
#define TOK_ERR_NOT_ALLOWED_HERE		17
#define TOK_ERR_UNEXPECTED_SEPARATOR	18
#define	TOK_ERR_UNXPECTED_COMMAND		19
#define	TOK_ERR_UNEXPECTED_VIRGULE		20
#define	TOK_ERR_NOT_ALLOWED_BRACE_OPEN	21
#define	TOK_ERR_END_WITH_OPEN_BRACE		22
#define	TOK_ERR_CANNOT_CLOSE_BRACE_HERE	23
#define	TOK_ERR_STRING_NOT_ALLOWED_HERE	24
#define	TOK_ERR_CANNOT_OPEN_FUNC_HERE	25
#define	TOK_ERR_NOT_ALLOWED_VAR_HERE	26
#define	TOK_ERR_IMM_NOT_ALLOWED_HERE	27
#define	TOK_ERR_SYNTAX_ERROR_WITH_0_IMM_PREFIX	28
#define	TOK_END_OF_EXPR_WITH_BINOP		29
#define	TOK_ERR_UNKNOWN_PREFIX_OP		30
#define	TOK_ERR_UNKNOWN_BINOP			31
#define	TOK_ERR_UNKNOWN_OP				32
#define	TOK_ERR_UNEXPECTED_END_OF_EXPR_WITHIN_FUNC	33
#define	TOK_ERR_UNEXPECTED_END_OF_EXPR_WITHIN_FUNC_OR_CMD 34
#define	TOK_ERR_END_OF_EXPR_UNCLOSED_BRACE 35
#define	TOK_ERR_PARMS_NOT_FEED	36
#define	TOK_ERR_EQ_NOT_ALLOWED_HERE	37
#define	TOK_ERR_BIN_SYNTAX_ERROR	38
#define	TOK_ERR_UNEXPECTED_ARITH_OP	39

#define Z_OPEARTOR_DOUBLE_CH    16

static const uint8_t OpLut[ 256 ] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,// 33 "! "
    0,// 34 """
    0,// 35 "# "
    0,// 36 "$ "
    _TOKEN_ID_MOD,// 37 "% "
    _TOKEN_ID_AND,// 38 "& "
    0,// 39 "' "
    0,// 40 "( "
    0,// 41 ") "
    _TOKEN_ID_MUL,// 42 "* "
    _TOKEN_ID_ADD,// 43 "+ "
    0,// 44 ","
    _TOKEN_ID_SUB,// 45 "- "
    0,// 46 ". "
    _TOKEN_ID_DIV,// 47 "/ "
    0,0,0,0,0,0,0,0,0,0,0,0,
    Z_OPEARTOR_DOUBLE_CH | _TOKEN_ID_SHL,// 60 "< "
    0,// 61 "= "
    Z_OPEARTOR_DOUBLE_CH | _TOKEN_ID_SHR,// 62 "> "
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    _TOKEN_ID_XOR,// 94 "^ "
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,// 123 "{ "
    _TOKEN_ID_OR,// 124 "| "
    0,// 125 "} "
    0,// 126 "~ "
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
