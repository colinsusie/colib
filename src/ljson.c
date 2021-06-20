/**
 * Json解析器：只支持utf-8格式
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
#include "lua.h"
#include "lauxlib.h"
#include "coconf.h"
#include "costream.h"
#include "coutf8.h"

//-----------------------------------------------------------------------------
// parser

//-------------------------------------
// 与Lua相关的代码
typedef struct loaddata {
	lua_State *L;
	filereader_t *freader;
} loaddata_t;

static inline void l_add_object(loaddata_t *lt) {
	lua_checkstack(lt->L, 10);
	lua_newtable(lt->L);
}
static inline void l_begin_pair(loaddata_t *lt, const char *k, size_t sz) {
	lua_pushlstring(lt->L, k, sz);
}
static inline void l_end_pair(loaddata_t *lt) {
	lua_rawset(lt->L, -3);
}
static inline void l_add_array(loaddata_t *lt) {
	lua_checkstack(lt->L, 10);
	lua_newtable(lt->L);
}
static inline void l_add_index(loaddata_t *lt, int i) {
	lua_rawseti(lt->L, -2, i+1);
}
static inline void l_add_string(loaddata_t *lt, const char *s, size_t sz) {
	lua_pushlstring(lt->L, s, sz);
}
static inline void l_add_float(loaddata_t *lt, double f) {
	lua_pushnumber(lt->L, (lua_Number)f);
}
static inline void l_add_integer(loaddata_t *lt, int64_t i) {
	lua_pushinteger(lt->L, (lua_Integer)i);
}
static inline void l_add_boolean(loaddata_t *lt, int b) {
	lua_pushboolean(lt->L, b);
}
static inline void l_add_null(loaddata_t *lt) {
	lua_pushlightuserdata(lt->L, NULL);
}
static inline void l_error(loaddata_t *lt, const char *msg) {
	if (lt->freader)	// 先关掉文件流
		ifilestream_close(lt->freader);
	luaL_error(lt->L, msg);
}

// 解析事件
#define ON_ADD_OBJECT(ud) l_add_object((loaddata_t*)(ud))
#define ON_BEGIN_PAIR(ud, k, sz) l_begin_pair((loaddata_t*)(ud), k, sz)
#define ON_END_PAIR(ud) l_end_pair((loaddata_t*)(ud))
#define ON_ADD_ARRAY(ud) l_add_array((loaddata_t*)(ud))
#define ON_ADD_INDEX(ud, i) l_add_index((loaddata_t*)(ud), i)
#define ON_ADD_STRING(ud, s, sz) l_add_string((loaddata_t*)(ud), s, sz)
#define ON_ADD_FLOAT(ud, f) l_add_float((loaddata_t*)(ud), f)
#define ON_ADD_INTEGER(ud, i) l_add_integer((loaddata_t*)(ud), i)
#define ON_ADD_BOOLEAN(ud, b) l_add_boolean((loaddata_t*)(ud), b)
#define ON_ADD_NULL(ud) l_add_null((loaddata_t*)(ud))
#define ON_ERROR(ud, msg) l_error((loaddata_t*)(ud), msg)

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
	"<boolean>", "<null>", ":", ",", "<eof>", "", 
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
	int c;				// 当前解析到的字符
	json_token_t tk;	// 当前的Token
	istream_t *stm;		// 提供内容的流
	void *ud;			// 解析事件的用户数据
	membuffer_t buff;	// 临时缓存
	uint16_t curdepth;	// 当前层次
	uint16_t maxdepth;	// 最大层次
	char errmsg[ERRMSG_SIZE];	// 保存错误消息 
	//>>>jmp_buf jb;			// 用于实现从解析中出错直接跳出
} json_parser_t;

static inline void parser_init(json_parser_t *parser, istream_t *stm, void *ud, 
	uint16_t maxdepth) {
	membuffer_init(&parser->buff);
	parser->stm = stm;
	parser->ud = ud;
	parser->c = istream_getc(stm);
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
#define nextchar(p) ((p)->c = istream_getc(p->stm))
#define savechar(p, c) membuffer_putc(&(p)->buff, c)
#define savecurr(p) savechar((p), (p)->c)
#define popchar(p, n) membuffer_remove(&(p)->buff, n)
#define save_and_next(p) (savecurr(p), nextchar(p))
#define currpos(p) (unsigned long)istream_pos((p)->stm)
#define errmsg(p) (savechar(p, '\0'), p->buff.b)

// 忽略空白字符
static inline void skip_whitespace(json_parser_t *p) {
	while (1) {
		switch (p->c) {
			case '\n':  case '\r':  case '\t': case ' ': break;
			default: return;
		}
		nextchar(p);
	}
}

// 检查单词是不是w
static inline int check_word(json_parser_t *p, const char *w) {
	int c;
	while ((c = *w++) != '\0') {
		if (p->c != c) {
			if (p->c != EOF) savecurr(p);	
			return 0;
		}
		save_and_next(p);
	}
	return 1;
}

// 解析数字
static void parser_parse_number(json_parser_t *p) {
	// 提取数字，并对数字进行正确性验证
	#define ST_MI 1
	#define ST_ZE 2
	#define ST_IN 3
	#define ST_FR 4
	#define ST_FS 5
	#define ST_E1 6
	#define ST_E2 7
	#define ST_E3 8
	#define invalid_number(p) parser_throw_error(p, "Invalid number '%s', at: %lu", errmsg(p), currpos(p))
	int isfloat = 0;
	int st, c;
	if (p->c == '-') st = ST_MI;
	else if (p->c == '0') st = ST_ZE;
	else st = ST_IN;
	for (;;) {
		save_and_next(p);
		c = p->c;
		switch (st) {
			case ST_MI: 
				if (c == '0') st = ST_ZE;
				else if (c >= '1' && c <= '9') st = ST_IN;
				else invalid_number(p);
				break;
			case ST_ZE:
				isfloat = 1;
				if (c == '.') st = ST_FR;
				else if (c == 'e' || c == 'E') st = ST_E1;
				else goto convert;
				break;
			case ST_IN:
				if (c == '.') st = ST_FR;
				else if (c >= '0' && c <= '9') st = ST_IN;
				else if (c == 'e' || c == 'E') st = ST_E1;
				else goto convert;
				break;
			case ST_FR:
				isfloat = 1;
				if (c >= '0' && c <= '9') st = ST_FS;
				else invalid_number(p);
				break;
			case ST_FS:
				if (c >= '0' && c <= '9') st = ST_FS;
				else if (c == 'e' || c == 'E') st = ST_E1;
				else goto convert;
				break;
			case ST_E1:
				isfloat = 1;
				if (c >= '+' && c <= '-') st = ST_E2;
				else if (c >= '0' && c <= '9') st = ST_E3;
				else invalid_number(p);
				break;
			case ST_E2:
				if (c >= '0' && c <= '9') st = ST_E3;
				else invalid_number(p);
				break;
			case ST_E3:
				if (c >= '0' && c <= '9') st = ST_E3;
				else goto convert;
				break;
		}
	}

convert:
	savechar(p, '\0');
	char *endptr;
	errno = 0;
	if (isfloat) {
		p->tk.type = TK_FLOAT;
		double val = strtod(p->buff.b, &endptr);
		if (p->buff.b + p->buff.sz - 1 != endptr)
			parser_throw_error(p, "Invalid float '%s', at: %lu", errmsg(p), currpos(p));
		else if (errno == ERANGE)
			parser_throw_error(p, "Float overflow '%s', at: %lu", errmsg(p), currpos(p));
		p->tk.value.d = val;
		
	} else {
		p->tk.type = TK_INTEGER;
		long long val = strtoll(p->buff.b, &endptr, 10);
		if (p->buff.b + p->buff.sz - 1 != endptr)
			parser_throw_error(p, "Invalid integer '%s', at: %lu", errmsg(p), currpos(p));
		else if (errno == ERANGE)
			parser_throw_error(p, "Integer overflow '%s', at: %lu", errmsg(p), currpos(p));
		p->tk.value.i = (int64_t)val;
	}
}

// 解析utf8转义
static void parser_parse_utf8esc(json_parser_t *p) {
	save_and_next(p);		// skip u
	uint32_t cp;
	int hex[4];
	int i, c;
	for (i = 0; i < 4; ++i) {
		c = p->c;
		if (c == EOF) return;
		if (!isxdigit(c)) {
			savecurr(p);
			parser_throw_error(p, "Invalid utf8 escape sequence: '%s', at: %lu", errmsg(p), currpos(p));
		}
		hex[i] = isdigit(c) ? c - '0' : (tolower(c) - 'a') + 10;
		save_and_next(p);
	}
	cp = (hex[0] << 12) | (hex[1] << 8) | (hex[2] << 4) | hex[3];
	popchar(p, 6);	// pop \uXXXX

	char buff[UTF8BUFFSZ];
	int n = coutf8_encode(buff, cp);
	for (; n > 0; n--)
		savechar(p, buff[UTF8BUFFSZ - n]);
}

// 解析字符串
static void parser_parse_string(json_parser_t *p) {
	nextchar(p);		// skip "
	while (p->c != '"') {
		switch (p->c) {
			case '\\': {	// escape
				int c;
				save_and_next(p);
				switch (p->c) {
					case 'b': c = '\b'; goto read_save;
					case 'f': c = '\f'; goto read_save;
					case 'n': c = '\n'; goto read_save;
					case 'r': c = '\r'; goto read_save;
					case 't': c = '\t'; goto read_save;
					case '/': c = '/';  goto read_save;
					case '\\': c = '\\';  goto read_save;
					case '"': c = '"';  goto read_save;
					case 'u': parser_parse_utf8esc(p);  goto no_save;
					case EOF: goto no_save;
					default:
						c = 0;			// compilation warning
						savecurr(p);
						parser_throw_error(p, "Invalid escape sequence: '%s', at: %lu", errmsg(p), currpos(p));
				}
				read_save: 
					nextchar(p);
					popchar(p, 1);
					savechar(p, c);
				no_save: break;
			}
			case EOF: case '\n': case '\r':
				parser_throw_error(p, "Unfinished string: '%s', at: %lu", errmsg(p), currpos(p));
				break;
			default:
				save_and_next(p);
		}
	}
	nextchar(p);	// skip "
	p->tk.type = TK_STRING;
	p->tk.strsz = p->buff.sz;
	p->tk.value.s = p->buff.b;
}

// 取下一个token
static void parser_next_token(json_parser_t *p) {
	skip_whitespace(p);
	membuffer_reset(&p->buff);
	switch (p->c) {
		case '{':
			nextchar(p); 
			p->tk.type = TK_OBJ_BEGIN;
			return;
		case '}':
			nextchar(p); 
			p->tk.type = TK_OBJ_END;
			return; 
		case '[':
			nextchar(p); 
			p->tk.type = TK_ARY_BEGIN;
			return;
		case ']':
			nextchar(p); 
			p->tk.type = TK_ARY_END;
			return;
		case '"':
			parser_parse_string(p);
			return;
		case '-': case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			parser_parse_number(p);
			return;
		case 'f':
			if (check_word(p, "false")) {
				p->tk.type = TK_BOOLEAN;
				p->tk.value.b = 0;
				return;
			} else {
				parser_throw_error(p, "Invalid boolean: '%s', at: %lu", errmsg(p), currpos(p));
				return;
			}
		case 't':
			if (check_word(p, "true")) {
				p->tk.type = TK_BOOLEAN;
				p->tk.value.b = 1;
				return;
			} else {
				parser_throw_error(p, "Invalid boolean: '%s', at: %lu", errmsg(p), currpos(p));
				return;
			}
		case 'n':
			if (check_word(p, "null")) {
				p->tk.type = TK_NULL;
				return;
			} else {
				parser_throw_error(p, "Invalid null: '%s', at: %lu", errmsg(p), currpos(p));
				return;
			}
		case ':':
			nextchar(p); 
			p->tk.type = TK_COLON;
			return;
		case ',':
			nextchar(p); 
			p->tk.type = TK_COMMA;
			return;
		case EOF:
			p->tk.type = TK_END;
			return;
		default:
			parser_throw_error(p, "Invalid token: '%c', at: %lu", p->c, currpos(p));
			return;
	}
}

static void parser_parse_value(json_parser_t *parser);

// 增加深度
static inline void parser_add_depth(json_parser_t *p) {
	p->curdepth++;
	if (p->curdepth >= p->maxdepth)
		parser_throw_error(p, "Too many nested data, max depth is %d, at: %lu", p->maxdepth, currpos(p));
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
			parser_throw_error(p, "Expect '<object key(string)>' but got '%s', at: %lu", token_tname[p->tk.type], currpos(p));
		
		ON_BEGIN_PAIR(p->ud, p->tk.value.s, p->tk.strsz);
		parser_next_token(p);
		if (p->tk.type != TK_COLON)
			parser_throw_error(p, "Expect ':' but got '%s', at: %lu", token_tname[p->tk.type], currpos(p));
		parser_next_token(p);
		parser_parse_value(p);
		ON_END_PAIR(p->ud);

		parser_next_token(p);
		if (p->tk.type == TK_OBJ_END) {
			p->curdepth--;
			return;
		} else if (p->tk.type != TK_COMMA)
			parser_throw_error(p, "Expect ',' but got '%s', at: %lu", token_tname[p->tk.type], currpos(p));
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
			parser_throw_error(p, "Expect ',' but got '%s', at: %lu", token_tname[p->tk.type], currpos(p));
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
			parser_throw_error(p, "Expect '<json value>' but got '%s', at: %lu", token_tname[p->tk.type], currpos(p));
			break;
	}
}

// 解析json文本
static void parser_do_parse(istream_t *stm, void *ud, uint16_t maxdepth) {
	json_parser_t parser;
	parser_init(&parser, stm, ud, maxdepth);
	//>>>if (setjmp(parser.jb) == 0) {
		parser_next_token(&parser);
		parser_parse_value(&parser);
		parser_next_token(&parser);
		if (parser.tk.type != TK_END) {
			parser_throw_error(&parser, "Expect '<eof>' but got '%s', at: %lu", token_tname[parser.tk.type], currpos(&parser));
		}
		parser_free(&parser);
	//>>>}
}

//-----------------------------------------------------------------------------
// dump



//-----------------------------------------------------------------------------
// 接口

#define DEF_MAX_DEPTH 128
#define DEF_FILE_BUFFSZ 4096

// 从字符串加载：json.load(str) -> obj
static int l_load(lua_State *L) {
	size_t sz;
	const char *str = luaL_checklstring(L, 1, &sz);
	uint16_t maxdepth = (uint16_t)luaL_optinteger(L, 2, DEF_MAX_DEPTH);
	istream_t stm;
	strreader_t sreader;
	istrstream_init(&stm, &sreader, str, sz);
	loaddata_t lt = {L, NULL};
	parser_do_parse(&stm, &lt, maxdepth);
	return 1;
}

// 从文件加载：json.loadf(fname) -> obj
static int l_loadf(lua_State *L) {
	const char *fname = luaL_checkstring(L, 1);
	uint16_t maxdepth = (uint16_t)luaL_optinteger(L, 2, DEF_MAX_DEPTH);
	istream_t stm;
	filereader_t freader;
	if (!ifilestream_initf(&stm, &freader, fname, DEF_FILE_BUFFSZ))
		luaL_error(L, "loadf failed: %s", strerror(errno));
	ifilestream_skipbom(&freader);
	loaddata_t lt = {L, &freader};
	parser_do_parse(&stm, &lt, maxdepth);
	ifilestream_close(&freader);
	return 1;
}

// 保存到字符串: json.dump(obj) -> str
static int l_dump(lua_State *L) {
	return 0;
}

// 保存到文件：json.dumpf(obj, fname)
static int l_dumpf(lua_State *L) {
	return 0;
}

static const luaL_Reg lib[] = {
	{"load", l_load},
	{"loadf", l_loadf},
	{"dump", l_dump},
	{"dumpf", l_dumpf},
	{NULL, NULL},
};

LUAMOD_API int luaopen_colibc_json(lua_State *L) {
	luaL_newlib(L, lib);
	// json.null
    lua_pushlightuserdata(L, NULL);
    lua_setfield(L, -2, "null");
	return 1;
}