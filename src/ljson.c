/**
 * json解析器：只支持utf-8格式
 */
#define LUA_LIB
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <math.h>
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
	luaL_checkstack(L, 6, NULL);
	lua_newtable(L);
}
static inline void l_begin_pair(lua_State *L, const char *k, size_t sz) {
	lua_pushlstring(L, k, sz);
}
static inline void l_end_pair(lua_State *L) {
	lua_rawset(L, -3);
}
static inline void l_add_array(lua_State *L) {
	luaL_checkstack(L, 6, NULL);
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

// 错误消息的大小
#define ERRMSG_SIZE 256

// json解析器
typedef struct {
	const char *str;	// json字符串
	const char *ptr;	// json字符串解析指针
	void *ud;			// 解析事件的用户数据
	membuffer_t buff;	// 临时缓存
	int curdepth;	// 当前层次
	int maxdepth;	// 最大层次
	int allowcomment; // 是否允许注释
	char errmsg[ERRMSG_SIZE];	// 保存错误消息 
	//>>>jmp_buf jb;			// 用于实现从解析中出错直接跳出
} json_parser_t;

static inline void parser_init(json_parser_t *parser, const char *str, size_t size, void *ud,  
	int maxdepth, int allowcomment) {
	membuffer_init(&parser->buff);
	membuffer_ensure_space(&parser->buff, size);
	parser->str = str;
	parser->ptr = str;
	parser->ud = ud;
	parser->maxdepth = maxdepth;
	parser->curdepth = 0;
	parser->allowcomment = allowcomment;
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
#define getchar(p) (*(p)->ptr)
#define skipchar(p) (++(p)->ptr)
#define get_and_next(p) (*(p)->ptr++)
#define next_and_get(p) (*(++(p)->ptr))
#define savechar(p, c) membuffer_putc_unsafe(&(p)->buff, (c))
#define currpos(p) (size_t)((p)->ptr - (p)->str)

// 取解析到的错误内容
static const char* parser_error_content(json_parser_t *p) {
	size_t n = currpos(p);
	if (n > 50) n = 50;	// 调整这个数获得更长的内容
	membuffer_reset(&p->buff);
	membuffer_putb(&p->buff, p->ptr - n, n);
	membuffer_putc(&p->buff, '\0');
	return p->buff.b;
}

// 增加深度
static inline void parser_add_depth(json_parser_t *p) {
	p->curdepth++;
	if (p->curdepth >= p->maxdepth)
		parser_throw_error(p, "Too many nested data, max depth is %d, at: %s[:%lu]", p->maxdepth, 
			parser_error_content(p), currpos(p));
}

static inline void parser_skip_whitespaces(json_parser_t *p) {
	if (likely(!p->allowcomment)) {
		char ch = getchar(p);
		while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
			ch = next_and_get(p);
	} else {
		char ch = getchar(p);
		for (;;) {
			while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
				ch = next_and_get(p);
			if (ch == '/') {
				ch = next_and_get(p);
				if (ch == '/') {
					ch = next_and_get(p);
					while (ch != '\n' && ch != '\r' && ch != '\0')
						ch = next_and_get(p);
					continue;
				} else {
					parser_throw_error(p, "Invalid comment, at: %s[:%lu]", parser_error_content(p), currpos(p));
				}
			}
			break;
		}
	}
}

static inline void parser_expect_char(json_parser_t *p, char c) {
	if (likely(getchar(p) == c))
		skipchar(p);
	else
		parser_throw_error(p, "Expect '%c' at: %s[:%lu]", c, parser_error_content(p), currpos(p));
}

static inline void parser_process_false(json_parser_t *p) {
	if (likely(get_and_next(p) == 'a' && get_and_next(p) == 'l' &&  get_and_next(p) == 's' && 
		get_and_next(p) == 'e')) {
		ON_ADD_BOOLEAN(p->ud, 0);
	} else
		parser_throw_error(p, "Invalid boolean, at: %s[:%lu]", parser_error_content(p), currpos(p));
}

static inline void parser_process_true(json_parser_t *p) {
	if (likely(get_and_next(p) == 'r' && get_and_next(p) == 'u' &&  get_and_next(p) == 'e')) {
		ON_ADD_BOOLEAN(p->ud, 1);
	} else
		parser_throw_error(p, "Invalid boolean, at: %s[:%lu]", parser_error_content(p), currpos(p));
}

static inline void parser_process_null(json_parser_t *p) {
	if (likely(get_and_next(p) == 'u' && get_and_next(p) == 'l' &&  get_and_next(p) == 'l')) {
		ON_ADD_NULL(p->ud);
	} else
		parser_throw_error(p, "Invalid null, at: %s[:%lu]", parser_error_content(p), currpos(p));
}

static inline void parser_process_utf8esc(json_parser_t *p) {
	uint32_t cp = 0;
	char ch;
	int i, hex;
	for (i = 0; i < 4; ++i) {
		ch = get_and_next(p);
		if ('0' <= ch && ch <= '9')
			hex = ch - '0';
		else {
			ch |= 0x20;
			if ('a' <= ch && ch <= 'f')
				hex = 10 + ch - 'a';
			else {
				parser_throw_error(p, "Invalid utf8 escape sequence, at: %s[:%lu]", parser_error_content(p), currpos(p));
				return;
			}
		}
		cp |= hex << (12 - (i << 2));
	}
	char buff[UTF8BUFFSZ];
	int n = coutf8_encode(buff, cp);
	membuffer_putb_unsafe(&p->buff, buff + (UTF8BUFFSZ - n), n);
}

static const char escape2char[256] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0~19
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0, '\"',0,  0,  0,  0,  0,  // 20~39
	0,  0,  0,  0,  0,  0,  0, '/', 0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 40~59
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 60~79
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0, '\\',0,  0,  0,  0,  0, '\b',0,  // 80~99
	0,  0, '\f',0,  0,  0,  0,  0,  0,  0,  '\n',0,  0,  0, '\r',0, '\t',0,  0,  0,  // 100~119
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 120~139
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 140~159
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 160~179
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 180~199
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 200~219
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 220~239
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,                  // 240~256
};

static inline void parser_process_string(json_parser_t *p, int iskey) {
	membuffer_reset(&p->buff);
	char ch = get_and_next(p);
	for (;;) {
		if (ch == '\\') {
			unsigned char nch = (unsigned char)get_and_next(p);
			if (likely(escape2char[nch])) {
				savechar(p, escape2char[nch]);
			} else if (nch == 'u') {
				parser_process_utf8esc(p);
			} else {
				parser_throw_error(p, "Invalid escape sequence, at: %s[:%lu]", parser_error_content(p), currpos(p));
			}
		} else if (ch == '"') { 
			break;
		} else if ((unsigned char)ch < 32) {
			parser_throw_error(p, "Invalid string, at: %s[:%lu]", parser_error_content(p), currpos(p));
		} else {
			savechar(p, ch);
		}
		ch = get_and_next(p);
	}
	if (iskey)
		ON_BEGIN_PAIR(p->ud, p->buff.b, p->buff.sz);
	else
		ON_ADD_STRING(p->ud, p->buff.b, p->buff.sz);
}


static inline void parser_process_number(json_parser_t *p, char ch) {
#define invalid_number(p) parser_throw_error(p, "Invalid value, at: %s[:%lu]", parser_error_content(p), currpos(p))
#define MAXBY10		(int64_t)(INT64_MAX / 10)
#define MAXLASTD	(int)(INT64_MAX % 10)
static double powersOf10[] = {10., 100., 1.0e4, 1.0e8, 1.0e16, 1.0e32, 1.0e64, 1.0e128, 1.0e256};

	int64_t in = 0;			// 整型值
	double db = 0.0;		// 浮点数
	int isdouble = 0;			// 是否是浮点数
	int neg = 0;			// 是否是负数
	int eneg = 0;			// 指数部分是否负数
	int decimals = 0;		// 小数位数
	int exponent = 0;		// 指数位数

	membuffer_reset(&p->buff);
	if (ch == '-') {	// 负值
		neg = 1;
		ch = get_and_next(p);
	}
	if (ch == '0') {	// 0开头的后面只能是：.eE或结束
		ch = getchar(p);
	} else if (ch >= '1' && ch <= '9') {
		in = ch - '0';
		ch = getchar(p);
		int d;
		while (isdigit(ch)) {
			skipchar(p);
			d = ch - '0';
			if (in >= MAXBY10 && (in > MAXBY10 || d > MAXLASTD + neg)) {	// 更大的数字就用浮点数表示
				isdouble = 1;
				db = (double)in;
				break;
			}
			in = in * 10 + d;
			ch = getchar(p);
		}
	} else {
		invalid_number(p);		// 只能是0~9开头
	}

	if (isdouble) {	// 用浮点数表示大数
		while (isdigit(ch)) {
			skipchar(p);
			in = in * 10 + (ch - '0');
			ch = getchar(p);
		}
	}

	if (ch == '.') {	// 小数点部分
		if (!isdouble) {
			isdouble = 1;
			db = (double)in;
		}
		skipchar(p);
		ch = getchar(p);
		if (!isdigit(ch))
			invalid_number(p);  // .后面一定是数字
		do {
			skipchar(p);
			db = db * 10. + (ch - '0');
			decimals++;
			ch = getchar(p);
		} while (isdigit(ch));
		exponent -= decimals;
	}

	if (ch == 'e' || ch == 'E') {	// 指数部分
		if (!isdouble) {		// 有e强制认为是浮点数
			isdouble = 1;
			db = (double)in;
		}
		skipchar(p);
		ch = getchar(p);
		eneg = 0;
		if (ch == '-') {
			eneg = 1;
			skipchar(p);
			ch = getchar(p);
		} else if (ch == '+') {
			skipchar(p);
			ch = getchar(p);
		}
		if (!isdigit(ch))
			invalid_number(p);  // 后面一定是数字
		int exp = 0;
		do {
			skipchar(p);
			exp = exp * 10. + (ch - '0');
			ch = getchar(p);
		} while (isdigit(ch));
		if (eneg) exponent -= exp;
		else exponent += exp;
	}

	if (isdouble) {
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
		if (neg) db = -db;
		ON_ADD_FLOAT(p->ud, db);
	} else {
		if (neg) in = -in;
		ON_ADD_INTEGER(p->ud, neg);
	}
}

static inline void parser_process_value(json_parser_t *p);

static inline void parser_process_object(json_parser_t *p) {
	parser_add_depth(p);
	ON_ADD_OBJECT(p->ud);
	parser_skip_whitespaces(p);
	char ch = getchar(p);
	if (ch == '}') {
		skipchar(p);
		p->curdepth--;
		return;
	}
	for (;;) {
		parser_expect_char(p, '"');
		parser_process_string(p, 1);		// key

		parser_skip_whitespaces(p);
		parser_expect_char(p, ':');

		parser_process_value(p);			// value
		ON_END_PAIR(p->ud);

		parser_skip_whitespaces(p);
		if (getchar(p) == '}') {
			skipchar(p);
			p->curdepth--;
			return;
		}
		else {
			parser_expect_char(p, ',');
			parser_skip_whitespaces(p);
		}
	}
}

static inline void parser_process_array(json_parser_t *p) {
	parser_add_depth(p);
	ON_ADD_ARRAY(p->ud);
	parser_skip_whitespaces(p);
	char ch = getchar(p);
	if (ch == ']') {
		skipchar(p);
		p->curdepth--;
		return;
	}
	int i;
	for (i = 0; ;++i) {
		parser_process_value(p);
		ON_ADD_INDEX(p->ud, i);

		parser_skip_whitespaces(p);
		if (getchar(p) == ']') {
			skipchar(p);
			p->curdepth--;
			return;
		}
		else {
			parser_expect_char(p, ',');
		}
	}
}

static inline void parser_process_value(json_parser_t *p) {
	parser_skip_whitespaces(p);
	char ch = get_and_next(p);
	switch (ch) {
		case 'f': 
			parser_process_false(p);
			break;
		case 't':
			parser_process_true(p);
			break;
		case 'n': 
			parser_process_null(p);
			break;
		case '"':
			parser_process_string(p, 0);
			break;
		case '{':
			parser_process_object(p);
			break;
		case '[':
			parser_process_array(p);
			break;
		default:
			parser_process_number(p, ch);
			break;
	}
}

// 解析json文本
static void parser_do_parse(const char *str, size_t size, void *ud, int maxdepth, int allowcomment) {
	json_parser_t p;
	parser_init(&p, str, size, ud, maxdepth, allowcomment);
	//>>>if (setjmp(p.jb) == 0) {
		parser_process_value(&p);
		parser_skip_whitespaces(&p);
		if (getchar(&p) != '\0') {
			parser_throw_error(&p, "Expect '<eof>' but got '%c', at: %s[:%lu]", getchar(&p), 
				parser_error_content(&p), currpos(&p));
		}
		parser_free(&p);
	//>>>}
}

//-----------------------------------------------------------------------------
// dumpper

typedef struct {
	membuffer_t buff;	// 临时缓存
	int maxdepth;	// 最大层次
	int format;			// 是否格式化
	int empty_as_array; // 空表是否当成数组
	int num_as_str;		// 数字Key转为字符串
	char errmsg[ERRMSG_SIZE];	// 保存错误消息 
} json_dumpper_t;

// 足够转换数字的缓存大小
#define NUMBER_BUFF_SZ 44

// 抛出错误
static void dumpper_throw_error(json_dumpper_t *d, lua_State *L, const char *fmt, ...) {
	membuffer_free(&d->buff);
	va_list arg;
	va_start(arg, fmt);
	vsnprintf(d->errmsg, ERRMSG_SIZE, fmt, arg);
	va_end(arg);
	luaL_error(L, d->errmsg);
}

static void dumpper_dump_integer(json_dumpper_t *d, lua_State *L, int idx) {
	lua_Integer in = lua_tointeger(L, idx);
	membuffer_ensure_space(&d->buff, NUMBER_BUFF_SZ);
	char *p = membuffer_getp(&d->buff);
	int len = sprintf(p, LUA_INTEGER_FMT, in);
	membuffer_add_size(&d->buff, len);
}

static void dumpper_dump_number(json_dumpper_t *d, lua_State *L, int idx) {
	lua_Number num = lua_tonumber(L, idx);
	 if (isinf(num) || isnan(num))
		 dumpper_throw_error(d, L, "The number is NaN or Infinity");
	membuffer_ensure_space(&d->buff, NUMBER_BUFF_SZ);
	char *p = membuffer_getp(&d->buff);
	int len = sprintf(p, LUA_NUMBER_FMT, num);
	membuffer_add_size(&d->buff, len);
}

// 字符转义表
static const char *char2escape[128] = {
	"\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006", "\\u0007",
	"\\b", "\\t", "\\n", "\\u000b", "\\f", "\\r", "\\u000e", "\\u000f",
	"\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017",
	"\\u0018", "\\u0019", "\\u001a", "\\u001b", "\\u001c", "\\u001d", "\\u001e", "\\u001f",
	NULL, NULL, "\\\"", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "\\\\", NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "\\u007f",
};

static void dumpper_dump_string(json_dumpper_t *d, lua_State *L, int idx) {
	membuffer_t *buff = &d->buff;
	size_t len, i;
	const char *str = lua_tolstring(L, idx, &len);
	membuffer_ensure_space(buff, len * 6 + 2);
	membuffer_putc_unsafe(buff, '\"');
	const char *esc;
	unsigned char ch;
	for (i = 0; i < len; ++i) {
		ch = (unsigned char)str[i];
		if (ch < 128) {
			esc = char2escape[ch];
			if (!esc) 
				membuffer_putc_unsafe(buff, (char)ch);
			else {
				while (*esc != '\0') {
					membuffer_putc_unsafe(buff, *esc);
					++esc;
				}
			}
		} else {
			membuffer_putc_unsafe(buff, (char)ch);
		}
	}
	membuffer_putc_unsafe(buff, '\"');
}

static void dumpper_dump_value(json_dumpper_t *d, lua_State *L, int depth);

static int dumpper_check_array(json_dumpper_t *d, lua_State *L, int *len) {
	int asize = lua_rawlen(L, -1);
	if (asize > 0) {
		lua_pushinteger(L, asize);
		if (lua_next(L, -2) == 0) {
			*len = asize;
			return 1;
		} else {
			lua_pop(L, 2);
			return 0;
		}
	} else {
		lua_pushnil(L);
		if (lua_next(L, -2) == 0) {
			*len = asize;
			return d->empty_as_array;
		} else {
			lua_pop(L, 2);
			return 0;
		}
	}
}

static inline void dumpper_dump_indent(json_dumpper_t *d, int count) {
	membuffer_ensure_space(&d->buff, count);
	int i;
	for (i = 0; i < count; ++i)
		membuffer_putc_unsafe(&d->buff, '\t');
}

static void dumpper_dump_array(json_dumpper_t *d, lua_State *L, int len, int depth) {
	membuffer_t *buff = &d->buff;
	membuffer_putc(buff, '[');

	int i;
	for (i = 1; i <= len; ++i) {
		if (d->format && i == 1) membuffer_putc(buff, '\n');
		lua_rawgeti(L, -1, i);
		if (d->format) dumpper_dump_indent(d, depth);
		dumpper_dump_value(d, L, depth);
		lua_pop(L, 1);
		if (i < len)
			membuffer_putc(buff, ',');
		if (d->format) membuffer_putc(buff, '\n');
	}

	if (d->format && i > 1)  dumpper_dump_indent(d, depth-1);
	membuffer_putc(buff, ']');
}

static void dumpper_dump_object(json_dumpper_t *d, lua_State *L, int depth) {
	membuffer_t *buff = &d->buff;
	membuffer_putc(buff, '{');

	int ktp;
	int comma = 0;
	lua_pushnil(L);		// t nil
	while (lua_next(L, -2) != 0) {	// t k v
		if (comma) {
			membuffer_putc(buff, ',');
			if (d->format) membuffer_putc(buff, '\n');
		} else {
			comma = 1;
			if (d->format) membuffer_putc(buff, '\n');
		} 
		// key
		ktp = lua_type(L, -2);
		if (ktp == LUA_TSTRING) {
			if (d->format) dumpper_dump_indent(d, depth);
			dumpper_dump_string(d, L, -2);
			if (!d->format)
				membuffer_putc(buff, ':');
			else
				membuffer_putb(buff, " : ", 3);
		} else if (ktp == LUA_TNUMBER && d->num_as_str) {
			if (d->format) dumpper_dump_indent(d, depth);
			membuffer_putc(buff, '\"');
			if (lua_isinteger(L, -2))
				dumpper_dump_integer(d, L, -2);
			else
				dumpper_dump_number(d, L, -2);
			if (!d->format)
				membuffer_putb(buff, "\":", 2);
			else
				membuffer_putb(buff, "\" : ", 4);
		} else {
			dumpper_throw_error(d, L, "Table key must be a string");
		}
		// value
		dumpper_dump_value(d, L, depth);
		lua_pop(L, 1);
	}
	if (d->format && comma) {
		membuffer_putc(buff, '\n');
		dumpper_dump_indent(d, depth-1);
	} 
	membuffer_putc(buff, '}');
}

static inline void dumpper_dump_table(json_dumpper_t *d, lua_State *L, int depth) {
	depth++;
	if (depth > d->maxdepth)
		dumpper_throw_error(d, L, "Too many nested data, max depth is %d", d->maxdepth);
	luaL_checkstack(L, 6, NULL);

	int len;
	if (dumpper_check_array(d, L, &len))
		dumpper_dump_array(d, L, len, depth);
	else
		dumpper_dump_object(d, L, depth);
}

static void dumpper_dump_value(json_dumpper_t *d, lua_State *L, int depth) {
	int tp = lua_type(L, -1);
	switch (tp) {
		case LUA_TSTRING:
			dumpper_dump_string(d, L, -1);
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1))
				dumpper_dump_integer(d, L, -1);
			else
				dumpper_dump_number(d, L, -1);
			break;
		case LUA_TBOOLEAN:
			if (lua_toboolean(L, -1))
				membuffer_putb(&d->buff, "true", 4);
			else
				membuffer_putb(&d->buff, "false", 5);
			break;
		case LUA_TTABLE:
			dumpper_dump_table(d, L, depth);
			break;
		case LUA_TNIL:
			membuffer_putb(&d->buff, "null", 4);
			break;
		case LUA_TLIGHTUSERDATA:
			if (lua_touserdata(L, -1) == NULL) {
				membuffer_putb(&d->buff, "null", 4);
				break;
			}
			goto error;
		default:
		error:
			dumpper_throw_error(d, L, "Unsupport type %s", lua_typename(L, tp));
	}
}

