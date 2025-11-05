#define TCH_BLANK	  1
#define TCH_OPERATOR  2
#define TCH_DIGIT	  4
#define TCH_LETTER	  8
#define TCH_HEX		  16
#define TCH_DELIMITER 32
#define TCH_OCT		  64
#define TCH_ID		  128

const uint8_t tchtab[256] = {

		0,	  // 0 "

		0,	  // 1
		0,	  // 2
		0,	  // 3
		0,	  // 4
		0,	  // 5
		0,	  // 6
		0,	  // 7
		0,	  // 8

		TCH_BLANK,	  // 9
		0,			  // 10

		0,	  // 11
		0,	  // 12

		0,	  // 13

		0,	  // 14
		0,	  // 15
		0,	  // 16
		0,	  // 17
		0,	  // 18
		0,	  // 19
		0,	  // 20
		0,	  // 21
		0,	  // 22
		0,	  // 23
		0,	  // 24
		0,	  // 25
		0,	  // 26
		0,	  // 27
		0,	  // 28
		0,	  // 29
		0,	  // 30
		0,	  // 31

		TCH_BLANK,	  // 32

		TCH_DELIMITER,					 // 33 " ! "
		TCH_DELIMITER,					 // 34 " " "
		TCH_DELIMITER,					 // 35 " # "
		TCH_DELIMITER,					 // 36 " $ "
		TCH_DELIMITER | TCH_OPERATOR,	 // 37 " % "
		TCH_DELIMITER | TCH_OPERATOR,	 // 38 " & "
		TCH_DELIMITER,					 // 39 " ' "
		TCH_DELIMITER,					 // 40 " ( "
		TCH_DELIMITER,					 // 41 " ) "
		TCH_DELIMITER | TCH_OPERATOR,	 // 42 " * "
		TCH_DELIMITER | TCH_OPERATOR,	 // 43 " + "
		TCH_DELIMITER,					 // 44 " , "
		TCH_DELIMITER | TCH_OPERATOR,	 // 45 " - "
		TCH_DELIMITER,					 // 46 " . "
		TCH_DELIMITER | TCH_OPERATOR,	 // 47 " / "

		TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,	   // 48 " 0 "
		TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,	   // 49 " 1 "
		TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,	   // 50 " 2 "
		TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,	   // 51 " 3 "
		TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,	   // 52 " 4 "
		TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,	   // 53 " 5 "
		TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,	   // 54 " 6 "
		TCH_ID | TCH_DIGIT | TCH_HEX | TCH_OCT,	   // 55 " 7 "
		TCH_ID | TCH_DIGIT | TCH_HEX,			   // 56 " 8 "
		TCH_ID | TCH_DIGIT | TCH_HEX,			   // 57 " 9 "

		TCH_DELIMITER,	  // 58 " : "
		TCH_DELIMITER,	  // 59 " ; "

		TCH_DELIMITER | TCH_OPERATOR,	 // 60 " < "
		TCH_DELIMITER | TCH_OPERATOR,	 // 61 " = "
		TCH_DELIMITER | TCH_OPERATOR,	 // 62 " > "
		TCH_DELIMITER,					 // 63 " ? "
		TCH_DELIMITER,					 // 64 " @ "

		TCH_ID | TCH_LETTER | TCH_HEX,	  // 65 " A "
		TCH_ID | TCH_LETTER | TCH_HEX,	  // 66 " B "
		TCH_ID | TCH_LETTER | TCH_HEX,	  // 67 " C "
		TCH_ID | TCH_LETTER | TCH_HEX,	  // 68 " D "
		TCH_ID | TCH_LETTER | TCH_HEX,	  // 69 " E "
		TCH_ID | TCH_LETTER | TCH_HEX,	  // 70 " F "

		TCH_ID | TCH_LETTER,	// 71 " G "
		TCH_ID | TCH_LETTER,	// 72 " H "
		TCH_ID | TCH_LETTER,	// 73 " I "
		TCH_ID | TCH_LETTER,	// 74 " J "
		TCH_ID | TCH_LETTER,	// 75 " K "
		TCH_ID | TCH_LETTER,	// 76 " L "
		TCH_ID | TCH_LETTER,	// 77 " M "
		TCH_ID | TCH_LETTER,	// 78 " N "
		TCH_ID | TCH_LETTER,	// 79 " O "
		TCH_ID | TCH_LETTER,	// 80 " P "
		TCH_ID | TCH_LETTER,	// 81 " Q "
		TCH_ID | TCH_LETTER,	// 82 " R "
		TCH_ID | TCH_LETTER,	// 83 " S "
		TCH_ID | TCH_LETTER,	// 84 " T "
		TCH_ID | TCH_LETTER,	// 85 " U "
		TCH_ID | TCH_LETTER,	// 86 " V "
		TCH_ID | TCH_LETTER,	// 87 " W "
		TCH_ID | TCH_LETTER,	// 88 " X "
		TCH_ID | TCH_LETTER,	// 89 " Y "
		TCH_ID | TCH_LETTER,	// 90 " Z "

		TCH_DELIMITER,					 // 91 " [ "
		TCH_DELIMITER,					 // 92 " \ "
		TCH_DELIMITER,					 // 93 " ] "
		TCH_DELIMITER | TCH_OPERATOR,	 // 94 " ^ "

		TCH_ID | TCH_LETTER,	// 95 " _ "

		TCH_DELIMITER,	  // 96 " ` "

		TCH_ID | TCH_LETTER | TCH_HEX,	  // 97 " a "
		TCH_ID | TCH_LETTER | TCH_HEX,	  // 98 " b "
		TCH_ID | TCH_LETTER | TCH_HEX,	  // 99 " c "
		TCH_ID | TCH_LETTER | TCH_HEX,	  // 100 " d "
		TCH_ID | TCH_LETTER | TCH_HEX,	  // 101 " e "
		TCH_ID | TCH_LETTER | TCH_HEX,	  // 102 " f "

		TCH_ID | TCH_LETTER,	// 103 " g "
		TCH_ID | TCH_LETTER,	// 104 " h "
		TCH_ID | TCH_LETTER,	// 105 " i "
		TCH_ID | TCH_LETTER,	// 106 " j "
		TCH_ID | TCH_LETTER,	// 107 " k "
		TCH_ID | TCH_LETTER,	// 108 " l "
		TCH_ID | TCH_LETTER,	// 109 " m "
		TCH_ID | TCH_LETTER,	// 110 " n "
		TCH_ID | TCH_LETTER,	// 111 " o "
		TCH_ID | TCH_LETTER,	// 112 " p "
		TCH_ID | TCH_LETTER,	// 113 " q "
		TCH_ID | TCH_LETTER,	// 114 " r "
		TCH_ID | TCH_LETTER,	// 115 " s "
		TCH_ID | TCH_LETTER,	// 116 " t "
		TCH_ID | TCH_LETTER,	// 117 " u "
		TCH_ID | TCH_LETTER,	// 118 " v "
		TCH_ID | TCH_LETTER,	// 119 " w "
		TCH_ID | TCH_LETTER,	// 120 " x "
		TCH_ID | TCH_LETTER,	// 121 " y "
		TCH_ID | TCH_LETTER,	// 122 " z "

		TCH_DELIMITER,					 // 123 " { "
		TCH_DELIMITER | TCH_OPERATOR,	 // 124 " | "
		TCH_DELIMITER,					 // 125 " } "
		TCH_DELIMITER | TCH_OPERATOR,	 // 126 " ~ "

		0,	  // 127
		0,	  // 128
		0,	  // 129
		0,	  // 130
		0,	  // 131
		0,	  // 132
		0,	  // 133
		0,	  // 134
		0,	  // 135
		0,	  // 136
		0,	  // 137
		0,	  // 138
		0,	  // 139
		0,	  // 140
		0,	  // 141
		0,	  // 142
		0,	  // 143
		0,	  // 144
		0,	  // 145
		0,	  // 146
		0,	  // 147
		0,	  // 148
		0,	  // 149
		0,	  // 150
		0,	  // 151
		0,	  // 152
		0,	  // 153
		0,	  // 154
		0,	  // 155
		0,	  // 156
		0,	  // 157
		0,	  // 158
		0,	  // 159
		0,	  // 160
		0,	  // 161
		0,	  // 162
		0,	  // 163
		0,	  // 164
		0,	  // 165
		0,	  // 166
		0,	  // 167
		0,	  // 168
		0,	  // 169
		0,	  // 170
		0,	  // 171
		0,	  // 172
		0,	  // 173
		0,	  // 174
		0,	  // 175
		0,	  // 176
		0,	  // 177
		0,	  // 178
		0,	  // 179
		0,	  // 180
		0,	  // 181
		0,	  // 182
		0,	  // 183
		0,	  // 184
		0,	  // 185
		0,	  // 186
		0,	  // 187
		0,	  // 188
		0,	  // 189
		0,	  // 190
		0,	  // 191
		0,	  // 192
		0,	  // 193
		0,	  // 194
		0,	  // 195
		0,	  // 196
		0,	  // 197
		0,	  // 198
		0,	  // 199
		0,	  // 200
		0,	  // 201
		0,	  // 202
		0,	  // 203
		0,	  // 204
		0,	  // 205
		0,	  // 206
		0,	  // 207
		0,	  // 208
		0,	  // 209
		0,	  // 210
		0,	  // 211
		0,	  // 212
		0,	  // 213
		0,	  // 214
		0,	  // 215
		0,	  // 216
		0,	  // 217
		0,	  // 218
		0,	  // 219
		0,	  // 220
		0,	  // 221
		0,	  // 222
		0,	  // 223
		0,	  // 224
		0,	  // 225
		0,	  // 226
		0,	  // 227
		0,	  // 228
		0,	  // 229
		0,	  // 230
		0,	  // 231
		0,	  // 232
		0,	  // 233
		0,	  // 234
		0,	  // 235
		0,	  // 236
		0,	  // 237
		0,	  // 238
		0,	  // 239
		0,	  // 240
		0,	  // 241
		0,	  // 242
		0,	  // 243
		0,	  // 244
		0,	  // 245
		0,	  // 246
		0,	  // 247
		0,	  // 248
		0,	  // 249
		0,	  // 250
		0,	  // 251
		0,	  // 252
		0,	  // 253
		0,	  // 254
		0	  // 255
};

