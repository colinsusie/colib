/**
 * json解析器：只支持utf-8格式
 */
#define LUA_LIB
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include "lua.h"
#include "lauxlib.h"
#include "coconf.h"
#include "costream.h"
#include "coutf8.h"

//-----------------------------------------------------------------------------
// parser
//-------------------------------------
// 与Lua相关的代码

static inline void l_add_object(lua_State *L) {
	lua_checkstack(L, 5);
	lua_newtable(L);
}
static inline void l_begin_pair(lua_State *L, const char *k, size_t sz) {
	lua_pushlstring(L, k, sz);
}
static inline void l_end_pair(lua_State *L) {
	lua_rawset(L, -3);
}
static inline void l_add_array(lua_State *L) {
	lua_checkstack(L, 10);
	lua_newtable(L);
}
static inline void l_add_index(lua_State *L, int i) {
	lua_rawseti(L, -2, i+1);
}
static inline void l_add_string(lua_State *L, const char *s, size_t sz) {
	lua_pushlstring(L, s, sz);
}
static inline void l_add_float(lua_State *L, double f) {
	lua_pushnumber(L, (lua_Number)f);
}
static inline void l_add_integer(lua_State *L, int64_t i) {
	lua_pushinteger(L, (lua_Integer)i);
}
static inline void l_add_boolean(lua_State *L, int b) {
	lua_pushboolean(L, b);
}
static inline void l_add_null(lua_State *L) {
	lua_pushlightuserdata(L, NULL);
}
static inline void l_error(lua_State *L, const char *msg) {
	luaL_error(L, msg);
}

// 解析事件
#define ON_ADD_OBJECT(ud) l_add_object((lua_State*)(ud))
#define ON_BEGIN_PAIR(ud, k, sz) l_begin_pair((lua_State*)(ud), k, sz)
#define ON_END_PAIR(ud) l_end_pair((lua_State*)(ud))
#define ON_ADD_ARRAY(ud) l_add_array((lua_State*)(ud))
#define ON_ADD_INDEX(ud, i) l_add_index((lua_State*)(ud), i)
#define ON_ADD_STRING(ud, s, sz) l_add_string((lua_State*)(ud), s, sz)
#define ON_ADD_FLOAT(ud, f) l_add_float((lua_State*)(ud), f)
#define ON_ADD_INTEGER(ud, i) l_add_integer((lua_State*)(ud), i)
#define ON_ADD_BOOLEAN(ud, b) l_add_boolean((lua_State*)(ud), b)
#define ON_ADD_NULL(ud) l_add_null((lua_State*)(ud))
#define ON_ERROR(ud, msg) l_error((lua_State*)(ud), msg)

//-------------------------------------
// 解析json，这部分代码与Lua无关，是通用的解析器；如果要移植这部分代码，需要把 //>>> 开头的注释去掉

// token type
typedef enum {
	TK_OBJ_BEGIN, TK_OBJ_END, TK_ARY_BEGIN, TK_ARY_END, TK_STRING, TK_FLOAT, TK_INTEGER, 
	TK_BOOLEAN, TK_NULL, TK_COLON, TK_COMMA, TK_END
} json_token_e;

// token type name
static const char *token_tname[] = {
	"{", "}", "[", "]", "<string>", "<float>", "<integer>", 
	"<boolean>", "<null>", ":", ",", "<eof>", 
};

// token data
typedef struct {
	json_token_e type;
	union {
		double d;
		int64_t i;
		const char *s;
		int b;
	} value;
	size_t strsz;
} json_token_t;

// 错误消息的大小
#define ERRMSG_SIZE 256

// json解析器
typedef struct {
	json_token_t tk;	// 当前的Token
	const char *str;	// json字符串
	const char *ptr;	// json字符串解析指针
	void *ud;			// 解析事件的用户数据
	membuffer_t buff;	// 临时缓存
	uint16_t curdepth;	// 当前层次
	uint16_t maxdepth;	// 最大层次
	char errmsg[ERRMSG_SIZE];	// 保存错误消息 
	//>>>jmp_buf jb;			// 用于实现从解析中出错直接跳出
} json_parser_t;

