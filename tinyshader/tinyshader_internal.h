#ifndef TINYSHADER_INTERNAL_H
#define TINYSHADER_INTERNAL_H

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "spirv.h"
#include "GLSL.std.450.h"
#include "tinyshader.h"

// Rounds to the next multiple of four
#define ROUND_TO_4(x) (((x) + 3) & ~0x03)

#define NEW(compiler, type) ts__bumpZeroAlloc(&(compiler)->alloc, sizeof(type))
#define NEW_ARRAY(compiler, type, count)                                                 \
    ts__bumpZeroAlloc(&(compiler)->alloc, sizeof(type) * (count))

#define arrFree(a) ((a) ? free(arr__sbraw(a)), 0 : 0)
#define arrPush(a, v) (arr__sbmaybegrow(a, 1), (a)[arr__sbn(a)++] = (v))
#define arrPop(a) (--arr__sbn(a))
#define arrLength(a) ((a) ? arr__sbn(a) : 0)
#define arrAdd(a, n) (arr__sbmaybegrow(a, n), arr__sbn(a) += (n), &(a)[arr__sbn(a) - (n)])
#define arrLast(a) (&((a)[arr__sbn(a) - 1]))

#define arr__sbraw(a) ((uint32_t *)(void *)(a)-2)
#define arr__sbm(a) arr__sbraw(a)[0]
#define arr__sbn(a) arr__sbraw(a)[1]

#define arr__sbneedgrow(a, n) ((a) == 0 || arr__sbn(a) + (n) >= arr__sbm(a))
#define arr__sbmaybegrow(a, n) (arr__sbneedgrow(a, (n)) ? arr__sbgrow(a, n) : 0)
#define arr__sbgrow(a, n) (*((void **)&(a)) = arr__sbgrowf((a), (n), sizeof(*(a))))

static void *arr__sbgrowf(void *arr, uint32_t increment, uint32_t itemsize)
{
    uint32_t dbl_cur = arr ? 2 * arr__sbm(arr) : 0;
    uint32_t min_needed = arrLength(arr) + increment;
    uint32_t m = dbl_cur > min_needed ? dbl_cur : min_needed;
    uint32_t *p = (uint32_t *)realloc(
        arr ? arr__sbraw(arr) : 0, itemsize * m + sizeof(uint32_t) * 2);
    if (p)
    {
        if (!arr) p[1] = 0;
        p[0] = m;
        return p + 2;
    }
    else
    {
        return (
            void *)(2 * sizeof(uint32_t)); // try to force a NULL pointer exception later
    }
}

////////////////////////////////
//
// Type definitons
//
////////////////////////////////

typedef struct HashMap
{
    const char **keys;
    uint64_t *hashes;
    uint64_t *indices;
    uint64_t size;

    /*array*/ void **values;
} HashMap;

typedef struct BumpBlock
{
    unsigned char *data;
    size_t size;
    size_t pos;
    struct BumpBlock *next;
} BumpBlock;

typedef struct BumpAlloc
{
    size_t block_size;
    size_t last_block_size;
    BumpBlock base_block;
    BumpBlock *last_block;
} BumpAlloc;

typedef struct StringBuilder
{
    char *buf;
    char *scratch;
    size_t len;
    size_t cap;
} StringBuilder;

typedef struct File File;
typedef struct Module Module;

typedef struct Scope Scope;

typedef struct AstExpr AstExpr;
typedef struct AstStmt AstStmt;
typedef struct AstDecl AstDecl;

typedef struct Location
{
    File *file;
    uint32_t pos;
    uint32_t length;
    uint32_t line;
    uint32_t col;
} Location;

typedef struct Error
{
    Location loc;
    const char *message;
} Error;

//
// Token
//