static const uint8_t atoh_lut256[ 256 ] = {

	0xFF, // 0
	0xFF, // 1
	0xFF, // 2
	0xFF, // 3
	0xFF, // 4
	0xFF, // 5
	0xFF, // 6
	0xFF, // 7
	0xFF, // 8
	0xFF, // 9
	0xFF, // 10
	0xFF, // 11
	0xFF, // 12
	0xFF, // 13
	0xFF, // 14
	0xFF, // 15
	0xFF, // 16
	0xFF, // 17
	0xFF, // 18
	0xFF, // 19
	0xFF, // 20
	0xFF, // 21
	0xFF, // 22
	0xFF, // 23
	0xFF, // 24
	0xFF, // 25
	0xFF, // 26
	0xFF, // 27
	0xFF, // 28
	0xFF, // 29
	0xFF, // 30
	0xFF, // 31
	0xFF, // 32
	0xFF, // 33
	0xFF, // 34
	0xFF, // 35
	0xFF, // 36
	0xFF, // 37
	0xFF, // 38
	0xFF, // 39
	0xFF, // 40
	0xFF, // 41
	0xFF, // 42
	0xFF, // 43
	0xFF, // 44
	0xFF, // 45
	0xFF, // 46
	0xFF, // 47
	0, // 48 " 0 "
	1, // 49 " 1 "
	2, // 50 " 2 "
	3, // 51 " 3 "
	4, // 52 " 4 "
	5, // 53 " 5 "
	6, // 54 " 6 "
	7, // 55 " 7 "
	8, // 56 " 8 "
	9, // 57 " 9 "
	0xFF, // 58
	0xFF, // 59
	0xFF, // 60
	0xFF, // 61
	0xFF, // 62
	0xFF, // 63
	0xFF, // 64
	10, // 65 " A "
	11, // 66 " B "
	12, // 67 " C "
	13, // 68 " D "
	14, // 69 " E "
	15, // 70 " F "
	0xFF, // 71
	0xFF, // 72
	0xFF, // 73
	0xFF, // 74
	0xFF, // 75
	0xFF, // 76
	0xFF, // 77
	0xFF, // 78
	0xFF, // 79
	0xFF, // 80
	0xFF, // 81
	0xFF, // 82
	0xFF, // 83
	0xFF, // 84
	0xFF, // 85
	0xFF, // 86
	0xFF, // 87
	0xFF, // 88
	0xFF, // 89
	0xFF, // 90
	0xFF, // 91
	0xFF, // 92
	0xFF, // 93
	0xFF, // 94
	0xFF, // 95
	0xFF, // 96
	10, // 97 " a "
	11, // 98 " b "
	12, // 99 " c "
	13, // 100 " d "
	14, // 101 " e "
	15, // 102 " f "
	0xFF, // 103
	0xFF, // 104
	0xFF, // 105
	0xFF, // 106
	0xFF, // 107
	0xFF, // 108
	0xFF, // 109
	0xFF, // 110
	0xFF, // 111
	0xFF, // 112
	0xFF, // 113
	0xFF, // 114
	0xFF, // 115
	0xFF, // 116
	0xFF, // 117
	0xFF, // 118
	0xFF, // 119
	0xFF, // 120
	0xFF, // 121
	0xFF, // 122
	0xFF, // 123
	0xFF, // 124
	0xFF, // 125
	0xFF, // 126
	0xFF, // 127
	0xFF, // 128
	0xFF, // 129
	0xFF, // 130
	0xFF, // 131
	0xFF, // 132
	0xFF, // 133
	0xFF, // 134
	0xFF, // 135
	0xFF, // 136
	0xFF, // 137
	0xFF, // 138
	0xFF, // 139
	0xFF, // 140
	0xFF, // 141
	0xFF, // 142
	0xFF, // 143
	0xFF, // 144
	0xFF, // 145
	0xFF, // 146
	0xFF, // 147
	0xFF, // 148
	0xFF, // 149
	0xFF, // 150
	0xFF, // 151
	0xFF, // 152
	0xFF, // 153
	0xFF, // 154
	0xFF, // 155
	0xFF, // 156
	0xFF, // 157
	0xFF, // 158
	0xFF, // 159
	0xFF, // 160
	0xFF, // 161
	0xFF, // 162
	0xFF, // 163
	0xFF, // 164
	0xFF, // 165
	0xFF, // 166
	0xFF, // 167
	0xFF, // 168
	0xFF, // 169
	0xFF, // 170
	0xFF, // 171
	0xFF, // 172
	0xFF, // 173
	0xFF, // 174
	0xFF, // 175
	0xFF, // 176
	0xFF, // 177
	0xFF, // 178
	0xFF, // 179
	0xFF, // 180
	0xFF, // 181
	0xFF, // 182
	0xFF, // 183
	0xFF, // 184
	0xFF, // 185
	0xFF, // 186
	0xFF, // 187
	0xFF, // 188
	0xFF, // 189
	0xFF, // 190
	0xFF, // 191
	0xFF, // 192
	0xFF, // 193
	0xFF, // 194
	0xFF, // 195
	0xFF, // 196
	0xFF, // 197
	0xFF, // 198
	0xFF, // 199
	0xFF, // 200
	0xFF, // 201
	0xFF, // 202
	0xFF, // 203
	0xFF, // 204
	0xFF, // 205
	0xFF, // 206
	0xFF, // 207
	0xFF, // 208
	0xFF, // 209
	0xFF, // 210
	0xFF, // 211
	0xFF, // 212
	0xFF, // 213
	0xFF, // 214
	0xFF, // 215
	0xFF, // 216
	0xFF, // 217
	0xFF, // 218
	0xFF, // 219
	0xFF, // 220
	0xFF, // 221
	0xFF, // 222
	0xFF, // 223
	0xFF, // 224
	0xFF, // 225
	0xFF, // 226
	0xFF, // 227
	0xFF, // 228
	0xFF, // 229
	0xFF, // 230
	0xFF, // 231
	0xFF, // 232
	0xFF, // 233
	0xFF, // 234
	0xFF, // 235
	0xFF, // 236
	0xFF, // 237
	0xFF, // 238
	0xFF, // 239
	0xFF, // 240
	0xFF, // 241
	0xFF, // 242
	0xFF, // 243
	0xFF, // 244
	0xFF, // 245
	0xFF, // 246
	0xFF, // 247
	0xFF, // 248
	0xFF, // 249
	0xFF, // 250
	0xFF, // 251
	0xFF, // 252
	0xFF, // 253
	0xFF, // 254
	0xFF // 255
};