static inline void parser_init(json_parser_t *parser, const char *str, void *ud, 
	uint16_t maxdepth) {
	membuffer_init(&parser->buff);
	parser->str = str;
	parser->ptr = str;
	parser->ud = ud;
	parser->maxdepth = maxdepth;
	parser->curdepth = 0;
}

static inline void parser_free(json_parser_t *parser) {
	membuffer_free(&parser->buff);
}

// 抛出错误
static void parser_throw_error(json_parser_t *parser, const char *fmt, ...) {
	membuffer_free(&parser->buff);
	va_list arg;
	va_start(arg, fmt);
	vsnprintf(parser->errmsg, ERRMSG_SIZE, fmt, arg);
	va_end(arg);
	ON_ERROR(parser->ud, parser->errmsg);
	// 直接跳出解析代码，由于Lua的lua_error也是用longjmp，所以下面的代码没有机会执行到。但其他语言就不一定。
	//>>>longjmp(parser->jb, 1);
}

// 辅助宏
#define peek_and_next(p) (*(p)->ptr++)
#define peek(p) (*(p)->ptr)
#define next(p) ((p)->ptr++)
#define savechar(p, c) membuffer_putc(&(p)->buff, c)
#define currpos(p) (unsigned long)((p)->ptr - (p)->str)

// 取解析到的错误内容
static const char* parser_error_content(json_parser_t *p) {
	int n = currpos(p);
	if (n > 50) n = 50;	// 调整这个数获得更长的内容
	membuffer_reset(&p->buff);
	membuffer_putb(&p->buff, p->ptr - n, n);
	membuffer_putc(&p->buff, '\0');
	return p->buff.b;
}

// 解析数字
#define invalid_number(p) parser_throw_error(p, "Invalid number, at: %s[:%lu]", parser_error_content(p), currpos(p))
#define MAXBY10		(int64_t)(INT64_MAX / 10)
#define MAXLASTD	(int)(INT64_MAX % 10)
static double powersOf10[] = {10., 100., 1.0e4,  1.0e8,   1.0e16, 1.0e32, 1.0e64, 1.0e128, 1.0e256};
static void parser_parse_number(json_parser_t *p, char ch) {
	int64_t in = 0;			// 整型值
	double db = 0.0;		// 浮点数
	int isdb = 0;			// 是否是浮点数
	int neg = 0;			// 是否是负数
	int eneg = 0;			// 指数部分是否负数
	int decimals = 0;		// 小数位数
	int exponent = 0;		// 指数位数

	if (ch == '-') {	// 负值
		neg = 1;
		ch = peek_and_next(p);
	}
	if (unlikely(ch == '0')) {	// 0开头的后面只能是：.eE或结束
		ch = peek(p);
	} else if (likely(ch >= '1' && ch <= '9')) {
		in = ch - '0';
		ch = peek(p);
		int d;
		while (likely(isdigit(ch))) {
			next(p);
			d = ch - '0';
			if (unlikely(in >= MAXBY10 && (in > MAXBY10 || d > MAXLASTD + neg))) {	// 更大的数字就用浮点数表示
				isdb = 1;
				db = (double)in;
				break;
			}
			in = in * 10 + d;
			ch = peek(p);
		}
	} else {
		invalid_number(p);		// 只能是0~9开头
	}

	if (isdb) {	// 用浮点数表示大数
		while (isdigit(ch)) {
			next(p);
			in = in * 10 + (ch - '0');
			ch = peek(p);
		}
	}

	if (ch == '.') {	// 小数点部分
		if (!isdb) {
			isdb = 1;
			db = (double)in;
		}
		next(p);
		ch = peek(p);
		if (unlikely(!isdigit(ch)))
			invalid_number(p);  // .后面一定是数字
		do {
			next(p);
			db = db * 10. + (ch - '0');
			decimals++;
			ch = peek(p);
		} while (isdigit(ch));
		exponent -= decimals;
	}

	if (ch == 'e' || ch == 'E') {	// 指数部分
		if (!isdb) {		// 有e强制认为是浮点数
			isdb = 1;
			db = (double)in;
		}
		next(p);
		ch = peek(p);
		eneg = 0;
		if (ch == '-') {
			eneg = 1;
			next(p);
			ch = peek(p);
		} else if (ch == '+') {
			next(p);
			ch = peek(p);
		}
		if (unlikely(!isdigit(ch)))
			invalid_number(p);  // 后面一定是数字
		int exp = 0;
		do {
			next(p);
			exp = exp * 10. + (ch - '0');
			ch = peek(p);
		} while (isdigit(ch));
		if (eneg) exponent -= exp;
		else exponent += exp;
	}

	if (isdb) {
		if (exponent < DBL_MIN_10_EXP || exponent > DBL_MAX_10_EXP)	// 判断是否溢出
			parser_throw_error(p, "Float overflow, at: %s[:%lu]", parser_error_content(p), currpos(p));
		// 计算结果
		int n = exponent;
		if (n < 0) n = -n;
		double p10 = 1.0;
		double *d;
		for (d = powersOf10; n != 0; n >>= 1, d += 1) {
			if (n & 1) p10 *= *d;
		}
		if (exponent < 0)
			db /= p10;
		else
			db *= p10;
		p->tk.type = TK_FLOAT;
		p->tk.value.d = neg ? -db : db;
	} else {
		p->tk.type = TK_INTEGER;
		p->tk.value.i = neg ? -in : in;
	}
}