typedef enum TokenKind {
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACK,
    TOKEN_RBRACK,
    TOKEN_LCURLY,
    TOKEN_RCURLY,
    TOKEN_ATTR_LBRACK,
    TOKEN_ATTR_RBRACK,

    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_COLON_COLON,

    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_MOD,

    TOKEN_BITOR,
    TOKEN_BITXOR,
    TOKEN_BITAND,
    TOKEN_BITNOT,

    TOKEN_LSHIFT,
    TOKEN_RSHIFT,

    TOKEN_PERIOD,
    TOKEN_COMMA,

    TOKEN_NOT,    // !
    TOKEN_ASSIGN, // =

    TOKEN_EQUAL,     // ==
    TOKEN_NOTEQ,     // !=
    TOKEN_LESS,      // <
    TOKEN_LESSEQ,    // <=
    TOKEN_GREATER,   // >
    TOKEN_GREATEREQ, // >=

    TOKEN_ADDEQ, // +=
    TOKEN_SUBEQ, // -=
    TOKEN_MULEQ, // *=
    TOKEN_DIVEQ, // /=
    TOKEN_MODEQ, // %=

    TOKEN_BITANDEQ, // &=
    TOKEN_BITOREQ,  // |=
    TOKEN_BITXOREQ, // ^=

    TOKEN_LSHIFTEQ, // <<=
    TOKEN_RSHIFTEQ, // >>=

    TOKEN_AND, // &&
    TOKEN_OR,  // ||

    TOKEN_IDENT,
    TOKEN_IN,
    TOKEN_OUT,
    TOKEN_INOUT,
    TOKEN_IMPORT,
    TOKEN_STRUCT,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_RETURN,
    TOKEN_CONST,
    TOKEN_CONSTANT_BUFFER,
    TOKEN_SAMPLER_STATE,
    TOKEN_TEXTURE_1D,
    TOKEN_TEXTURE_2D,
    TOKEN_TEXTURE_3D,
    TOKEN_TEXTURE_CUBE,
    TOKEN_SAMPLED_TEXTURE_1D,
    TOKEN_SAMPLED_TEXTURE_2D,
    TOKEN_SAMPLED_TEXTURE_3D,
    TOKEN_SAMPLED_TEXTURE_CUBE,

    TOKEN_DOT_BUILTIN,
    TOKEN_CROSS_BUILTIN,
    TOKEN_LENGTH_BUILTIN,
    TOKEN_DEGREES_BUILTIN,
    TOKEN_RADIANS_BUILTIN,
    TOKEN_MUL_BUILTIN,

    TOKEN_INT_LIT,
    TOKEN_FLOAT_LIT,
    TOKEN_STRING_LIT,

    TOKEN_BOOL,

    TOKEN_UINT,
    TOKEN_INT,
    TOKEN_FLOAT,

    TOKEN_VOID,

    TOKEN_FALSE,
    TOKEN_TRUE,

    TOKEN_VECTOR_TYPE,
    TOKEN_MATRIX_TYPE,
} TokenKind;

typedef struct Token
{
    TokenKind kind;
    Location loc;
    union
    {
        char *str;
        double double_;
        int64_t int_;
        struct
        {
            TokenKind elem_type;
            uint8_t dim;
        } vector_type;
        struct
        {
            TokenKind elem_type;
            uint8_t dim1;
            uint8_t dim2;
        } matrix_type;
    };
} Token;

//
// IR
//

typedef struct IRDecoration
{
    SpvDecoration kind;
    union
    {
        uint32_t value;
    };
} IRDecoration;

typedef struct IRMemberDecoration
{
    SpvDecoration kind;
    uint32_t member_index;
    union
    {
        uint32_t value;
    };
} IRMemberDecoration;

typedef enum IRTypeKind {
    IR_TYPE_VOID,

    IR_TYPE_BOOL,
    IR_TYPE_FLOAT,
    IR_TYPE_INT,

    IR_TYPE_VECTOR,
    IR_TYPE_MATRIX,

    IR_TYPE_POINTER,
    IR_TYPE_FUNC,
    IR_TYPE_STRUCT,

    IR_TYPE_SAMPLER,
    IR_TYPE_IMAGE,
    IR_TYPE_SAMPLED_IMAGE,
} IRTypeKind;