//-----------------------------------------------------------------------------
// 接口
#define DEF_MAX_DEPTH 128

// 从字符串加载：json.load(str, maxdepth) -> obj
// 要求字符串必须以0结尾
static int l_load(lua_State *L) {
	size_t size;
	const char *str = luaL_checklstring(L, 1, &size);
	int maxdepth = (int)luaL_optinteger(L, 2, DEF_MAX_DEPTH);
	int allowcomment = lua_toboolean(L, 3);
	parser_do_parse(str, size, L, maxdepth, allowcomment);
	return 1;
}

// 保存到字符串: json.dump(obj) -> str
static int l_dump(lua_State *L) {
	luaL_checkany(L, 1);
	json_dumpper_t dumpper;
	membuffer_init(&dumpper.buff);
	dumpper.format = lua_toboolean(L, 2);
	dumpper.empty_as_array = lua_toboolean(L, 3);
	dumpper.num_as_str = lua_toboolean(L, 4);
	dumpper.maxdepth = (int)luaL_optinteger(L, 5, DEF_MAX_DEPTH);

	lua_settop(L, 1);
	dumpper_dump_value(&dumpper, L, 0);
	lua_pushlstring(L, dumpper.buff.b, dumpper.buff.sz);
	membuffer_free(&dumpper.buff);
	return 1;
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