// 解析utf8转义
static void parser_parse_utf8esc(json_parser_t *p) {
	uint32_t cp = 0;
	char ch;
	int i, hex;
	// hex[4]
	for (i = 0; i < 4; ++i) {
		ch = peek_and_next(p);
		if (!isxdigit(ch)) {
			parser_throw_error(p, "Invalid utf8 escape sequence, at: %s[:%lu]", parser_error_content(p), currpos(p));
		}
		hex = isdigit(ch) ? ch - '0' : ((ch | 0x20) - 'a') + 10;
		cp |= hex << (12 - (i << 2));
		// hex[i] = (isdigit(ch) ? ch - '0' : ((ch | 0x20) - 'a') + 10);
	}
	// cp = (hex[0] << 12) | (hex[1] << 8) | (hex[2] << 4) | hex[3];

	char buff[UTF8BUFFSZ];
	int n = coutf8_encode(buff, cp);
	membuffer_putb(&p->buff, buff + (UTF8BUFFSZ - n), n);
}

// 解析字符串
static void parser_parse_string(json_parser_t *p) {
	char ch = peek_and_next(p);
	while (likely(ch != '"')) {
		if (unlikely(ch == '\0' || ch == '\n' || ch == '\r')) {
			parser_throw_error(p, "Unfinished string, at: %s[:%lu]", parser_error_content(p), currpos(p));
		} else if (unlikely(ch == '\\')) {
			ch = peek_and_next(p);
			switch (ch) {
				case 'b': 
					savechar(p, '\b'); break;
				case 'f': 
					savechar(p, '\f'); break;
				case 'n': 
					savechar(p, '\n'); break;
				case 'r': 
					savechar(p, '\r'); break;
				case 't': 
					savechar(p, '\t'); break;
				case '/': 
					savechar(p, '/'); break;
				case '\\': 
					savechar(p, '\\'); break;
				case '"': 
					savechar(p, '"'); break;
				case 'u': 
					parser_parse_utf8esc(p); break;
				default:
					parser_throw_error(p, "Invalid escape sequence, at: %s[:%lu]", parser_error_content(p), currpos(p));
			}
			ch = peek_and_next(p);
		} else {
			savechar(p, ch);
			ch = peek_and_next(p);
		}	
	}
	p->tk.type = TK_STRING;
	p->tk.strsz = p->buff.sz;
	p->tk.value.s = p->buff.b;
}