typedef struct IRType
{
    IRTypeKind kind;
    char *string;
    uint32_t id;

    union
    {
        struct
        {
            uint32_t bits;
        } float_;
        struct
        {
            uint32_t bits;
            bool is_signed;
        } int_;
        struct
        {
            SpvStorageClass storage_class;
            struct IRType *sub;
        } ptr;
        struct
        {
            uint32_t size;
            struct IRType *elem_type;
        } vector;
        struct
        {
            uint32_t col_count;
            struct IRType *col_type;
        } matrix;
        struct
        {
            struct IRType *return_type;

            struct IRType **params;
            uint32_t param_count;
        } func;
        struct
        {
            char *name;

            struct IRType **fields;
            uint32_t field_count;
            IRDecoration *decorations;
            uint32_t decoration_count;
            IRMemberDecoration *field_decorations;
            uint32_t field_decoration_count;
        } struct_;
        struct
        {
            struct IRType *sampled_type;
            SpvDim dim;
            uint32_t depth;
            uint32_t arrayed;
            uint32_t multisampled;
            uint32_t sampled;
            SpvImageFormat format;
        } image;
        struct
        {
            struct IRType *image_type;
        } sampled_image;
    };
} IRType;

typedef struct IRModule IRModule;
typedef struct IRInst IRInst;

typedef enum IRInstKind {
    IR_INST_ENTRY_POINT,
    IR_INST_FUNCTION,
    IR_INST_BLOCK,
    IR_INST_FUNC_PARAM,
    IR_INST_VARIABLE,
    IR_INST_CONSTANT,
    IR_INST_CONSTANT_BOOL,
    IR_INST_RETURN,
    IR_INST_STORE,
    IR_INST_LOAD,
    IR_INST_ACCESS_CHAIN,
    IR_INST_FUNC_CALL,
    IR_INST_BRANCH,
    IR_INST_COND_BRANCH,

    IR_INST_BUILTIN_CALL,
    IR_INST_CAST,
    IR_INST_COMPOSITE_CONSTRUCT,
    IR_INST_COMPOSITE_EXTRACT,
    IR_INST_VECTOR_SHUFFLE,

    IR_INST_SAMPLE_IMPLICIT_LOD,

    IR_INST_UNARY,
    IR_INST_BINARY,
} IRInstKind;

typedef enum IRBuiltinInstKind {
    IR_BUILTIN_DOT,
    IR_BUILTIN_CROSS,
    IR_BUILTIN_LENGTH,
    IR_BUILTIN_MUL,
    IR_BUILTIN_DEGREES,
    IR_BUILTIN_RADIANS,
    IR_BUILTIN_CREATE_SAMPLED_IMAGE,
} IRBuiltinInstKind;

struct IRInst
{
    IRInstKind kind;
    uint32_t id;
    IRType *type;
    /*array*/ IRDecoration *decorations;

    union
    {
        struct
        {
            IRInst *func;
            char *name;
            SpvExecutionModel execution_model;
            IRInst **globals;
            uint32_t global_count;
        } entry_point;

        struct
        {
            /*array*/ IRInst **params;
            /*array*/ IRInst **blocks;
            /*array*/ IRInst **inputs;
            /*array*/ IRInst **outputs;
        } func;

        struct
        {
            IRInst *func;
            /*array*/ IRInst **insts;
        } block;

        struct
        {
            SpvStorageClass storage_class;
            IRInst *initializer;
        } var;

        struct
        {
            void *value;
            size_t value_size_bytes;
        } constant;

        struct 
        {
            bool value;
        } constant_bool;

        struct
        {
            IRInst *value;
        } return_;

        struct
        {
            IRInst *pointer;
            IRInst *value;
        } store;

        struct
        {
            IRInst *pointer;
        } load;

