#include "type_system.h"

/*
 * Conversion between ASTType and SemType
 */
SemType sem_type_from_ast_type(ASTType ast_type) {
    switch (ast_type) {
        case TYPE_INT:   return SEM_TYPE_INT;
        case TYPE_FLOAT: return SEM_TYPE_FLOAT;
        case TYPE_CHAR:  return SEM_TYPE_CHAR;
        case TYPE_VOID:  return SEM_TYPE_VOID;
        default:         return SEM_TYPE_ERROR;
    }
}

ASTType sem_type_to_ast_type(SemType sem_type) {
    switch (sem_type) {
        case SEM_TYPE_INT:   return TYPE_INT;
        case SEM_TYPE_FLOAT: return TYPE_FLOAT;
        case SEM_TYPE_CHAR:  return TYPE_CHAR;
        case SEM_TYPE_VOID:  return TYPE_VOID;
        default:             return TYPE_INT;
    }
}

const char *sem_type_name(SemType type) {
    switch (type) {
        case SEM_TYPE_VOID:  return "void";
        case SEM_TYPE_INT:   return "int";
        case SEM_TYPE_FLOAT: return "float";
        case SEM_TYPE_CHAR:  return "char";
        case SEM_TYPE_BOOL:  return "bool (internal)";
        case SEM_TYPE_ERROR: return "error";
        default:             return "unknown";
    }
}

/*
 * Type Identity
 */
int type_is_same(SemType a, SemType b) {
    if (a == SEM_TYPE_ERROR || b == SEM_TYPE_ERROR) return 0;
    return (a == b);
}

/*
 * Implicit Conversion Matrix (Section 19 & Appendix B)
 * Only int -> float widening conversion is allowed across different value types.
 */
int type_can_implicitly_convert(SemType src, SemType dest) {
    if (src == SEM_TYPE_ERROR || dest == SEM_TYPE_ERROR ||
        src == SEM_TYPE_VOID  || dest == SEM_TYPE_VOID  ||
        src == SEM_TYPE_BOOL  || dest == SEM_TYPE_BOOL) {
        return 0;
    }

    if (src == dest) {
        return 1;
    }

    /* Only widening numeric conversion int -> float is allowed implicitly */
    if (src == SEM_TYPE_INT && dest == SEM_TYPE_FLOAT) {
        return 1;
    }

    return 0;
}

/*
 * Explicit Cast Matrix (Section 18 & Appendix B)
 * C-style explicit casts are supported between all combinations of int, float, and char.
 */
int type_can_explicit_cast(SemType src, SemType target) {
    int src_valid = (src == SEM_TYPE_INT || src == SEM_TYPE_FLOAT || src == SEM_TYPE_CHAR);
    int target_valid = (target == SEM_TYPE_INT || target == SEM_TYPE_FLOAT || target == SEM_TYPE_CHAR);
    return (src_valid && target_valid);
}

/*
 * Binary Arithmetic Rules (+, -, *, /) (Section 20.1)
 */
SemType type_arithmetic_result(TokenType op, SemType left, SemType right) {
    if (op == TOKEN_MODULO) {
        if (left == SEM_TYPE_INT && right == SEM_TYPE_INT) {
            return SEM_TYPE_INT;
        }
        return SEM_TYPE_ERROR;
    }

    if (op == TOKEN_PLUS || op == TOKEN_MINUS || op == TOKEN_MULTIPLY || op == TOKEN_DIVIDE) {
        if (left == SEM_TYPE_INT && right == SEM_TYPE_INT) {
            return SEM_TYPE_INT;
        }
        if ((left == SEM_TYPE_INT && right == SEM_TYPE_FLOAT) ||
            (left == SEM_TYPE_FLOAT && right == SEM_TYPE_INT) ||
            (left == SEM_TYPE_FLOAT && right == SEM_TYPE_FLOAT)) {
            return SEM_TYPE_FLOAT;
        }
    }

    return SEM_TYPE_ERROR;
}

/*
 * Modulus Rules (%) (Section 20.1)
 */
int type_is_valid_modulus(SemType left, SemType right) {
    return (left == SEM_TYPE_INT && right == SEM_TYPE_INT);
}

/*
 * Relational and Equality Rules (<, <=, >, >=, ==, !=) (Section 20.2)
 */
int type_is_valid_comparison(SemType left, SemType right) {
    if (left == SEM_TYPE_CHAR && right == SEM_TYPE_CHAR) {
        return 1;
    }
    if ((left == SEM_TYPE_INT || left == SEM_TYPE_FLOAT) &&
        (right == SEM_TYPE_INT || right == SEM_TYPE_FLOAT)) {
        return 1;
    }
    return 0;
}

SemType type_comparison_result(TokenType op, SemType left, SemType right) {
    (void)op;
    if (type_is_valid_comparison(left, right)) {
        return SEM_TYPE_BOOL;
    }
    return SEM_TYPE_ERROR;
}

/*
 * Logical Operator Rules (&&, ||, !) (Section 20.3)
 */
int type_is_valid_logical_operand(SemType type) {
    return (type == SEM_TYPE_BOOL);
}

SemType type_logical_result(TokenType op, SemType left, SemType right) {
    if (op == TOKEN_LOGICAL_AND || op == TOKEN_LOGICAL_OR) {
        if (left == SEM_TYPE_BOOL && right == SEM_TYPE_BOOL) {
            return SEM_TYPE_BOOL;
        }
    }
    return SEM_TYPE_ERROR;
}

SemType type_unary_logical_result(TokenType op, SemType operand) {
    if (op == TOKEN_LOGICAL_NOT) {
        if (operand == SEM_TYPE_BOOL) {
            return SEM_TYPE_BOOL;
        }
    }
    return SEM_TYPE_ERROR;
}

/*
 * Increment / Decrement Rules (++, --) (Section 20.4)
 */
int type_is_valid_increment_type(SemType type) {
    return (type == SEM_TYPE_INT || type == SEM_TYPE_FLOAT);
}

/*
 * Compound Assignment Rules (+=, -=, *=, /=, %=) (Section 20.5)
 */
int type_is_valid_compound_assignment(TokenType op, SemType target, SemType value) {
    TokenType base_op;
    switch (op) {
        case TOKEN_PLUS_ASSIGN:     base_op = TOKEN_PLUS; break;
        case TOKEN_MINUS_ASSIGN:    base_op = TOKEN_MINUS; break;
        case TOKEN_MULTIPLY_ASSIGN: base_op = TOKEN_MULTIPLY; break;
        case TOKEN_DIVIDE_ASSIGN:   base_op = TOKEN_DIVIDE; break;
        case TOKEN_MODULO_ASSIGN:   base_op = TOKEN_MODULO; break;
        default: return 0;
    }

    SemType result_type = type_arithmetic_result(base_op, target, value);
    if (result_type == SEM_TYPE_ERROR) {
        return 0;
    }

    return type_can_implicitly_convert(result_type, target);
}

/*
 * Assignment Compatibility (=)
 */
int type_is_valid_assignment(SemType target, SemType value) {
    return type_can_implicitly_convert(value, target);
}

/*
 * Function Return Compatibility (Section 15 & 16.7)
 */
int type_is_valid_return(SemType func_return_type, SemType expr_type) {
    if (func_return_type == SEM_TYPE_VOID) {
        return (expr_type == SEM_TYPE_VOID);
    }
    return type_can_implicitly_convert(expr_type, func_return_type);
}

/*
 * Array Index Compatibility (Section 14)
 */
int type_is_valid_array_index(SemType index_type) {
    return (index_type == SEM_TYPE_INT);
}
