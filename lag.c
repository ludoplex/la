#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef enum {
    TYPE_FLOAT = 0,
    TYPE_DOUBLE,
    TYPE_INT,
    COUNT_TYPES,
} Type;

typedef struct {
    const char *name;
    const char *suffix;
} Type_Def;

static_assert(COUNT_TYPES == 3, "The amount of type definitions have changed. Please update the array bellow accordingly");
static Type_Def type_defs[COUNT_TYPES] = {
    [TYPE_FLOAT]        = {.name = "float", .suffix = "f"},
    [TYPE_DOUBLE]       = {.name = "double", .suffix = "d"},
    [TYPE_INT]          = {.name = "int", .suffix = "i"},
};

typedef enum {
    OP_SUM = 0,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    COUNT_OPS,
} Op_Type;

typedef struct {
    const char *suffix;
    const char *op;
} Op_Def;

static_assert(COUNT_OPS == 4, "The amount of operator definitions have changed. Please update the array below accordingly");
static Op_Def op_defs[COUNT_OPS] = {
    [OP_SUM] = {.suffix = "sum", .op = "+="},
    [OP_SUB] = {.suffix = "sub", .op = "-="},
    [OP_MUL] = {.suffix = "mul", .op = "*="},
    [OP_DIV] = {.suffix = "div", .op = "/="},
};

typedef struct {
    char data[128];
} Short_String;

#if defined(__GNUC__) || defined(__clang__)
#define CHECK_PRINTF_FMT(a, b) __attribute__ ((format (printf, a, b)))
#else
#define CHECK_PRINTF_FMT(...)
#endif

CHECK_PRINTF_FMT(1, 2) Short_String shortf(const char *fmt, ...)
{
    Short_String result = {0};

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(result.data, sizeof(result.data), fmt, args);
    assert(n >= 0);
    assert((size_t) n + 1 <= sizeof(result.data));
    va_end(args);

    return result;
}

Short_String vector_type(size_t n, Type_Def type_def)
{
    return shortf("V%zu%s", n, type_def.suffix);
}

Short_String vector_prefix(size_t n, Type_Def type_def)
{
    return shortf("v%zu%s", n, type_def.suffix);
}

void gen_vector_def(FILE *stream, size_t n, Type_Def type_def)
{
    fprintf(stream, "typedef struct { %s c[%zu]; } V%zu%s;\n",
            type_def.name, n, n, type_def.suffix);
}

void gen_vector_op_sig(FILE *stream, size_t n, Type_Def type_def, Op_Def op_def)
{
    Short_String type = vector_type(n, type_def);
    Short_String prefix = vector_prefix(n, type_def);

    fprintf(stream, "%s %s_%s(%s a, %s b)",
            type.data,
            prefix.data, op_def.suffix,
            type.data, type.data);
}

void gen_vector_ctor_sig(FILE *stream, size_t n, Type_Def type_def)
{
    Short_String type = vector_type(n, type_def);
    Short_String prefix = vector_prefix(n, type_def);

    fprintf(stream, "%s %s(", type.data, prefix.data);
    for (size_t i = 0; i < n; ++i) {
        if (i > 0) fprintf(stream, ", ");
        fprintf(stream, "%s x%zu", type_def.name, i);
    }
    fprintf(stream, ")");
}

void gen_vector_scalar_ctor_sig(FILE *stream, size_t n, Type_Def type_def)
{
    Short_String type = vector_type(n, type_def);
    Short_String prefix = vector_prefix(n, type_def);

    fprintf(stream, "%s %ss(%s x)", type.data, prefix.data, type_def.name);
}

void gen_vector_scalar_ctor_decl(FILE *stream, size_t n, Type_Def type_def)
{
    gen_vector_scalar_ctor_sig(stream, n, type_def);
    fprintf(stream, ";\n");
}