        struct
        {
            IRInst *base;
            IRInst **indices;
            uint32_t index_count;
        } access_chain;

        struct
        {
            IRInst *func;
            IRInst **params;
            uint32_t param_count;
        } func_call;

        struct
        {
            bool is_by_reference;
        } func_param;

        struct
        {
            IRBuiltinInstKind kind;
            IRInst **params;
            uint32_t param_count;
        } builtin_call;

        struct
        {
            SpvOp op;
            IRType *dst_type;
            IRInst *value;
            bool redundant;
        } cast;

        struct
        {
            IRInst **fields;
            uint32_t field_count;
        } composite_construct;

        struct
        {
            IRInst *value;
            uint32_t *indices;
            uint32_t index_count;
        } composite_extract;

        struct
        {
            IRInst *vector_a;
            IRInst *vector_b;
            uint32_t *indices;
            uint32_t index_count;
        } vector_shuffle;

        struct
        {
            IRInst *image_sampler;
            IRInst *coords;
        } sample;

        struct
        {
            IRInst *right;
            SpvOp op;
        } unary;

        struct
        {
            IRInst *left;
            IRInst *right;
            SpvOp op;
        } binary;

        struct
        {
            IRInst *target;
        } branch;

        struct
        {
            IRInst *cond;
            IRInst *true_target;
            IRInst *false_target;
            IRInst *merge_target;
        } cond_branch;
    };
};

struct IRModule
{
    TsCompiler *compiler;
    Module *mod;

    HashMap type_cache;

    /*array*/ IRInst **entry_points;
    /*array*/ IRInst **constants;
    /*array*/ IRInst **functions;
    /*array*/ IRInst **globals;     /* This only counts uniforms/storage variables */
    /*array*/ IRInst **all_globals; /* This counts uniforms/storage variables and all
                                       inputs/outputs of every stage */

    uint32_t glsl_ext_inst; // ID of the imported GLSL instruction set

    IRInst *current_block;

    uint32_t id_bound;
    /*array*/ uint32_t *stream;
};

//
// AST
//

typedef enum AstTypeKind {
    TYPE_VOID,
    TYPE_TYPE,

    TYPE_BOOL,
    TYPE_FLOAT,
    TYPE_INT,

    TYPE_VECTOR,
    TYPE_MATRIX,

    TYPE_POINTER,
    TYPE_FUNC,
    TYPE_STRUCT,

    TYPE_SAMPLER,
    TYPE_IMAGE,
    TYPE_SAMPLED_IMAGE,
} AstTypeKind;

typedef struct AstType
{
    AstTypeKind kind;
    char *string;
    uint32_t size;
    uint32_t align;

    union
    {
        struct
        {
            uint32_t bits;
        } float_;
        struct
        {
            uint32_t bits;
            bool is_signed;
        } int_;
        struct
        {
            SpvStorageClass storage_class;
            struct AstType *sub;
        } ptr;
        struct
        {
            uint32_t size;
            struct AstType *elem_type;
        } vector;
        struct
        {
            uint32_t col_count;
            struct AstType *col_type;
        } matrix;
        struct
        {
            struct AstType *return_type;

            struct AstType **params;
            uint32_t param_count;
        } func;
        struct
        {
            char *name;

            AstDecl **field_decls;
            struct AstType **fields;
            /*array*/ IRDecoration *decorations;
            /*array*/ IRMemberDecoration *field_decorations;
            uint32_t field_count;
        } struct_;
        struct
        {
            struct AstType *sampled_type;
            SpvDim dim;
            uint32_t depth;
            uint32_t arrayed;
            uint32_t multisampled;
            uint32_t sampled;
            SpvImageFormat format;
        } image;
        struct
        {
            struct AstType *image_type;
        } sampled_image;
    };
} AstType;

typedef struct AstAttribute
{
    char *name;
    /*array*/ AstExpr **values;
} AstAttribute;