// 取下一个token
static void parser_next_token(json_parser_t *p) {
	// 删除空白字符
	char ch = peek_and_next(p);
	while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
		ch = peek_and_next(p);

	membuffer_reset(&p->buff);
	switch (ch) {
		case '{':
			p->tk.type = TK_OBJ_BEGIN;
			return;
		case '}':
			p->tk.type = TK_OBJ_END;
			return; 
		case '[':
			p->tk.type = TK_ARY_BEGIN;
			return;
		case ']':
			p->tk.type = TK_ARY_END;
			return;
		case '"':
			parser_parse_string(p);
			return;
		case '-':
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			parser_parse_number(p, ch);
			return;
		case 'f':
			if (peek_and_next(p) == 'a') 
			if (peek_and_next(p) == 'l')
			if (peek_and_next(p) == 's')
			if (peek_and_next(p) == 'e') {
				p->tk.type = TK_BOOLEAN;
				p->tk.value.b = 0;
				return;
			}
			parser_throw_error(p, "Invalid boolean, at: %s[:%lu]", parser_error_content(p), currpos(p));
			return;
		case 't':
			if (peek_and_next(p) == 'r') 
			if (peek_and_next(p) == 'u')
			if (peek_and_next(p) == 'e') {
				p->tk.type = TK_BOOLEAN;
				p->tk.value.b = 1;
				return;
			}
			parser_throw_error(p, "Invalid boolean, at: %s[:%lu]", parser_error_content(p), currpos(p));
			return;
		case 'n':
			if (peek_and_next(p) == 'u') 
			if (peek_and_next(p) == 'l')
			if (peek_and_next(p) == 'l') {
				p->tk.type = TK_NULL;
				return;
			}
			parser_throw_error(p, "Invalid null, at: %s[:%lu]", parser_error_content(p), currpos(p));
			return;
		case ':':
			p->tk.type = TK_COLON;
			return;
		case ',':
			p->tk.type = TK_COMMA;
			return;
		case '\0':
			p->tk.type = TK_END;
			return;
		default:
			parser_throw_error(p, "Invalid token, at: %s[:%lu]", parser_error_content(p), currpos(p));
			return;
	}
}

static void parser_parse_value(json_parser_t *parser);

// 增加深度
static inline void parser_add_depth(json_parser_t *p) {
	p->curdepth++;
	if (p->curdepth >= p->maxdepth)
		parser_throw_error(p, "Too many nested data, max depth is %d, at: %s[:%lu]", p->maxdepth, 
			parser_error_content(p), currpos(p));
}

// 解析对象
static void parser_parse_object(json_parser_t *p) {
	parser_add_depth(p);
	ON_ADD_OBJECT(p->ud);
	parser_next_token(p);
	if (p->tk.type == TK_OBJ_END) {
		p->curdepth--;
		return;
	}
	while (1) {
		if (p->tk.type != TK_STRING)
			parser_throw_error(p, "Expect '<object key(string)>' but got '%s', at: %s[:%lu]", 
				token_tname[p->tk.type], parser_error_content(p), currpos(p));
		
		ON_BEGIN_PAIR(p->ud, p->tk.value.s, p->tk.strsz);
		parser_next_token(p);
		if (p->tk.type != TK_COLON)
			parser_throw_error(p, "Expect ':' but got '%s', at: %s[:%lu]", token_tname[p->tk.type], 
				parser_error_content(p), currpos(p));
		parser_next_token(p);
		parser_parse_value(p);
		ON_END_PAIR(p->ud);

		parser_next_token(p);
		if (p->tk.type == TK_OBJ_END) {
			p->curdepth--;
			return;
		} else if (p->tk.type != TK_COMMA)
			parser_throw_error(p, "Expect ',' but got '%s', at: %s[:%lu]", token_tname[p->tk.type], 
				parser_error_content(p), currpos(p));
		parser_next_token(p);
	}
}