#define _TOKEN_MAX_ID_LEN								  63

#define _OP_DOUBLE_CH							      16
#define _OP_ID_UNARY_PREFIX_PLUS					  0
#define _OP_ID_UNARY_PREFIX_MINUS					  1
#define _OP_ID_UNARY_NOT							  2
#define _OP_ID_MUL									  3
#define _OP_ID_DIV									  4
#define _OP_ID_MOD									  5
#define _OP_ID_ADD									  6
#define _OP_ID_SUB									  7
#define _OP_ID_SHR									  8
#define _OP_ID_SHL									  9
#define _OP_ID_OR									  10
#define _OP_ID_AND									  11
#define _OP_ID_XOR									  12
#define _OP_ID_BRACKET_OPEN							  13
#define _OP_ID_FUNC									  14
#define _OP_ID_VR									  15

static const uint8_t OpLut[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0,				  // 33 " ! "
		0,				  // 34 " " "
		0,				  // 35 " # "
		0,				  // 36 " $ "
		_OP_ID_MOD | (2 << 5),	  // 37 " % "
		_OP_ID_AND + (1 << 5),	  // 38 " & "
		0,				  // 39 " ' "
		0,				  // 40 " ( "
		0,				  // 41 " ) "
		_OP_ID_MUL | (2 << 5),	  // 42 " * "
		_OP_ID_ADD + (1 << 5),	  // 43 " + "
		0,				  // 44 " , "
		_OP_ID_SUB + (1 << 5),	  // 45 " - "
		0,				  // 46 " . "
		_OP_ID_DIV | (2 << 5),	  // 47 " / "
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		_OP_DOUBLE_CH | _OP_ID_SHL | (2 << 5),	 // 60 " < "
		0,										 // 61 " = "
		_OP_DOUBLE_CH | _OP_ID_SHR | (2 << 5),	 // 62 " > "
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		_OP_ID_XOR + (1 << 5),	  // 94 " ^ "
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0,				 // 123 " { "
		_OP_ID_OR + (1 << 5),	 // 124 " | "
		0,				 // 125 " } "
		0,				 // 126 " ~ "
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