typedef enum AstUnaryOp {
    UNOP_NEG,
    UNOP_NOT,
} AstUnaryOp;

typedef enum AstBinaryOp {
    BINOP_ADD,
    BINOP_SUB,
    BINOP_MUL,
    BINOP_DIV,
    BINOP_MOD,

    BINOP_EQ,
    BINOP_NOTEQ,
    BINOP_LESS,
    BINOP_LESSEQ,
    BINOP_GREATER,
    BINOP_GREATEREQ,
} AstBinaryOp;

typedef enum AstVarKind {
    VAR_PLAIN = 0,
    VAR_UNIFORM,
    VAR_IN_PARAM,
    VAR_OUT_PARAM,
    VAR_INOUT_PARAM,
} AstVarKind;

typedef enum AstStmtKind {
    STMT_DECL,
    STMT_EXPR,
    STMT_VAR_ASSIGN,
    STMT_RETURN,
    STMT_BLOCK,
    STMT_IF,
} AstStmtKind;

typedef enum AstDeclKind {
    DECL_FUNC,
    DECL_VAR,
    DECL_CONST,

    DECL_STRUCT,
    DECL_STRUCT_FIELD,
} AstDeclKind;

typedef enum AstExprKind {
    EXPR_PRIMARY,
    EXPR_IDENT,
    EXPR_ACCESS,
    EXPR_SAMPLER_TYPE,
    EXPR_TEXTURE_TYPE,
    EXPR_FUNC_CALL,
    EXPR_BUILTIN_CALL,
    EXPR_UNARY,
    EXPR_BINARY,
} AstExprKind;

struct AstStmt
{
    AstStmtKind kind;
    Location loc;

    union
    {
        AstDecl *decl;
        AstExpr *expr;
        struct
        {
            AstExpr *assigned_expr;
            AstExpr *value_expr;
        } var_assign;
        struct
        {
            AstExpr *value;
        } return_;

        struct
        {
            /*array*/ AstStmt **stmts;
            Scope *scope;
        } block;
        
        struct 
        {
            AstExpr *cond;
            AstStmt *if_stmt;
            AstStmt *else_stmt;
        } if_;
    };
};

struct AstDecl
{
    AstDeclKind kind;
    Location loc;
    char *name;
    AstType *type;
    AstType *as_type;
    IRInst *value;
    Scope *scope;
    /*array*/ AstAttribute *attributes;
    /*array*/ IRDecoration *decorations;
    int64_t *resolved_int;

    union
    {
        struct
        {
            SpvExecutionModel *execution_model;

            /*array*/ AstStmt **stmts;
            /*array*/ AstDecl **all_params;
            AstExpr *return_type;

            // To be filled later:
            /*array*/ AstDecl **var_decls;
            /*array*/ AstDecl **func_params;
            /*array*/ AstDecl **inputs;
            /*array*/ AstDecl **outputs;

            bool called; // if this function was called or if it's an entry point
        } func;

        struct
        {
            AstExpr *type_expr;
            AstExpr *value_expr;
            char *semantic;
            AstVarKind kind;
        } var;

        struct
        {
            AstExpr *type_expr;
            AstExpr *value_expr;
        } constant;

        struct
        {
            /*array*/ AstDecl **fields;
        } struct_;

        struct
        {
            AstExpr *type_expr;
            uint32_t index;
            char *semantic;
        } struct_field;

        struct
        {
            uint32_t set_index;
            /*array*/ AstDecl **params;
        } parameter_block;
    };
};

struct AstExpr
{
    AstExprKind kind;
    Location loc;
    IRInst *value;
    AstType *type;
    AstType *as_type;
    bool assignable;
    Scope *inhabited_scope;
    Scope *scope;
    int64_t *resolved_int;

    union
    {
        struct
        {
            Token *token;
        } primary;

        struct
        {
            char *name;

            uint32_t *shuffle_indices;
            uint32_t shuffle_index_count;
            AstDecl *decl; // The declaration this identifier refers to
        } ident;