// 解析数据
static void parser_parse_array(json_parser_t *p) {
	parser_add_depth(p);
	ON_ADD_ARRAY(p->ud);
	parser_next_token(p);
	if (p->tk.type == TK_ARY_END) {
		p->curdepth--;
		return;
	}
	int i;
	for (i = 0; ; i++) {
		parser_parse_value(p);
		ON_ADD_INDEX(p->ud, i);
		parser_next_token(p);
		if (p->tk.type == TK_ARY_END) {
			p->curdepth--;
			return;
		} else if (p->tk.type != TK_COMMA)
			parser_throw_error(p, "Expect ',' but got '%s', at: %s[:%lu]", token_tname[p->tk.type], 
				parser_error_content(p), currpos(p));
		parser_next_token(p);
	}
}

// 解析一个json值
static void parser_parse_value(json_parser_t *p) {
	switch (p->tk.type) {
		case TK_STRING:
			ON_ADD_STRING(p->ud, p->tk.value.s, p->tk.strsz);
			break;
		case TK_FLOAT:
			ON_ADD_FLOAT(p->ud, p->tk.value.d);
			break;
		case TK_INTEGER:
			ON_ADD_INTEGER(p->ud, p->tk.value.i);
			break;
		case TK_BOOLEAN:
			ON_ADD_BOOLEAN(p->ud, p->tk.value.b);
			break;
		case TK_NULL:
			ON_ADD_NULL(p->ud);
			break;
		case TK_OBJ_BEGIN:
			parser_parse_object(p);
			break;
		case TK_ARY_BEGIN:
			parser_parse_array(p);
			break;
		default:
			parser_throw_error(p, "Expect '<json value>' but got '%s', at: %s[:%lu]", token_tname[p->tk.type], 
				parser_error_content(p), currpos(p));
			break;
	}
}

// 解析json文本
static void parser_do_parse(const char *str, void *ud, uint16_t maxdepth) {
	json_parser_t parser;
	parser_init(&parser, str, ud, maxdepth);
	//>>>if (setjmp(parser.jb) == 0) {
		parser_next_token(&parser);
		parser_parse_value(&parser);
		parser_next_token(&parser);
		if (parser.tk.type != TK_END) {
			parser_throw_error(&parser, "Expect '<eof>' but got '%s', at: %s[:%lu]", token_tname[parser.tk.type], 
				parser_error_content(&parser), currpos(&parser));
		}
		parser_free(&parser);
	//>>>}
}

//-----------------------------------------------------------------------------
// dumper



//-----------------------------------------------------------------------------
// 接口

#define DEF_MAX_DEPTH 128
#define DEF_FILE_BUFFSZ 4096

// 从字符串加载：json.load(str) -> obj
// 要求字符串必须以0结尾
static int l_load(lua_State *L) {
	const char *str = luaL_checkstring(L, 1);
	uint16_t maxdepth = (uint16_t)luaL_optinteger(L, 2, DEF_MAX_DEPTH);
	parser_do_parse(str, L, maxdepth);
	return 1;
}

// 保存到字符串: json.dump(obj) -> str
static int l_dump(lua_State *L) {
	return 0;
}

static const luaL_Reg lib[] = {
	{"load", l_load},
	{"dump", l_dump},
	{NULL, NULL},
};

LUAMOD_API int luaopen_colibc_json(lua_State *L) {
	luaL_newlib(L, lib);
	// json.null
    lua_pushlightuserdata(L, NULL);
    lua_setfield(L, -2, "null");
	return 1;
}