void gen_vector_scalar_ctor_impl(FILE *stream, size_t n, Type_Def type_def)
{
    gen_vector_scalar_ctor_sig(stream, n, type_def);
    fprintf(stream, "\n");
    fprintf(stream, "{\n");
    fprintf(stream, "    return %s(", vector_prefix(n, type_def).data);
    for (size_t i = 0; i < n; ++i) {
        if (i > 0) fprintf(stream, ", ");
        fprintf(stream, "x");
    }
    fprintf(stream, ");\n");
    fprintf(stream, "}\n");
}

void gen_vector_ctor_decl(FILE *stream, size_t n, Type_Def type_def)
{
    gen_vector_ctor_sig(stream, n, type_def);
    fprintf(stream, ";\n");
}

void gen_vector_ctor_impl(FILE *stream, size_t n, Type_Def type_def)
{
    Short_String type = vector_type(n, type_def);

    gen_vector_ctor_sig(stream, n, type_def);
    fprintf(stream, "\n");
    fprintf(stream, "{\n");
    fprintf(stream, "    %s result;\n", type.data);
    for (size_t i = 0; i < n; ++i) {
        fprintf(stream, "    result.c[%zu] = x%zu;\n", i, i);
    }
    fprintf(stream, "    return result;\n");
    fprintf(stream, "}\n");
}

void gen_vector_op_decl(FILE *stream, size_t n, Type_Def type_def, Op_Def op_def)
{
    gen_vector_op_sig(stream, n, type_def, op_def);
    fprintf(stream, ";\n");
}

void gen_vector_op_impl(FILE *stream, size_t n, Type_Def type_def, Op_Def op_def)
{
    gen_vector_op_sig(stream, n, type_def, op_def);
    fprintf(stream, "\n");
    fprintf(stream, "{\n");
    fprintf(stream, "    for (int i = 0; i < %zu; ++i) a.c[i] %s b.c[i];\n", n, op_def.op);
    fprintf(stream, "    return a;\n");
    fprintf(stream, "}\n");
}

void gen_vector_ops_decl(FILE *stream, size_t n, Type_Def type_def)
{
    for (Op_Type op = 0; op < COUNT_OPS; ++op) {
        gen_vector_op_decl(stream, n, type_def, op_defs[op]);
    }
}

void gen_vector_ops_impl(FILE *stream, size_t n, Type_Def type_def)
{
    for (Op_Type op = 0; op < COUNT_OPS; ++op) {
        gen_vector_op_impl(stream, n, type_def, op_defs[op]);
    }
}

// TODO: sqrt operation for vectors
// TODO: pow operation for vectors
// TODO: lerp operation for vectors
// TODO: len operation for vectors
// TODO: sqrlen operation for vectors
// TODO: min operation for vectors
// TODO: max operation for vectors
// TODO: matrices
// TODO: macro blocks to disable certain sizes, types, etc

int main()
{
    // Header Part
    {
        fprintf(stdout, "#ifndef LA_H_\n");
        fprintf(stdout, "#define LA_H_\n");
        fprintf(stdout, "\n");

        for (size_t n = 2; n <= 4; ++n) {
            for (Type type = 0; type < COUNT_TYPES; ++type) {
                gen_vector_def(stdout, n, type_defs[type]);
                gen_vector_ops_decl(stdout, n, type_defs[type]);
                gen_vector_ctor_decl(stdout, n, type_defs[type]);
                gen_vector_scalar_ctor_decl(stdout, n, type_defs[type]);
                printf("\n");
            }
        }

        fprintf(stdout, "#endif // LA_H_\n");
        fprintf(stdout, "\n");
    }

    // C part
    {
        fprintf(stdout, "#ifdef LA_IMPLEMENTATION\n");
        fprintf(stdout, "\n");

        for (size_t n = 2; n <= 4; ++n) {
            for (Type type = 0; type < COUNT_TYPES; ++type) {
                gen_vector_ops_impl(stdout, n, type_defs[type]);
                printf("\n");
                gen_vector_ctor_impl(stdout, n, type_defs[type]);
                printf("\n");
                gen_vector_scalar_ctor_impl(stdout, n, type_defs[type]);
                printf("\n");
            }
        }

        fprintf(stdout, "#endif // LA_IMPLEMENTATION\n");
    }

    return 0;
}