        struct
        {
            AstExpr *base;
            /*array*/ AstExpr **chain;
        } access;

        struct
        {
            AstExpr *func_expr;
            /*array*/ AstExpr **params;
            AstExpr *self_param;
        } func_call;

        struct
        {
            IRBuiltinInstKind kind;
            /*array*/ AstExpr **params;
        } builtin_call;

        struct
        {
            AstExpr *sampled_type_expr;
            SpvDim dim;
        } texture;

        struct
        {
            AstUnaryOp op;
            AstExpr *right;
        } unary;

        struct
        {
            AstBinaryOp op;
            AstExpr *left;
            AstExpr *right;
        } binary;
    };
};

//
// Scope
//

struct Scope
{
    struct Scope *parent;
    AstDecl *owner;
    HashMap map; // string -> *AstDecl
};

//
// Compiler
//

typedef struct TsCompiler
{
    BumpAlloc alloc;
    StringBuilder sb;

    HashMap keyword_table;
    HashMap files; // Maps absolute paths to files

    /*array*/ File **file_queue;
    /*array*/ Error *errors;
} TsCompiler;

//
// File
//

struct File
{
    char *text;
    size_t text_size;

    char *path;
    char *dir;

    /*array*/ Token *tokens;
    /*array*/ AstDecl **decls;
};

struct Module
{
    TsCompiler *compiler;
    Scope *scope;
    /*array*/ File **files;

    const char *entry_point; // Requested entry point name
    TsShaderStage stage;

    HashMap type_cache;
};

//
// Lexer
//

typedef struct Lexer
{
    TsCompiler *compiler;
    File *file;

    Token token;

    size_t pos;
    uint32_t line;
    uint32_t col;
} Lexer;

typedef struct Parser
{
    TsCompiler *compiler;
    File *file;

    size_t pos;
} Parser;

typedef struct Analyzer
{
    TsCompiler *compiler;
    Module *module;

    AstDecl *scope_func;
    Scope **scope_stack;
} Analyzer;

////////////////////////////////
//
// Functions
//
////////////////////////////////

void ts__hashInit(HashMap *map, uint64_t size);
void *ts__hashSet(HashMap *map, const char *key, void *value);
bool ts__hashGet(HashMap *map, const char *key, void **result);
void ts__hashDestroy(HashMap *map);

void ts__bumpInit(BumpAlloc *alloc, size_t block_size);
void *ts__bumpAlloc(BumpAlloc *alloc, size_t size);
void *ts__bumpZeroAlloc(BumpAlloc *alloc, size_t size);
char *ts__bumpStrndup(BumpAlloc *alloc, const char *str, size_t length);
void ts__bumpDestroy(BumpAlloc *alloc);

void ts__sbInit(StringBuilder *sb);
void ts__sbDestroy(StringBuilder *sb);
void ts__sbReset(StringBuilder *sb);
void ts__sbAppend(StringBuilder *sb, const char *str);
void ts__sbAppendChar(StringBuilder *sb, char c);
void ts__sbSprintf(StringBuilder *sb, const char *fmt, ...);
char *ts__sbBuildMalloc(StringBuilder *sb);
char *ts__sbBuild(StringBuilder *sb, BumpAlloc *bump);

void ts__addErr(TsCompiler *compiler, Location *loc, const char *msg);

void ts__lexerLex(Lexer *l, TsCompiler *compiler, File *file);
void ts__parserParse(Parser *p, TsCompiler *compiler, File *file);
void ts__analyzerAnalyze(Analyzer *a, TsCompiler *compiler, Module *module);

AstType *ts__getScalarType(AstType *type);
AstType *ts__getComparableType(AstType *type);
AstType *ts__getLogicalType(AstType *type);
AstType *ts__getElemType(AstType *type);

uint32_t *ts__irModuleCodegen(Module *mod, size_t *word_count);

#endif