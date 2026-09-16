#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include "token.h"
#include "ast.h"

/*
 * Semantic Value Types.
 * Represents expression and variable data types during semantic analysis.
 * SEM_TYPE_BOOL is internal only (produced by relational and logical operators).
 * The language does not expose a user-visible bool type.
 */
typedef enum {
    SEM_TYPE_VOID,
    SEM_TYPE_INT,
    SEM_TYPE_FLOAT,
    SEM_TYPE_CHAR,
    SEM_TYPE_STRING,   /* Internal semantic type for string literals */
    SEM_TYPE_BOOL,     /* Internal semantic type for conditions and logical operations */
    SEM_TYPE_ERROR     /* Error propagation sentinel */
} SemType;

/*
 * Conversion between ASTType (source syntax) and SemType (semantic analysis)
 */
SemType sem_type_from_ast_type(ASTType ast_type);
ASTType sem_type_to_ast_type(SemType sem_type);
const char *sem_type_name(SemType type);

/*
 * Type Identity
 */
int type_is_same(SemType a, SemType b);

/*
 * Implicit Conversion Matrix (Section 19 & Appendix B)
 * Only int -> float widening conversion is allowed across different value types.
 */
int type_can_implicitly_convert(SemType src, SemType dest);

/*
 * Explicit Cast Matrix (Section 18 & Appendix B)
 * C-style explicit casts are supported between all combinations of int, float, and char.
 */
int type_can_explicit_cast(SemType src, SemType target);

/*
 * Binary Arithmetic Rules (+, -, *, /) (Section 20.1)
 * Evaluates operand promotion and returns result type, or SEM_TYPE_ERROR.
 */
SemType type_arithmetic_result(TokenType op, SemType left, SemType right);

/*
 * Modulus Rules (%) (Section 20.1)
 * Modulus is restricted strictly to int % int -> int.
 */
int type_is_valid_modulus(SemType left, SemType right);

/*
 * Relational and Equality Rules (<, <=, >, >=, ==, !=) (Section 20.2)
 * Comparisons produce an internal SEM_TYPE_BOOL.
 */
int type_is_valid_comparison(SemType left, SemType right);
SemType type_comparison_result(TokenType op, SemType left, SemType right);

/*
 * Logical Operator Rules (&&, ||, !) (Section 20.3)
 * Operands must be boolean-valued (SEM_TYPE_BOOL). General C truthiness is disallowed.
 */
int type_is_valid_logical_operand(SemType type);
SemType type_logical_result(TokenType op, SemType left, SemType right);
SemType type_unary_logical_result(TokenType op, SemType operand);

/*
 * Increment / Decrement Rules (++, --) (Section 20.4)
 * Allowed on modifiable int and float types. Not allowed on char.
 */
int type_is_valid_increment_type(SemType type);

/*
 * Compound Assignment Rules (+=, -=, *=, /=, %=) (Section 20.5)
 * Final assigned value must be compatible with left-hand type using implicit conversion rules.
 */
int type_is_valid_compound_assignment(TokenType op, SemType target, SemType value);

/*
 * Assignment Compatibility (=)
 * Target receives value if value is implicitly convertible to target.
 */
int type_is_valid_assignment(SemType target, SemType value);

/*
 * Function Return Compatibility (Section 15 & 16.7)
 * Expression returned must be implicitly convertible to function return type.
 */
int type_is_valid_return(SemType func_return_type, SemType expr_type);

/*
 * Array Index Compatibility (Section 14)
 * Array indices must have integer type (SEM_TYPE_INT).
 */
int type_is_valid_array_index(SemType index_type);

#endif /* TYPE_SYSTEM_H */
